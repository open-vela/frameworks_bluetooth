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

package com.openvela.bluetoothtest.bredr;

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

public class BredrInquiryActivity extends AppCompatActivity {
    private final String TAG = BredrInquiryActivity.class.getSimpleName();
    private static final long BREDR_INQUIRY_PERIOD_MS = 12 * 1000;
    private TextView tvInquiryState;
    private Button btnInquiry;
    private EditText etFilter;
    private BredrInquiryAdapter bredrInquiryAdapter;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_bredr_inquiry);
        bredrInquiryAdapter = new BredrInquiryAdapter(this);
        initView();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (bredrInquiryAdapter.isDiscovering()) {
            bredrInquiryAdapter.stopDiscovery();
        }
    }

    private void initView() {
        tvInquiryState = findViewById(R.id.tv_inquiry_state);
        btnInquiry = findViewById(R.id.btn_inquiry);
        etFilter = findViewById(R.id.et_filter);
        RecyclerView recyclerView = findViewById(R.id.recyclerView);

        btnInquiry.setOnClickListener(v -> {
            if (bredrInquiryAdapter.isDiscovering()) {
                bredrInquiryAdapter.stopDiscovery();
            } else {
                bredrInquiryAdapter.startDiscovery(new String[]{etFilter.getText().toString()}, BREDR_INQUIRY_PERIOD_MS, discoveryCallback);
            }
        });

        recyclerView.setLayoutManager(new LinearLayoutManager(this, LinearLayoutManager.VERTICAL, false));
        recyclerView.addItemDecoration(new DividerItemDecoration(this, DividerItemDecoration.VERTICAL));
        recyclerView.getItemAnimator().setChangeDuration(300);
        recyclerView.getItemAnimator().setMoveDuration(300);
        recyclerView.setAdapter(bredrInquiryAdapter);
    }

    private final BluetoothDiscoveryCallback<BtDevice> discoveryCallback = new BluetoothDiscoveryCallback<BtDevice>() {
        @Override
        public void onDiscoveryResult(final BtDevice device) {}

        @Override
        public void onStart() {
            super.onStart();
            tvInquiryState.setText("Inquiring...");
            btnInquiry.setText("STOP INQUIRY");
        }

        @Override
        public void onStop() {
            super.onStop();
            tvInquiryState.setText("Inquiry Stopped");
            btnInquiry.setText("START SCAN");
        }

        @Override
        public void onDiscoveryFailed(int errorCode) {
            super.onDiscoveryFailed(errorCode);
            tvInquiryState.setText("Inquiry Failed: " + errorCode);
            Log.e(TAG, "onDiscoveryFailed: " + errorCode);
        }
    };
}
