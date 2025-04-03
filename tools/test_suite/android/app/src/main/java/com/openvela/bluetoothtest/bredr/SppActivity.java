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

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.openvela.bluetooth.BtSock;
import com.openvela.bluetoothtest.MainActivity;
import com.openvela.bluetoothtest.R;

public class SppActivity extends AppCompatActivity {
    private final String TAG = SppActivity.class.getSimpleName();

    // Client/Server #1
    EditText textServiceUUID_1;
    EditText textBdAddr_1;
    EditText textCycles_1;
    EditText textDataToSend_1;
    BtSock btSpp_1;

    // Client/Server #2
    EditText textServiceUUID_2;
    EditText textBdAddr_2;
    EditText textCycles_2;
    EditText textDataToSend_2;
    BtSock btSpp_2;

    // Data to Display
    EditText textDataToDisplay;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_spp);

        // Service UUID #1
        textServiceUUID_1 = (EditText) findViewById(R.id.text_service_uuid_1);

        // Register Server #1
        Button buttonRegister_1 = findViewById(R.id.button_spp_server_register_1);
        buttonRegister_1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String uuid = textServiceUUID_1.getText().toString();

                Log.d(TAG, "onClick: Register UUID#1 = " + uuid);
                btSpp_1.register(uuid);
            }
        });

        // Unregister Server #1
        Button buttonUnregister_1 = findViewById(R.id.button_spp_server_unregister_1);
        buttonUnregister_1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.d(TAG, "onClick: Unregister Server#1");
                btSpp_1.unregister();
            }
        });

        // Service UUID #2
        textServiceUUID_2 = (EditText) findViewById(R.id.text_service_uuid_2);

        // Register Server #2
        Button buttonRegister_2 = findViewById(R.id.button_spp_server_register_2);
        buttonRegister_2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String uuid = textServiceUUID_2.getText().toString();

                Log.d(TAG, "onClick: Register UUID#2 = " + uuid);
                btSpp_2.register(uuid);
            }
        });

        // Unregister Server #2
        Button buttonUnregister_2 = findViewById(R.id.button_spp_server_unregister_2);
        buttonUnregister_2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.d(TAG, "onClick: Unregister Server#2");
                btSpp_2.unregister();
            }
        });

        // BD_ADDR #1
        textBdAddr_1 = (EditText) findViewById(R.id.text_bd_addr_1);

        // Connect by Client #1
        Button buttonConnect_1 = findViewById(R.id.button_spp_client_connect_1);
        buttonConnect_1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String uuid = textServiceUUID_1.getText().toString();
                String addr = textBdAddr_1.getText().toString();

                Log.d(TAG, "onClick: Connect by Client#1, to BD_ADDR =  " + addr);
                btSpp_1.connect(addr, uuid);
            }
        });

        // Disconnect with Client/Server #1
        Button buttonDisonnect_1 = findViewById(R.id.button_spp_disconnect_1);
        buttonDisonnect_1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.d(TAG, "onClick: Disconnect with Client/Server#1");
                btSpp_1.disconnect();
            }
        });

        // BD_ADDR #2
        textBdAddr_2 = (EditText) findViewById(R.id.text_bd_addr_2);

        // Connect by Client #2
        Button buttonConnect_2 = findViewById(R.id.button_spp_client_connect_2);
        buttonConnect_2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String uuid = textServiceUUID_2.getText().toString();
                String addr = textBdAddr_2.getText().toString();

                Log.d(TAG, "onClick: Connect by Client#2, to BD_ADDR =  " + addr);
                btSpp_2.connect(addr, uuid);
            }
        });

        // Disconnect with Client/Server #2
        Button buttonDisonnect_2 = findViewById(R.id.button_spp_disconnect_2);
        buttonDisonnect_2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.d(TAG, "onClick: Disconnect with Client/Server#2");
                btSpp_2.disconnect();
            }
        });

        // DataToSend #1
        textDataToSend_1 = (EditText) findViewById(R.id.text_data_to_send_1);

        // Cycles #1
        textCycles_1 = (EditText) findViewById(R.id.text_cycles_1);

        // Send to Client/Server #1
        Button buttonSend_1 = findViewById(R.id.button_spp_send_1);
        buttonSend_1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String str = textDataToSend_1.getText().toString();
                String cycles = textCycles_1.getText().toString();
                int numOfCycles = 1;

                if (!cycles.isEmpty())
                    numOfCycles = Integer.parseInt(cycles);

                Log.d(TAG, "onClick: Send by Client/Server#1, data =  " + str);
                btSpp_1.send(str, numOfCycles);
            }
        });

        // DataToSend #2
        textDataToSend_2 = (EditText) findViewById(R.id.text_data_to_send_2);

        // Cycles #1
        textCycles_2 = (EditText) findViewById(R.id.text_cycles_1);

        // Send to Client/Server #2
        Button buttonSend_2 = findViewById(R.id.button_spp_send_2);
        buttonSend_2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String str = textDataToSend_2.getText().toString();
                String cycles = textCycles_2.getText().toString();
                int numOfCycles = 1;

                if (!cycles.isEmpty())
                    numOfCycles = Integer.parseInt(cycles);

                Log.d(TAG, "onClick: Send by Client/Server#2, data =  " + str);
                btSpp_2.send(str, numOfCycles);
            }
        });

        // Data to display
        textDataToDisplay = (EditText) findViewById(R.id.text_data_to_display);

        // Init BtSock
        btSpp_1 = new BtSock(1, BtSock.SOCK_TYPE_SPP_INSECURE, mHandler);
        btSpp_2 = new BtSock(2, BtSock.SOCK_TYPE_SPP_INSECURE, mHandler);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    // Handle Message from BtSock
    private final Handler mHandler = new Handler(Looper.getMainLooper()) {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case BtSock.MESSAGE_SOCK_LOGGING:
                    Bundle bData = msg.getData();
                    String str = (String) bData.get("log");
                    if (str == null)
                        return;

                    Log.d(TAG, str);

                    // Update UI
                    String strToDisplay = textDataToDisplay.getText().toString();
                    textDataToDisplay.setText(str + strToDisplay);
                    break;
            }
        }
    };
}
