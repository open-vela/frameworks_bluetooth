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

import android.os.Bundle;
import android.util.Log;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Button;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.BluetoothLeAdvertiser;

import androidx.appcompat.app.AppCompatActivity;

import com.openvela.bluetoothtest.R;

public class BlePeripheralActivity extends AppCompatActivity {
    private final String TAG = BlePeripheralActivity.class.getSimpleName();
    private final BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    private volatile BluetoothLeAdvertiser bluetoothAdvertiser;
    private TextView tvAdvState;
    private Button btnAdv;
    private EditText etAdvName;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ble_peripheral);
        initView();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (isAdvertising()) {
            stopAdvertising();
        }
    }

    private void initView() {
        tvAdvState = findViewById(R.id.tv_adv_state);
        btnAdv = findViewById(R.id.btn_adv);
        etAdvName = findViewById(R.id.et_adv_name);

        btnAdv.setOnClickListener(v -> {
           if (isAdvertising()) {
               stopAdvertising();
               tvAdvState.setText("Advertise Stopped");
               btnAdv.setText("START ADVERTISE");
           } else {
               startAdvertising(etAdvName.getText().toString().getBytes());
           }
        });
    }

    private boolean isAdvertising() {
        return (bluetoothAdvertiser != null);
    }

    private void startAdvertising(final byte[] payload) {
        AdvertiseSettings advertiseSettings = new AdvertiseSettings.Builder()
            .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY)
            .setConnectable(true)
            .setTimeout(0)
            .setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_HIGH)
            .build();
        AdvertiseData advertiseData = new AdvertiseData.Builder()
            .addManufacturerData(0xFF00, payload)
            .setIncludeDeviceName(true)
            .build();

        bluetoothAdvertiser = bluetoothAdapter.getBluetoothLeAdvertiser();
        bluetoothAdvertiser.startAdvertising(advertiseSettings, advertiseData, advertiseCallback);
    }

    public void stopAdvertising() {
        bluetoothAdvertiser.stopAdvertising(advertiseCallback);
        bluetoothAdvertiser = null;
    }

    private final AdvertiseCallback advertiseCallback = new AdvertiseCallback() {
        @Override
        public void onStartSuccess(AdvertiseSettings settingsInEffect) {
            tvAdvState.setText("Advertising...");
            btnAdv.setText("STOP ADVERTISE");
        }

        @Override
        public void onStartFailure(int errorCode) {
            tvAdvState.setText("Advertise Failed: " + errorCode);
            Log.e(TAG, "onAdvStartFailure: " + errorCode);
        }
    };
}