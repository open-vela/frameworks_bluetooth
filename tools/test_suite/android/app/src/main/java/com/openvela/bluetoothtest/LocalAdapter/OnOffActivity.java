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

package com.openvela.bluetoothtest.LocalAdapter;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import com.openvela.bluetooth.BluetoothStateObserver;
import com.openvela.bluetooth.callback.BluetoothStateCallback;
import com.openvela.bluetoothtest.MainActivity;
import com.openvela.bluetoothtest.R;

public class OnOffActivity extends AppCompatActivity {
    private final String TAG = MainActivity.class.getSimpleName();
    private final int REQUEST_ENABLE_BT = 1;
    EditText textNumOfCycles;
    EditText textResultDisplay;
    private BluetoothStateObserver btStateObserver;
    private BluetoothAdapter bluetoothAdapter;
    private int timesOfCycles;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_on_off);
        listenBluetoothState();

        BluetoothManager bluetoothManager = getSystemService(BluetoothManager.class);
        bluetoothAdapter = bluetoothManager.getAdapter();
        if (bluetoothAdapter == null) {
            Log.e(TAG, "onClick: Device doesn't support Bluetooth");
            return;
        }

        textNumOfCycles = findViewById(R.id.textNumOfCycles);
        textResultDisplay = findViewById(R.id.textResultDisplay);

        Button buttonEnable = findViewById(R.id.button_enable_bluetooth);
        buttonEnable.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String str = textNumOfCycles.getText().toString();
                if (str.isEmpty())
                    timesOfCycles = 0;
                else
                    timesOfCycles = Integer.parseInt(str);

                Log.d(TAG, "onClick: Enable Bluetooth, timesOfCycles = " + timesOfCycles);
                enableBluetooth();
            }
        });

        Button buttonDisable = findViewById(R.id.button_disable_bluetooth);
        buttonDisable.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String str = textNumOfCycles.getText().toString();
                if (str.isEmpty())
                    timesOfCycles = 0;
                else
                    timesOfCycles = Integer.parseInt(str);

                Log.d(TAG, "onClick: Disable Bluetooth, timesOfCycles = " + timesOfCycles);
                new AsyncTask<Void, Void, Void>() {
                    @Override
                    protected Void doInBackground(Void... params) {
                        // Time consuming operation
                        disableBluetooth();
                        return null;
                    }
                    @Override
                    protected void onPostExecute(Void result) {
                        // Update UI
                    }
                }.execute();

            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        btStateObserver.unregisterReceiver();
    }

    private void listenBluetoothState() {
        btStateObserver = new BluetoothStateObserver(this);
        btStateObserver.registerReceiver(new BluetoothStateCallback() {
            @Override
            public void onEnabled() {
                String str = textResultDisplay.getText().toString();
                str = "\r\nBluetoothAdapter is enabled, timesOfCycles = " + timesOfCycles +str;
                textResultDisplay.setText(str);
                Log.i(TAG, str);

                // Disable Bluetooth again
                if (timesOfCycles > 0)
                    disableBluetooth();
            }

            @Override
            public void onDisabled() {
                String str = textResultDisplay.getText().toString();
                str = "\r\nBluetoothAdapter is disabled, timesOfCycles = " + timesOfCycles + str;
                textResultDisplay.setText(str);
                Log.i(TAG, str);

                // Enable Bluetooth again
                if (timesOfCycles > 0)
                    enableBluetooth();

                timesOfCycles--;
            }
        });
    }

    private boolean isBluetoothEnabled() {
        BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        return bluetoothAdapter != null && bluetoothAdapter.isEnabled();
    }

    private void enableBluetooth() {
        startActivityForResult(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), REQUEST_ENABLE_BT);
    }

    private void disableBluetooth() {
        bluetoothAdapter.disable();
    }
}
