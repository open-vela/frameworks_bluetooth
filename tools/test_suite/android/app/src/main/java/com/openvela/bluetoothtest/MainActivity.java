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
import androidx.core.app.ActivityCompat;

import com.openvela.bluetooth.BluetoothStateObserver;
import com.openvela.bluetooth.callback.BluetoothStateCallback;
import com.openvela.bluetoothtest.LocalAdapter.OnOffActivity;
import com.openvela.bluetoothtest.ble.BleL2capActivity;
import com.openvela.bluetoothtest.ble.BleScanActivity;
import com.openvela.bluetoothtest.ble.BlePeripheralActivity;
import com.openvela.bluetoothtest.bredr.BondActivity;
import com.openvela.bluetoothtest.bredr.BredrInquiryActivity;
import com.openvela.bluetoothtest.bredr.BredrL2capActivity;
import com.openvela.bluetoothtest.bredr.SppActivity;

public class MainActivity extends AppCompatActivity {
    private final String TAG = MainActivity.class.getSimpleName();

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        requestBluetoothPermission();
    }

    private void requestBluetoothPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            String[] necessaryBluetoothPermissioins = {
                    Manifest.permission.BLUETOOTH_CONNECT,
                    Manifest.permission.BLUETOOTH_SCAN,
                    Manifest.permission.BLUETOOTH_ADVERTISE,
                    Manifest.permission.ACCESS_COARSE_LOCATION,
                    Manifest.permission.ACCESS_FINE_LOCATION};
            if (necessaryBluetoothPermissioins.length > 0) {
                Log.d(TAG, "Request Bluetooth permissions");
                ActivityCompat.requestPermissions(this, necessaryBluetoothPermissioins, 1);
            }
        }
    }

    public void entryOnOffActivity(View view) {
        startActivity(new Intent(this, OnOffActivity.class));
    }

    public void entryBredrInquiryActivity(View view) {
        startActivity(new Intent(this, BredrInquiryActivity.class));
    }

    public void entryBondActivity(View view) {
        startActivity(new Intent(this, BondActivity.class));
    }

    public void entrySppActivity(View view) {
        startActivity(new Intent(this, SppActivity.class));
    }

    public void entryBredrL2capActivity(View view) {
        startActivity(new Intent(this, BredrL2capActivity.class));
    }

    public void entryBleCentralActivity(View view) {
        startActivity(new Intent(this, BleScanActivity.class));
    }

    public void entryBlePeripheralActivity(View view) {
        startActivity(new Intent(this, BlePeripheralActivity.class));
    }

    public void entryBleL2capActivity(View view) {
        startActivity(new Intent(this, BleL2capActivity.class));
    }
}
