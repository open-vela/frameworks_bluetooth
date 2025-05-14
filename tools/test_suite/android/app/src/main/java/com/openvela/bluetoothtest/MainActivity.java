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

package com.openvela.bluetoothtest;

import java.util.ArrayList;
import java.util.List;

import android.Manifest;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import android.bluetooth.BluetoothAdapter;

import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import com.openvela.bluetooth.BluetoothStateObserver;
import com.openvela.bluetooth.callback.BluetoothStateCallback;
import com.openvela.bluetoothtest.ble.BleScanActivity;
import com.openvela.bluetoothtest.ble.BlePeripheralActivity;
import com.openvela.bluetoothtest.bredr.BredrInquiryActivity;

public class MainActivity extends AppCompatActivity {
    private final String TAG = MainActivity.class.getSimpleName();
    private final int REQUEST_ENABLE_BT = 1;

    private LinearLayout llBluetoothAdapterTip;
    private BluetoothStateObserver btStateObserver;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        initView();
        requestBluetoothPermission();
        listenBluetoothState();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        btStateObserver.unregisterReceiver();
    }

    private void initView() {
        llBluetoothAdapterTip = findViewById(R.id.ll_adapter_tip);
        TextView tvAdapterStates = findViewById(R.id.tv_adapter_states);

        tvAdapterStates.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivityForResult(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), REQUEST_ENABLE_BT);
            }
        });
    }

    @RequiresApi(api = Build.VERSION_CODES.S)
    private void requestBluetoothPermission() {
        List<String> permissions = new ArrayList<>();
        permissions.add(Manifest.permission.BLUETOOTH_SCAN);
        permissions.add(Manifest.permission.BLUETOOTH_ADVERTISE);
        permissions.add(Manifest.permission.BLUETOOTH_CONNECT);
        permissions.add(Manifest.permission.ACCESS_COARSE_LOCATION);
        permissions.add(Manifest.permission.ACCESS_FINE_LOCATION);

        registerForActivityResult(new ActivityResultContracts.RequestMultiplePermissions(), map -> {
            if (!isBluetoothEnabled()) {
                llBluetoothAdapterTip.setVisibility(View.VISIBLE);
            }
        }).launch(permissions.toArray(new String[0]));
    }

    private void listenBluetoothState() {
        btStateObserver = new BluetoothStateObserver(this);
        btStateObserver.registerReceiver(new BluetoothStateCallback() {
            @Override
            public void onEnabled() {
                Log.i(TAG, "BluetoothAdapter is enabled!");
                llBluetoothAdapterTip.setVisibility(View.GONE);
            }

            @Override
            public void onDisabled() {
                Log.i(TAG, "BluetoothAdapter is disabled!");
                llBluetoothAdapterTip.setVisibility(View.VISIBLE);
            }
        });
    }

    private boolean isBluetoothEnabled() {
        BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        return bluetoothAdapter != null && bluetoothAdapter.isEnabled();
    }

    public void entryBredrInquiryActivity(View view) {
        startActivity(new Intent(this, BredrInquiryActivity.class));
    }

    public void entryBleCentralActivity(View view) {
        startActivity(new Intent(this, BleScanActivity.class));
    }

    public void entryBlePeripheralActivity(View view) {
        startActivity(new Intent(this, BlePeripheralActivity.class));
    }
}
