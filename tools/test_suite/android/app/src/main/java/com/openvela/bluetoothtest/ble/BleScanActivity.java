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

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.openvela.bluetooth.BtDevice;
import com.openvela.bluetooth.callback.BluetoothDiscoveryCallback;
import com.openvela.bluetoothtest.R;

public class BleScanActivity extends AppCompatActivity {
    private final String TAG = BleScanActivity.class.getSimpleName();
    private static final long BLE_SCAN_PERIOD_MS = 12 * 1000;
    private TextView tvScanState;
    private Button btnScan;
    private EditText etFilter;
    private BleScanAdapter bleScanAdapter;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ble_scan);
        bleScanAdapter = new BleScanAdapter(this);
        initView();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (bleScanAdapter.isScanning()) {
            bleScanAdapter.stopScan();
        }
    }

    private void initView() {
        tvScanState = findViewById(R.id.tv_scan_state);
        btnScan = findViewById(R.id.btn_scan);
        etFilter = findViewById(R.id.et_filter);
        RecyclerView recyclerView = findViewById(R.id.recyclerView);

        btnScan.setOnClickListener(v -> {
            if (bleScanAdapter.isScanning()) {
                bleScanAdapter.stopScan();
            } else {
                bleScanAdapter.startScan(new String[]{etFilter.getText().toString()}, BLE_SCAN_PERIOD_MS, discoveryCallback);
            }
        });

        recyclerView.setLayoutManager(new LinearLayoutManager(this, LinearLayoutManager.VERTICAL, false));
        recyclerView.addItemDecoration(new DividerItemDecoration(this, DividerItemDecoration.VERTICAL));
        recyclerView.getItemAnimator().setChangeDuration(300);
        recyclerView.getItemAnimator().setMoveDuration(300);
        recyclerView.setAdapter(bleScanAdapter);
    }

    private final BluetoothDiscoveryCallback<BtDevice> discoveryCallback = new BluetoothDiscoveryCallback<BtDevice>() {
        @Override
        public void onDiscoveryResult(final BtDevice device) {}

        @Override
        public void onStart() {
            super.onStart();
            tvScanState.setText("Scanning...");
            btnScan.setText("STOP SCAN");
        }

        @Override
        public void onStop() {
            super.onStop();
            tvScanState.setText("Scan Stopped");
            btnScan.setText("START SCAN");
        }

        @Override
        public void onDiscoveryFailed(int errorCode) {
            super.onDiscoveryFailed(errorCode);
            tvScanState.setText("Scan Failed: " + errorCode);
            Log.e(TAG, "onDiscoveryFailed: " + errorCode);
        }
    };

}
