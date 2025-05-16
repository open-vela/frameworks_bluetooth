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
import android.widget.Toast;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.app.Activity;

public class GattClientCharAdapter extends RecyclerAdapter<BluetoothGattCharacteristic> {
    private final static String TAG = GattClientCharAdapter.class.getSimpleName();
    private static final String BASE_UUID_REGEX = "0000([0-9a-f][0-9a-f][0-9a-f][0-9a-f])-0000-1000-8000-00805f9b34fb";
    private final Handler handler = new Handler(Looper.myLooper());
    private final BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    private final BluetoothGatt bluetoothGatt;
    private final Context context;
    public GattClientCharAdapter(Context context, List<BluetoothGattCharacteristic> data, BluetoothGatt bluetoothGatt) {
        super(context, R.layout.item_gatt_element, data);
        this.context = context;
        this.bluetoothGatt = bluetoothGatt;
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

        // If WRITE_NO_RESPONSE is supported, display the test button
        if ((charProp & BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE) != 0) {
            viewHolder.setVisibility(R.id.tv_write_tput, View.VISIBLE);
            viewHolder.setOnClickListener(R.id.tv_write_tput, v -> {
                EditText etPayloadSize = viewHolder.getView(R.id.et_payload_size);
                EditText etTotalCount = viewHolder.getView(R.id.et_total_count);

                int payloadSize = 239;
                int totalCount = 1000;

                try {
                    payloadSize = Integer.parseInt(etPayloadSize.getText().toString().trim());
                } catch (NumberFormatException ignored) {}

                try {
                    totalCount = Integer.parseInt(etTotalCount.getText().toString().trim());
                } catch (NumberFormatException ignored) {}

                startWriteThroughputTest(gattChar, payloadSize, totalCount);
            });
        } else {
            viewHolder.setVisibility(R.id.tv_write_tput, View.GONE);
        }
    }

    private void startWriteThroughputTest(BluetoothGattCharacteristic gattChar, int payloadSize, int totalCount) {
        if (bluetoothGatt == null || gattChar == null) {
            Log.e(TAG, "BluetoothGatt or Characteristic is null");
            return;
        }

        byte[] payload = new byte[payloadSize];
        for (int i = 0; i < payload.length; i++) {
            payload[i] = (byte) i;
        }

        Log.i(TAG, "Starting write throughput test");
        // The ProgressBar needs to be added to the layout file.
        ProgressBar progressBar = ((Activity) context).findViewById(R.id.progressBar);
        progressBar.setProgress(0);
        progressBar.setVisibility(View.VISIBLE);
    
        new Thread(() -> {
            int total = totalCount;
            for (int i = 0; i < total; i++) {
                gattChar.setValue(payload);
                boolean success = bluetoothGatt.writeCharacteristic(gattChar);
                Log.i(TAG, "Write #" + i + ": " + success);
                int progress = i + 1;
                // Update progress bar, must cut back to main thread
                new Handler(Looper.getMainLooper()).post(() -> {
                    progressBar.setProgress(progress * 100 / total);
                });
                // Simple delays that can be replaced with more elegant write-completion back-regulation flow logic
                try {
                    Thread.sleep(1);
                } catch (InterruptedException e) {
                    Log.e(TAG, "Sleep interrupted", e);
                    break;
                }
            }

            // Show Toast when sending is complete (return to main thread)
            new Handler(Looper.getMainLooper()).post(() ->
                Toast.makeText(context, "Send finish", Toast.LENGTH_SHORT).show()
            );

            // Send complete, main thread notifies UI
            new Handler(Looper.getMainLooper()).post(() -> {
                progressBar.setProgress(100); // Ensure 100% display
                Toast.makeText(context, "Send finish", Toast.LENGTH_SHORT).show();
            });
        }).start();
    }
}
