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
import java.util.HashMap;
import java.util.Map;
import android.view.View;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;

import androidx.core.os.HandlerCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.openvela.bluetooth.adapter.RecyclerAdapter;
import com.openvela.bluetooth.adapter.RecyclerViewHolder;
import com.openvela.bluetooth.BtDevice;
import com.openvela.bluetooth.callback.BleConnectCallback;
import com.openvela.bluetoothtest.R;

public class GattClientAdapter extends RecyclerAdapter<BluetoothGattService> {
    private final static String TAG = GattClientAdapter.class.getSimpleName();
    private static final String BASE_UUID_REGEX = "0000([0-9a-f][0-9a-f][0-9a-f][0-9a-f])-0000-1000-8000-00805f9b34fb";
    private static final int GATT_CONNECT_TIMEOUT_MS = 10 * 1000;
    private final Handler handler = new Handler(Looper.myLooper());
    private final BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    private final Map<String, BluetoothGatt> gattHashMap = new HashMap<>();
    private BleConnectCallback<BtDevice> bleConnectCallback;
    private int view_position = -1;

    public GattClientAdapter(Context context) {
        super(context, R.layout.item_gatt_service, new ArrayList<>());
    }

    @Override
    public void onBindViewHolderItem(RecyclerViewHolder viewHolder, BluetoothGattService gattService) {
        // Service UUID
        String serviceUuid = gattService.getUuid().toString();
        StringBuilder builder = new StringBuilder();
        builder.append("UUID: 0x");
        if (serviceUuid.toLowerCase().matches(BASE_UUID_REGEX)) {
            builder.append(serviceUuid.substring(4, 8).toUpperCase());
        } else {
            builder.append(serviceUuid);
        }
        viewHolder.setText(R.id.tv_service_uuid, builder.toString());
        // Service Type
        if (gattService.getType() == BluetoothGattService.SERVICE_TYPE_PRIMARY) {
            viewHolder.setText(R.id.tv_service_type, "PRIMARY SERVICE");
        } else if (gattService.getType() == BluetoothGattService.SERVICE_TYPE_SECONDARY) {
            viewHolder.setText(R.id.tv_service_type, "SECONDARY SERVICE");
        } else {
            viewHolder.setText(R.id.tv_service_type, "UNKNOWN SERVICE");
        }

        if (viewHolder.getBindingAdapterPosition() == view_position){
            GattClientCharAdapter charAdapter = new GattClientCharAdapter(mContext, gattService.getCharacteristics());
            RecyclerView recyclerView = viewHolder.getView(R.id.recyclerView);
            recyclerView.setLayoutManager(new LinearLayoutManager(mContext, LinearLayoutManager.VERTICAL, false));
            recyclerView.setAdapter(charAdapter);

            viewHolder.setVisibility(R.id.recyclerView, View.VISIBLE);
        } else {
            viewHolder.setVisibility(R.id.recyclerView, View.GONE);
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

    public void connect(BtDevice device, BleConnectCallback<BtDevice> callback) {
        bleConnectCallback = callback;

        String address = device.getAddress();
        final BluetoothDevice bluetoothdevice = bluetoothAdapter.getRemoteDevice(address);
        if (bluetoothdevice == null) {
            bleConnectCallback.onConnectFailed(address, BleConnectCallback.FAILED_DEVICE_NOT_FOUND);
            return;
        }

        BluetoothGatt bluetoothGatt;
        bluetoothGatt = bluetoothdevice.connectGatt(mContext, false, gattCallback, BluetoothDevice.TRANSPORT_LE);
        if (bluetoothGatt != null) {
            startConnectTimer(address);
            bleConnectCallback.onConnectionChanged(address, BluetoothProfile.STATE_CONNECTING);
            gattHashMap.put(address, bluetoothGatt);
        }
    }

    public void disconnect(BtDevice device) {
        String address = device.getAddress();
        BluetoothGatt bluetoothGatt = gattHashMap.get(address);

        if (bluetoothGatt != null) {
            cancelConnectTimer(address);
            bluetoothGatt.disconnect();
            bleConnectCallback.onConnectionChanged(address, BluetoothProfile.STATE_DISCONNECTING);
        }
    }

    public void cancelConnect(BtDevice device) {
        String address = device.getAddress();
        BluetoothGatt bluetoothGatt = gattHashMap.get(address);

        if (bluetoothGatt != null) {
            cancelConnectTimer(address);
            bluetoothGatt.disconnect();
            bleConnectCallback.onConnectionChanged(address, BluetoothProfile.STATE_DISCONNECTED);
        }
    }

    private void cancelConnectTimer(String address){
        handler.removeCallbacksAndMessages(address);
    }

    private void startConnectTimer(String address) {
        cancelConnectTimer(address);

        HandlerCompat.postDelayed(handler, () -> {
            bleConnectCallback.onConnectFailed(address, BleConnectCallback.FAILED_TIMEOUT);
            close(address);
        }, address, GATT_CONNECT_TIMEOUT_MS);
    }

    private void close(String address) {
        BluetoothGatt bluetoothGatt = gattHashMap.get(address);
        if (bluetoothGatt != null) {
            bluetoothGatt.close();
            gattHashMap.remove(address);
        }
    }

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            BluetoothDevice device = gatt.getDevice();
            if (device == null){
                return;
            }

            String address = device.getAddress();
            cancelConnectTimer(address);

            if (status != BluetoothGatt.GATT_SUCCESS) {
                if (bleConnectCallback != null){
                    bleConnectCallback.onConnectFailed(address, status);
                }
                close(address);
                return;
            }
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                if (bleConnectCallback != null) {
                    bleConnectCallback.onConnectionChanged(address, BluetoothProfile.STATE_CONNECTED);
                }
                gatt.discoverServices();
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                if (bleConnectCallback != null) {
                    bleConnectCallback.onConnectionChanged(address, BluetoothProfile.STATE_DISCONNECTED);
                }
                close(address);
            }
        }

        @SuppressLint("NotifyDataSetChanged")
        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            BluetoothDevice device = gatt.getDevice();
            if (device == null){
                return;
            }

            String address = device.getAddress();
            if (status == BluetoothGatt.GATT_SUCCESS) {
                if (bleConnectCallback != null) {
                    bleConnectCallback.onServicesDiscovered(address);
                }
                handler.post(() -> {
                    mItems.clear();
                    mItems.addAll(gatt.getServices());
                    notifyDataSetChanged();
                });
            } else {
                Log.e(TAG, "onServicesDiscovered failed: " + status);
            }
        }

        public void onMtuChanged(BluetoothGatt gatt, int mtu, int status){

        }
    };

}
