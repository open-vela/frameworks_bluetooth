/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
import android.util.Log;

import com.openvela.bluetooth.callback.BluetoothBondStateCallback;
import com.openvela.bluetooth.callback.BluetoothStateCallback;

public class BluetoothBondStateObserver extends BroadcastReceiver {
    private final String TAG = "BluetoothBondStateObserver";
    private static final boolean DBG = false;
    private final Context context;
    private BluetoothBondStateCallback bluetoothBondStateCallback;

    public BluetoothBondStateObserver(Context context){
        this.context = context;
    }

    public void registerReceiver(BluetoothBondStateCallback callback) {
        final IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        context.registerReceiver(this, filter);
        if (DBG)
            Log.d(TAG, "registerReceiver: ACTION_BOND_STATE_CHANGED");
        this.bluetoothBondStateCallback = callback;
    }

    public void unregisterReceiver() {
        try {
            context.unregisterReceiver(this);
            if (DBG)
                Log.d(TAG, "unregisterReceiver: ACTION_BOND_STATE_CHANGED");
            this.bluetoothBondStateCallback = null;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (action == null)
            return;

        if (action.equals(BluetoothDevice.ACTION_BOND_STATE_CHANGED)) {
            int bondState = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR);
            BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            if (DBG)
                Log.d(TAG, "onReceive Bond State = " + bondState);

            if (bondState == BluetoothDevice.BOND_BONDED) {
                if (bluetoothBondStateCallback != null) {
                    bluetoothBondStateCallback.onBonded(device);
                }
            } else if (bondState == BluetoothDevice.BOND_NONE) {
                if (bluetoothBondStateCallback != null) {
                    bluetoothBondStateCallback.onBondRemoved(device);
                }
            }
        }
    }
}
