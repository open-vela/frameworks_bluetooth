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

package com.openvela.bluetooth;

import android.bluetooth.BluetoothAdapter;
import android.util.Log;

public class BtHelper {
    private static final String TAG = "BtHelper";
    public static String formatMacAddress(String input) {
        if (input == null || input.trim().isEmpty()) {
            Log.e(TAG, "Invalid input");
            return null;
        }

        String cleaned = input.replaceAll("[^0-9A-Fa-f]", "").toUpperCase();
        if (cleaned.length() != 12) {
            Log.e(TAG, "Invalid Bluetooth MAC address");
            return null;
        }

        String addr = cleaned.replaceAll("(..)(..)(..)(..)(..)(..)", "$1:$2:$3:$4:$5:$6");
        if (!BluetoothAdapter.checkBluetoothAddress(addr)) {
            Log.e(TAG, "Bluetooth Address detection failed");
            return null;
        }

        return addr;
    }
}
