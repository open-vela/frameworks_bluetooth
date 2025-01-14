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
import android.widget.Button;
import android.widget.TextView;

import android.bluetooth.BluetoothProfile;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.openvela.bluetooth.BtDevice;
import com.openvela.bluetooth.callback.BleConnectCallback;
import com.openvela.bluetoothtest.R;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

public class BleCentralActivity extends AppCompatActivity {
    private final String TAG = BleCentralActivity.class.getSimpleName();
    public static final String EXTRA_TAG = "device";
    private TextView tvConnectState;
    private Button btnConnect;
    private @NotNull BtDevice currentDevice;
    private GattClientAdapter gattClientAdapter;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ble_central);
        gattClientAdapter = new GattClientAdapter(this);
        initView();

        currentDevice = getIntent().getParcelableExtra(EXTRA_TAG);
        getSupportActionBar().setSubtitle(currentDevice.getAddress());
        gattClientAdapter.connect(currentDevice, connectCallback);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (currentDevice.getConnectionState() != BluetoothProfile.STATE_CONNECTING) {
            gattClientAdapter.cancelConnect(currentDevice);
        } else if (currentDevice.getConnectionState() != BluetoothProfile.STATE_CONNECTED){
            gattClientAdapter.disconnect(currentDevice);
        }
    }

    private void initView() {
        tvConnectState = findViewById(R.id.tv_connect_state);
        btnConnect = findViewById(R.id.btn_connect);
        RecyclerView recyclerView = findViewById(R.id.recyclerView);

        btnConnect.setOnClickListener(v -> {
            if (currentDevice.getConnectionState() == BluetoothProfile.STATE_DISCONNECTED) {
                gattClientAdapter.connect(currentDevice, connectCallback);
            } else if (currentDevice.getConnectionState() == BluetoothProfile.STATE_CONNECTING) {
                gattClientAdapter.cancelConnect(currentDevice);
            } else {
                gattClientAdapter.disconnect(currentDevice);
            }
        });

        recyclerView.setLayoutManager(new LinearLayoutManager(this, LinearLayoutManager.VERTICAL, false));
        recyclerView.addItemDecoration(new DividerItemDecoration(this,DividerItemDecoration.VERTICAL));
        recyclerView.getItemAnimator().setChangeDuration(300);
        recyclerView.getItemAnimator().setMoveDuration(300);
        recyclerView.setAdapter(gattClientAdapter);
    }

    private final BleConnectCallback<BtDevice> connectCallback = new BleConnectCallback<BtDevice>() {
        @Override
        public void onConnectionChanged(String address, int newState) {
            if (!address.equals(currentDevice.getAddress())) {
                return;
            }
            currentDevice.setConnectionState(newState);
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                tvConnectState.setText("Connected");
                btnConnect.setText("DISCONNECT");
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED){
                tvConnectState.setText("Disconnected");
                btnConnect.setText("CONNECT");
            } else if (newState == BluetoothProfile.STATE_CONNECTING) {
                tvConnectState.setText("Connecting...");
                btnConnect.setText("DISCONNECT");
            } else if (newState == BluetoothProfile.STATE_DISCONNECTING){
                tvConnectState.setText("Disconnecting...");
                btnConnect.setText("DISCONNECT");
            }
        }

        @Override
        public void onConnectFailed(String address, int errorCode) {
            super.onConnectFailed(address, errorCode);
            if (errorCode == BleConnectCallback.FAILED_DEVICE_NOT_FOUND) {
                tvConnectState.setText("Connect Failed:" + "device not found");
            } else if (errorCode == BleConnectCallback.FAILED_TIMEOUT) {
                tvConnectState.setText("Connect Failed:" + "timeout");
            } else {
                tvConnectState.setText("Connect Failed:" + errorCode);
            }
            currentDevice.setConnectionState(BluetoothProfile.STATE_DISCONNECTED);
            btnConnect.setText("CONNECT");
        }

        @Override
        public void onServicesDiscovered(String address) {
            super.onServicesDiscovered(address);
            tvConnectState.setText("Discovered");
        }
    };

}
