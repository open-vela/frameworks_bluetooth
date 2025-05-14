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
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import com.openvela.bluetooth.callback.BluetoothStateCallback;

public class BluetoothStateObserver extends BroadcastReceiver {
    private final Context context;
    private BluetoothStateCallback bluetoothStateCallback;

    public BluetoothStateObserver(Context context){
        this.context = context;
    }

    public void registerReceiver(BluetoothStateCallback callback) {
        final IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        context.registerReceiver(this, filter);
        this.bluetoothStateCallback = callback;
    }

    public void unregisterReceiver() {
        try {
            context.unregisterReceiver(this);
            this.bluetoothStateCallback = null;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (action == null)
            return;

        if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
            int status = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
            if (status == BluetoothAdapter.STATE_ON) {
                if (bluetoothStateCallback != null) {
                    bluetoothStateCallback.onEnabled();
                }
            } else if (status == BluetoothAdapter.STATE_OFF) {
                if (bluetoothStateCallback != null) {
                    bluetoothStateCallback.onDisabled();
                }
            }
        }
    }
}
