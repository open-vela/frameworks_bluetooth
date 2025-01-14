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

package com.openvela.bluetooth;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import com.openvela.bluetooth.callback.BluetoothDiscoveryCallback;

public class BluetoothDiscoveryObserver extends BroadcastReceiver {
    private final Context context;
    private final BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    private BluetoothDiscoveryCallback<BtDevice> bluetoothDiscoveryCallback;

    public BluetoothDiscoveryObserver(Context context){
        this.context = context;
    }

    public boolean isDiscovering() {
        return bluetoothAdapter.isDiscovering();
    }

    public void startDiscovery() {
        bluetoothAdapter.startDiscovery();
    }

    public void cancelDiscovery() {
        bluetoothAdapter.cancelDiscovery();
    }

    public void registerReceiver(BluetoothDiscoveryCallback<BtDevice> callback) {
        final IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_STARTED);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
        filter.addAction(BluetoothDevice.ACTION_FOUND);
        context.registerReceiver(this, filter);
        this.bluetoothDiscoveryCallback = callback;
    }

    public void unregisterReceiver() {
        try {
            context.unregisterReceiver(this);
            this.bluetoothDiscoveryCallback = null;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (action == null)
            return;

        switch(action) {
            case BluetoothAdapter.ACTION_DISCOVERY_STARTED:
                if (bluetoothDiscoveryCallback != null)
                    bluetoothDiscoveryCallback.onStart();
                break;
            case BluetoothAdapter.ACTION_DISCOVERY_FINISHED:
                if (bluetoothDiscoveryCallback != null)
                    bluetoothDiscoveryCallback.onStop();
                break;
            case BluetoothDevice.ACTION_FOUND:
                BluetoothDevice bluetoothDevice = intent.getParcelableExtra(android.bluetooth.BluetoothDevice.EXTRA_DEVICE);
                int rssi = intent.getShortExtra(android.bluetooth.BluetoothDevice.EXTRA_RSSI, Short.MIN_VALUE);
                BtDevice btDevice = new BtDevice(bluetoothDevice.getAddress(), bluetoothDevice.getName());
                btDevice.setRssi(rssi);
                if (bluetoothDiscoveryCallback != null)
                    bluetoothDiscoveryCallback.onDiscoveryResult(btDevice);
                break;
            default:
                break;
        }
    }
}
