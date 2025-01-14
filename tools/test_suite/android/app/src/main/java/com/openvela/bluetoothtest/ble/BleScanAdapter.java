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

package com.openvela.bluetoothtest.ble;

import java.util.ArrayList;
import java.util.List;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.text.TextUtils;
import android.view.View;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanRecord;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;

import androidx.core.os.HandlerCompat;

import com.openvela.bluetooth.adapter.RecyclerAdapter;
import com.openvela.bluetooth.adapter.RecyclerViewHolder;
import com.openvela.bluetooth.BtDevice;
import com.openvela.bluetooth.callback.BluetoothDiscoveryCallback;
import com.openvela.bluetoothtest.R;

public class BleScanAdapter extends RecyclerAdapter<BtDevice> {
    private final String TAG = BleScanAdapter.class.getSimpleName();
    private static final String SCAN_TIMEOUT_TOKEN = "scan_timeout_token";
    private static final int RSSI_UPDATE_INTERVAL_MS = 2 * 1000;
    private final Handler handler = new Handler(Looper.myLooper());
    private final BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    private volatile BluetoothLeScanner bluetoothScanner;
    private ScanSettings scanSettings;
    private String[] scanFilters;
    private BluetoothDiscoveryCallback<BtDevice> bleDiscoveryCallback;
    private int view_position = -1;

    public BleScanAdapter(Context context) {
        super(context, R.layout.item_scan_result, new ArrayList<>());
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
            viewHolder.setVisibility(R.id.ll_detail, View.VISIBLE);

            ScanRecord scanRecord = device.getScanRecord();
            if (scanRecord != null) {
                // Flags
                if (scanRecord.getAdvertiseFlags() >= 0) {
                    viewHolder.setVisibility(R.id.tv_flags, View.VISIBLE);
                    viewHolder.setText(R.id.tv_flags, "Flags: 0x" + String.format("%02x", scanRecord.getAdvertiseFlags()));
                }
                // Local Name
                if (scanRecord.getDeviceName() != null) {
                    viewHolder.setVisibility(R.id.tv_local_name, View.VISIBLE);
                    viewHolder.setText(R.id.tv_local_name, "Local Name: "+ scanRecord.getDeviceName());
                }
                // Service UUIDs
                List<ParcelUuid> serviceUuids = scanRecord.getServiceUuids();
                if (serviceUuids != null && !serviceUuids.isEmpty()){
                    viewHolder.setVisibility(R.id.tv_uuid, View.VISIBLE);
                    viewHolder.setText(R.id.tv_uuid, String.format("Service Uuids: %s", TextUtils.join(", ", serviceUuids)));
                }
                // Raw Data
                byte[] rawData = scanRecord.getBytes();
                int totalLength = 0;
                do {
                    totalLength += rawData[totalLength] + 1;
                } while (rawData[totalLength] != 0);

                StringBuilder builder = new StringBuilder();
                builder.append("RAW: 0x");
                for (int i = 0; i < totalLength; i++) {
                    builder.append(String.format("%02x", rawData[i]));
                }
                viewHolder.setText(R.id.tv_raw_data, builder.toString());
            }

        } else {
            viewHolder.setVisibility(R.id.ll_detail, View.GONE);
        }

        if (device.isConnectable()) {
            viewHolder.setVisibility(R.id.tv_connect, View.VISIBLE);
            viewHolder.setOnClickListener(R.id.tv_connect, v -> {
                if (isScanning()) {
                    stopScan();
                }
                mContext.startActivity(new Intent(mContext, BleCentralActivity.class)
                        .putExtra(BleCentralActivity.EXTRA_TAG, device));
            });
        } else {
            viewHolder.setVisibility(R.id.tv_connect, View.GONE);
        }

        viewHolder.setOnClickListener(v -> {
            if(viewHolder.getBindingAdapterPosition() == view_position) {
                notifyItemChanged(view_position);
                view_position = -1;
            } else {
                notifyItemChanged(view_position);
                view_position = viewHolder.getBindingAdapterPosition();
                notifyItemChanged(view_position);
            }
        });
    }

    public boolean isScanning() {
        return (bluetoothScanner != null);
    }

    @SuppressLint("NotifyDataSetChanged")
    public void startScan(final String[] scanFilters, long scanPeriod, BluetoothDiscoveryCallback<BtDevice> callback) {
        bleDiscoveryCallback = callback;

        if (!bluetoothAdapter.isEnabled()) {
            if (bleDiscoveryCallback != null) {
                bleDiscoveryCallback.onDiscoveryFailed(BluetoothDiscoveryCallback.FAILED_INTERNAL_ERROR);
                return;
            }
        }

        if (isScanning()) {
            if (bleDiscoveryCallback != null){
                bleDiscoveryCallback.onDiscoveryFailed(BluetoothDiscoveryCallback.FAILED_ALREADY_STARTED);
            }
            return;
        }

        bluetoothScanner = bluetoothAdapter.getBluetoothLeScanner();
        if (bluetoothScanner != null) {
            super.mItems.clear();
            super.notifyDataSetChanged();
            startScanTimer(scanPeriod);

            this.scanFilters = scanFilters;
            scanSettings = new ScanSettings.Builder()
                            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                            .setReportDelay(0L)
                            .build();
            bluetoothScanner.startScan(null, scanSettings, scanCallback);
            bleDiscoveryCallback.onStart();
        } else {
            bleDiscoveryCallback.onDiscoveryFailed(BluetoothDiscoveryCallback.FAILED_INTERNAL_ERROR);
        }
    }

    public void stopScan() {
        if (!isScanning()) {
            return;
        }

        cancelScanTimer();
        bluetoothScanner.stopScan(scanCallback);
        bleDiscoveryCallback.onStop();
        bluetoothScanner = null;
    }

    private void cancelScanTimer() {
        handler.removeCallbacksAndMessages(SCAN_TIMEOUT_TOKEN);
    }

    private void startScanTimer(long scanPeriod) {
        cancelScanTimer();

        if (scanPeriod >= 0){
            HandlerCompat.postDelayed(handler, () -> {
                if (isScanning()) {
                    stopScan();
                }
            }, SCAN_TIMEOUT_TOKEN, scanPeriod);
        }
    }

    private final ScanCallback scanCallback = new ScanCallback() {
        @SuppressLint("NotifyDataSetChanged")
        @Override
        public void onScanResult(final int callbackType, final ScanResult result) {
            synchronized (this) {
                final String address = result.getDevice().getAddress();
                final String name = result.getDevice().getName();
                boolean found = true;

                if (scanFilters != null) {
                    found = false;
                    for (String filter : scanFilters) {
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
                        if (device.getRssi() != result.getRssi() && System.currentTimeMillis() - device.getRssiUpdateTime() > RSSI_UPDATE_INTERVAL_MS) {
                            device.setRssi(result.getRssi());
                            device.setRssiUpdateTime(System.currentTimeMillis());
                            mItems.set(i, device);
                            notifyItemChanged(i);
                        }
                        return;
                    }
                }

                BtDevice newDevice = new BtDevice(address, name);
                newDevice.setConnectable(result.isConnectable());
                newDevice.setScanRecord(result.getScanRecord());
                newDevice.setRssi(result.getRssi());
                newDevice.setRssiUpdateTime(System.currentTimeMillis());
                mItems.add(newDevice);
                notifyDataSetChanged();

                if (bleDiscoveryCallback != null) {
                    bleDiscoveryCallback.onDiscoveryResult(newDevice);
                }
            }
        }

        @Override
        public void onScanFailed(final int errorCode) {
            stopScan();
            if (bleDiscoveryCallback != null) {
                bleDiscoveryCallback.onDiscoveryFailed(errorCode);
            }
        }
    };

}
