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

package com.openvela.bluetoothtest.bredr;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
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

import com.openvela.bluetooth.BluetoothBondStateObserver;
import com.openvela.bluetooth.BluetoothStateObserver;
import com.openvela.bluetooth.callback.BluetoothBondStateCallback;
import com.openvela.bluetooth.callback.BluetoothStateCallback;
import com.openvela.bluetoothtest.MainActivity;
import com.openvela.bluetoothtest.R;

import java.lang.reflect.Method;
import java.util.Set;

public class BondActivity extends AppCompatActivity {
    private final String TAG = BondActivity.class.getSimpleName();
    EditText textBdAddr;
    EditText textNumOfCycles;
    EditText textPairedDevices;
    EditText textResultDisplay;
    private BluetoothBondStateObserver btBondStateObserver;
    private BluetoothAdapter bluetoothAdapter;
    private int timesOfCycles;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_bond);
        listenBluetoothBondState();

        BluetoothManager bluetoothManager = getSystemService(BluetoothManager.class);
        bluetoothAdapter = bluetoothManager.getAdapter();
        if (bluetoothAdapter == null) {
            Log.e(TAG, "onClick: Device doesn't support Bluetooth");
            return;
        }

        textBdAddr = findViewById(R.id.textBdAddr);
        textNumOfCycles = findViewById(R.id.textNumOfCycles);
        textPairedDevices = findViewById(R.id.textPairedDevices);
        textResultDisplay = findViewById(R.id.textResultDisplay);

        Button buttonEnable = findViewById(R.id.button_create_bond);
        buttonEnable.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String str = textNumOfCycles.getText().toString();
                if (str.isEmpty())
                    timesOfCycles = 0;
                else
                    timesOfCycles = Integer.parseInt(str);

                Log.d(TAG, "onClick: Create Bond, timesOfCycles = " + timesOfCycles);
                createBond();
            }
        });

        Button buttonDisable = findViewById(R.id.button_remove_bond);
        buttonDisable.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String str = textNumOfCycles.getText().toString();
                if (str.isEmpty())
                    timesOfCycles = 0;
                else
                    timesOfCycles = Integer.parseInt(str);

                Log.d(TAG, "onClick: Remove Bond, timesOfCycles = " + timesOfCycles);
                removeBond();
            }
        });
    }

    @Override
    protected void onStart() {
        super.onStart();

        showBondedDevices();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        btBondStateObserver.unregisterReceiver();
    }

    private void listenBluetoothBondState() {
        btBondStateObserver = new BluetoothBondStateObserver(this);
        btBondStateObserver.registerReceiver(new BluetoothBondStateCallback() {
            @Override
            public void onBonded(BluetoothDevice device) {
                String str = textResultDisplay.getText().toString();
                String bdAddr = device.getAddress();
                str = "\r\n" + bdAddr + " was bonded, timesOfCycles = " + timesOfCycles + str;
                textResultDisplay.setText(str);
                Log.i(TAG, str);

                showBondedDevices();

                // Disable Bluetooth again
                if (timesOfCycles > 0)
                    removeBond();
            }

            @Override
            public void onBondRemoved(BluetoothDevice device) {
                String str = textResultDisplay.getText().toString();
                String bdAddr = device.getAddress();
                str = "\r\n" + bdAddr + " was removed, timesOfCycles = " + timesOfCycles + str;
                textResultDisplay.setText(str);
                Log.i(TAG, str);

                showBondedDevices();

                // Enable Bluetooth again
                if (timesOfCycles > 0)
                    createBond();

                timesOfCycles--;
            }
        });
    }

    private boolean isBluetoothEnabled() {
        BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        return bluetoothAdapter != null && bluetoothAdapter.isEnabled();
    }

    private void createBond() {
        String addr = textBdAddr.getText().toString();

        try {
            BluetoothDevice btDevice = bluetoothAdapter.getRemoteDevice(addr);
            btDevice.createBond();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void removeBond() {
        String addr = textBdAddr.getText().toString();
        BluetoothDevice btDevice = bluetoothAdapter.getRemoteDevice(addr);
        //btDevice.removeBond();

        try {
            Method method = btDevice.getClass().getMethod("removeBond", (Class[]) null);
            method.invoke(btDevice, (Object[]) null);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void showBondedDevices() {
        BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter == null)
            return;

        Set<BluetoothDevice> pairedDevices = bluetoothAdapter.getBondedDevices();

        String str = "Paired devices:\r\n";
        if (pairedDevices.size() > 0) {
            // There are paired devices. Get the name and address of each paired device.
            for (BluetoothDevice device : pairedDevices) {
                String deviceName = device.getName();
                String deviceHardwareAddress = device.getAddress(); // MAC address

                str += deviceHardwareAddress + " (" + deviceName + ") \r\n";
            }
        }

        textPairedDevices.setText(str);
    }
}
