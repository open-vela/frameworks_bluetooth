/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

package com.openvela.bluetoothtest.bredr;

import java.util.ArrayList;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.view.View;

import android.bluetooth.BluetoothAdapter;

import androidx.core.os.HandlerCompat;

import com.openvela.bluetooth.adapter.RecyclerAdapter;
import com.openvela.bluetooth.adapter.RecyclerViewHolder;
import com.openvela.bluetooth.BluetoothDiscoveryObserver;
import com.openvela.bluetooth.BtDevice;
import com.openvela.bluetooth.callback.BluetoothDiscoveryCallback;

import com.openvela.bluetoothtest.R;

public class BredrInquiryAdapter extends RecyclerAdapter<BtDevice> {
    private final String TAG = BredrInquiryAdapter.class.getSimpleName();
    private static final String DISCOVERY_TIMEOUT_TOKEN = "discovery_timeout_token";
    private static final int RSSI_UPDATE_INTERVAL_MS = 2 * 1000;
    private final Handler handler = new Handler(Looper.myLooper());
    private final BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    private volatile BluetoothDiscoveryObserver bluetoothDiscoveryObserver;
    private String[] discoveryFilters;
    private BluetoothDiscoveryCallback<BtDevice> bluetoothDiscoveryCallback;
    private int view_position = -1;

    public BredrInquiryAdapter(Context context) {
        super(context, R.layout.item_inquiry_result, new ArrayList<>());
        bluetoothDiscoveryObserver = new BluetoothDiscoveryObserver(context);
        bluetoothDiscoveryObserver.registerReceiver(discoveryCallback);
    }

    @SuppressLint("DefaultLocale")
    @Override
    public void onBindViewHolderItem(RecyclerViewHolder viewHolder, BtDevice device) {
        viewHolder.setText(R.id.tv_address, device.getAddress());
        viewHolder.setText(R.id.tv_rssi, String.format("%ddBm", device.getRssi()));
        if (device.getName() == null){
            viewHolder.setText(R.id.tv_name, "Unknown");
        } else {
            viewHolder.setText(R.id.tv_name, device.getName());
        }

        if (viewHolder.getBindingAdapterPosition() == view_position){

        } else {
            viewHolder.setVisibility(R.id.ll_detail, View.GONE);
        }
    }

    public boolean isDiscovering() {
        return bluetoothDiscoveryObserver.isDiscovering();
    }

    public void startDiscovery(final String[] discoveryFilters, long discoveryPeriod, BluetoothDiscoveryCallback<BtDevice> callback) {
        bluetoothDiscoveryCallback = callback;

        if (!bluetoothAdapter.isEnabled()) {
            if (bluetoothDiscoveryCallback != null) {
                bluetoothDiscoveryCallback.onDiscoveryFailed(BluetoothDiscoveryCallback.FAILED_INTERNAL_ERROR);
                return;
            }
        }

        if (isDiscovering()) {
            if (bluetoothDiscoveryCallback != null){
                bluetoothDiscoveryCallback.onDiscoveryFailed(BluetoothDiscoveryCallback.FAILED_ALREADY_STARTED);
            }
            return;
        }

        super.mItems.clear();
        startDiscoveryTimer(discoveryPeriod);

        this.discoveryFilters = discoveryFilters;
        bluetoothDiscoveryObserver.startDiscovery();
    }

    public void stopDiscovery() {
        if (!isDiscovering()) {
            return;
        }

        cancelDiscoveryTimer();
        bluetoothDiscoveryObserver.cancelDiscovery();
    }

    private void cancelDiscoveryTimer() {
        handler.removeCallbacksAndMessages(DISCOVERY_TIMEOUT_TOKEN);
    }

    private void startDiscoveryTimer(long discoveryPeriod) {
        cancelDiscoveryTimer();

        if (discoveryPeriod >= 0){
            HandlerCompat.postDelayed(handler, () -> {
                if (isDiscovering()) {
                    stopDiscovery();
                }
            }, DISCOVERY_TIMEOUT_TOKEN, discoveryPeriod);
        }
    }

    private final BluetoothDiscoveryCallback<BtDevice> discoveryCallback = new BluetoothDiscoveryCallback<BtDevice>() {
        @Override
        public void onDiscoveryResult(final BtDevice foundDevice) {
            final String address = foundDevice.getAddress();
            final String name = foundDevice.getName();
            boolean found = true;

            if (discoveryFilters != null) {
                found = false;
                for (String filter : discoveryFilters) {
                    if ((address != null && address.toLowerCase().contains(filter.toLowerCase())) ||
                        (name != null && name.toLowerCase().contains(filter.toLowerCase()))) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                return;
            }

            for (int i = 0; i < getItemCount(); i++) {
                BtDevice device = mItems.get(i);
                if (TextUtils.equals(device.getAddress(), address)) {
                    if (device.getRssi() != foundDevice.getRssi() && System.currentTimeMillis() - device.getRssiUpdateTime() > RSSI_UPDATE_INTERVAL_MS) {
                        device.setRssi(foundDevice.getRssi());
                        device.setRssiUpdateTime(System.currentTimeMillis());
                        mItems.set(i, device);
                        notifyItemChanged(i);
                    }
                    return;
                }
            }

            BtDevice newDevice = new BtDevice(address, name);
            newDevice.setRssi(foundDevice.getRssi());
            newDevice.setRssiUpdateTime(System.currentTimeMillis());
            mItems.add(newDevice);
            notifyDataSetChanged();

            if (bluetoothDiscoveryCallback != null) {
                bluetoothDiscoveryCallback.onDiscoveryResult(newDevice);
            }
        }

        @Override
        public void onStart() {
            if (bluetoothDiscoveryCallback != null) {
                bluetoothDiscoveryCallback.onStart();
            }
        }

        @Override
        public void onStop() {
            if (bluetoothDiscoveryCallback != null) {
                bluetoothDiscoveryCallback.onStop();
            }
        }
    };
}
