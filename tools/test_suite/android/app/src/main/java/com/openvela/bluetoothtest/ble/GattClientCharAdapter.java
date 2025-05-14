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

import java.util.List;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;

import com.openvela.bluetooth.adapter.RecyclerAdapter;
import com.openvela.bluetooth.adapter.RecyclerViewHolder;
import com.openvela.bluetoothtest.R;

public class GattClientCharAdapter extends RecyclerAdapter<BluetoothGattCharacteristic> {
    private final static String TAG = GattClientCharAdapter.class.getSimpleName();
    private static final String BASE_UUID_REGEX = "0000([0-9a-f][0-9a-f][0-9a-f][0-9a-f])-0000-1000-8000-00805f9b34fb";
    private final Handler handler = new Handler(Looper.myLooper());
    private final BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

    public GattClientCharAdapter(Context context, List<BluetoothGattCharacteristic> data) {
        super(context, R.layout.item_gatt_element, data);
    }

    @Override
    public void onBindViewHolderItem(RecyclerViewHolder viewHolder, BluetoothGattCharacteristic gattChar) {
        // Char UUID
        String charUuid = gattChar.getUuid().toString();
        StringBuilder builder = new StringBuilder();
        builder.append("UUID: 0x");
        if (charUuid.toLowerCase().matches(BASE_UUID_REGEX)) {
            builder.append(charUuid.substring(4, 8).toUpperCase());
        } else {
            builder.append(charUuid);
        }
        viewHolder.setText(R.id.tv_char_uuid, builder.toString());

        // Char Properties
        int charProp = gattChar.getProperties();
        builder.setLength(0);
        if ((charProp & BluetoothGattCharacteristic.PROPERTY_READ) != 0) {
            builder.append("READ,");
        }
        if ((charProp & BluetoothGattCharacteristic.PROPERTY_WRITE) != 0) {
            builder.append("WRITE,");
        }
        if ((charProp & BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE) != 0) {
            builder.append("WRITE_NO_RESPONSE,");
        }
        if ((charProp & BluetoothGattCharacteristic.PROPERTY_NOTIFY) != 0) {
            builder.append("NOTIFY,");
        }
        if ((charProp & BluetoothGattCharacteristic.PROPERTY_INDICATE) != 0) {
            builder.append("INDICATE,");
        }
        viewHolder.setText(R.id.tv_char_prop, String.format("Properties: %s", builder.toString()));

        if ((charProp & BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE) != 0) {
            viewHolder.setVisibility(R.id.tv_write_tput, View.VISIBLE);
            viewHolder.setOnClickListener(R.id.tv_write_tput, v -> {
                Log.i(TAG, "Write Tput start!");
            });
        }
    }
}
