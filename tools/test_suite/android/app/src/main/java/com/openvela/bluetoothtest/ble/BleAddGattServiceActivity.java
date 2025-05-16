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

package com.openvela.bluetoothtest.ble;

import android.os.Bundle;
import androidx.appcompat.widget.Toolbar;
import com.openvela.bluetoothtest.R;
import android.app.AlertDialog;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import android.widget.Button;
import android.view.View;
import android.widget.CheckBox;
import android.graphics.Color;
import android.content.SharedPreferences;
import org.json.JSONArray;
import org.json.JSONObject;

public class BleAddGattServiceActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ble_add_gatt_service);

        // Set Toolbar
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        // Enable Return Key
        if (getSupportActionBar() != null) {
            getSupportActionBar().setTitle("Add Service");
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        }

        // Reads and displays saved Services and Characteristics.
        loadSavedServices();

        // Add service button click event
        Button btnAddService = findViewById(R.id.btn_add_service);
        btnAddService.setOnClickListener(v -> {
            // Creating a dialog layout
            LinearLayout layout = new LinearLayout(this);
            layout.setOrientation(LinearLayout.VERTICAL);
            layout.setPadding(50, 40, 50, 10);

            final EditText nameInput = new EditText(this);
            nameInput.setHint("Service Name");
            layout.addView(nameInput);

            final EditText uuidInput = new EditText(this);
            uuidInput.setHint("Service UUID");
            layout.addView(uuidInput);

            // Create AlertDialog
            new AlertDialog.Builder(this)
                    .setTitle("Add GATT Service")
                    .setView(layout)
                    .setPositiveButton("OK", (dialog, which) -> {
                        String name = nameInput.getText().toString();
                        String uuid = uuidInput.getText().toString();
                        Toast.makeText(this, "Service Name: " + name + "\nUUID: " + uuid, Toast.LENGTH_SHORT).show();

                        // Save to SharedPreferences
                        saveServiceData(name, uuid);

                        // Display Service Name and UUID in the entry
                        displayAddedService(name, uuid);
                    })
                    .setNegativeButton("CANCEL", (dialog, which) -> dialog.dismiss())
                    .show();
        });
    }

    private void saveServiceData(String name, String uuid) {
        SharedPreferences sharedPreferences = getSharedPreferences("GattServiceData", MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPreferences.edit();

        // Getting saved service data
        String savedServices = sharedPreferences.getString("services", "[]");
        try {
            JSONArray serviceArray = new JSONArray(savedServices);
            JSONObject serviceObject = new JSONObject();
            serviceObject.put("serviceName", name);
            serviceObject.put("serviceUuid", uuid);
            serviceObject.put("characteristics", new JSONArray());  // Initialize to an empty array of Characteristics.

            serviceArray.put(serviceObject);

            editor.putString("services", serviceArray.toString());
            editor.apply();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // Reading and loading saved services and features
    private void loadSavedServices() {
        SharedPreferences sharedPreferences = getSharedPreferences("GattServiceData", MODE_PRIVATE);
        String savedServices = sharedPreferences.getString("services", "[]");

        try {
            JSONArray serviceArray = new JSONArray(savedServices);

            for (int i = 0; i < serviceArray.length(); i++) {
                JSONObject serviceObject = serviceArray.getJSONObject(i);
                String serviceName = serviceObject.getString("serviceName");
                String serviceUuid = serviceObject.getString("serviceUuid");

                displayAddedService(serviceName, serviceUuid);

                // Loads and displays all features of the service
                JSONArray characteristicsArray = serviceObject.getJSONArray("characteristics");
                for (int j = 0; j < characteristicsArray.length(); j++) {
                    JSONObject characteristicObject = characteristicsArray.getJSONObject(j);
                    String charName = characteristicObject.getString("charName");
                    String charUuid = characteristicObject.getString("charUuid");
                    String charValue = characteristicObject.getString("charValue");

                    displayCharacteristic(charName, charUuid, charValue);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void displayAddedService(String name, String uuid) {
        LinearLayout addedServiceList = findViewById(R.id.added_service_list);

        // Parent container: used to wrap service entries and feature buttons
        LinearLayout serviceContainer = new LinearLayout(this);
        serviceContainer.setOrientation(LinearLayout.VERTICAL);
        serviceContainer.setPadding(0, 20, 0, 0);

        // Top horizontal layout: text + delete button
        LinearLayout serviceHeader = new LinearLayout(this);
        serviceHeader.setOrientation(LinearLayout.HORIZONTAL);
        serviceHeader.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        ));
        serviceHeader.setBackgroundColor(getResources().getColor(android.R.color.holo_blue_light));
        serviceHeader.setPadding(16, 16, 16, 16);

        // Display Service Name and UUID
        TextView newService = new TextView(this);
        newService.setText("Name: " + name + "\nUUID: " + uuid);
        newService.setLayoutParams(new LinearLayout.LayoutParams(
                0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)); // fill up the remaining space

        // Delete button
        Button deleteButton = new Button(this);
        deleteButton.setText("🗑");
        deleteButton.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        ));

        // Subcontainer: used to add Characteristic buttons
        LinearLayout characteristicContainer = new LinearLayout(this);
        characteristicContainer.setOrientation(LinearLayout.VERTICAL);
        characteristicContainer.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        ));

        // Removal logic: removes the entire serviceContainer from the parent layout and updates the SharedPreferences.
        deleteButton.setOnClickListener(v -> {
            // Remove this service from SharedPreferences
            removeServiceData(name);

            // Remove the service and all related feature entries
            addedServiceList.removeView(serviceContainer);
            Toast.makeText(this, "Service and its characteristics deleted", Toast.LENGTH_SHORT).show();
        });

        // Click on the Add Characteristic button to add an entry.
        newService.setOnClickListener(v -> {
            addCharacteristicButton(characteristicContainer);
        });

        // Assembly Layout
        serviceHeader.addView(newService);
        serviceHeader.addView(deleteButton);
        serviceContainer.addView(serviceHeader);
        serviceContainer.addView(characteristicContainer);
        addedServiceList.addView(serviceContainer);
    }

    private void addCharacteristicButton(LinearLayout container) {
        Button addCharBtn = new Button(this);
        addCharBtn.setText("Add Characteristic");
        addCharBtn.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        ));
        addCharBtn.setPadding(16, 16, 16, 16);

        addCharBtn.setOnClickListener(v -> {
            showAddCharacteristicDialog(container); // Show dialog box
        });

        container.addView(addCharBtn);
    }

    private void showAddCharacteristicDialog(LinearLayout container) {
        // Loading custom layouts
        View dialogView = getLayoutInflater().inflate(R.layout.dialog_add_characteristic, null);

        EditText etName = dialogView.findViewById(R.id.et_char_name);
        EditText etUuid = dialogView.findViewById(R.id.et_char_uuid);
        EditText etValue = dialogView.findViewById(R.id.et_char_value);

        CheckBox cbRead = dialogView.findViewById(R.id.cb_read);
        CheckBox cbWrite = dialogView.findViewById(R.id.cb_write);
        CheckBox cbWriteNoResp = dialogView.findViewById(R.id.cb_write_no_response);
        CheckBox cbNotify = dialogView.findViewById(R.id.cb_notify);
        CheckBox cbIndicate = dialogView.findViewById(R.id.cb_indicate);

        new AlertDialog.Builder(this)
                .setTitle("Add Characteristic")
                .setView(dialogView)
                .setNegativeButton("Cancel", (dialog, which) -> dialog.dismiss())
                .setPositiveButton("OK", (dialog, which) -> {
                    String name = etName.getText().toString();
                    String uuid = etUuid.getText().toString();
                    String value = etValue.getText().toString();

                    // Save characteristic to SharedPreferences
                    saveCharacteristicData(name, uuid, value);

                    // Create and display features
                    displayCharacteristic(name, uuid, value);
                })
                .show();
    }

    private void saveCharacteristicData(String name, String uuid, String value) {
        SharedPreferences sharedPreferences = getSharedPreferences("GattServiceData", MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPreferences.edit();

        // Getting saved service data
        String savedServices = sharedPreferences.getString("services", "[]");
        try {
            JSONArray serviceArray = new JSONArray(savedServices);
            for (int i = 0; i < serviceArray.length(); i++) {
                JSONObject serviceObject = serviceArray.getJSONObject(i);
                JSONArray characteristicsArray = serviceObject.getJSONArray("characteristics");

                // Create a new characteristic object and add
                JSONObject characteristicObject = new JSONObject();
                characteristicObject.put("charName", name);
                characteristicObject.put("charUuid", uuid);
                characteristicObject.put("charValue", value);

                characteristicsArray.put(characteristicObject);
            }

            // Saving updated services data
            editor.putString("services", serviceArray.toString());
            editor.apply();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void displayCharacteristic(String name, String uuid, String value) {
        LinearLayout addedServiceList = findViewById(R.id.added_service_list);

        // Creating a new feature view
        TextView charView = new TextView(this);
        charView.setText("Characteristic Name: " + name + "\nUUID: " + uuid + "\nInitial Value: " + value);
        charView.setPadding(32, 16, 32, 16);
        charView.setBackgroundColor(getResources().getColor(android.R.color.darker_gray));
        charView.setTextColor(Color.WHITE);

        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        lp.setMargins(0, 8, 0, 0);
        charView.setLayoutParams(lp);

        addedServiceList.addView(charView);
    }

    private void removeServiceData(String serviceName) {
        SharedPreferences sharedPreferences = getSharedPreferences("GattServiceData", MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPreferences.edit();

        // Getting saved service data
        String savedServices = sharedPreferences.getString("services", "[]");
        try {
            JSONArray serviceArray = new JSONArray(savedServices);
            JSONArray newServiceArray = new JSONArray();

            for (int i = 0; i < serviceArray.length(); i++) {
                JSONObject serviceObject = serviceArray.getJSONObject(i);
                if (!serviceObject.getString("serviceName").equals(serviceName)) {
                    newServiceArray.put(serviceObject);
                }
            }

            // Saving updated services data
            editor.putString("services", newServiceArray.toString());
            editor.apply();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // Closes the Activity when the back arrow is clicked
    @Override
    public boolean onSupportNavigateUp() {
        finish();
        return true;
    }
}

