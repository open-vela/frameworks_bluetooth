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

package com.openvela.bluetooth;

import android.os.Parcel;
import android.os.Parcelable;

import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.ScanRecord;

import androidx.annotation.RestrictTo;

public class BtDevice implements Parcelable {
    private int connectionState = BluetoothProfile.STATE_CONNECTED;
    private String address;
    private String name;
    private int rssi;
    private boolean connectable;
    private ScanRecord scanRecord;
    private long rssiUpdateTime;

    public BtDevice(String address, String name) {
        this.address = address;
        this.name = name;
    }

    protected BtDevice(Parcel in) {
        this.connectionState = in.readInt();
        this.address = in.readString();
        this.name = in.readString();
        this.connectable = in.readBoolean();
    }

    @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
    public static final Creator<BtDevice> CREATOR = new Creator<BtDevice>() {
        @Override
        public BtDevice createFromParcel(Parcel in) {
            return new BtDevice(in);
        }

        @Override
        public BtDevice[] newArray(int size) {
            return new BtDevice[size];
        }
    };

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(this.connectionState);
        dest.writeString(this.address);
        dest.writeString(this.name);
        dest.writeBoolean(this.connectable);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    public int getConnectionState() {
        return this.connectionState;
    }

    public void setConnectionState(int state) {
        this.connectionState = state;
    }

    public String getAddress() {
        return this.address;
    }

    public void setAddress(String address) {
        this.address = address;
    }

    public String getName() {
        return this.name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public int getRssi() {
        return this.rssi;
    }

    public void setRssi(int rssi) {
        this.rssi = rssi;
    }

    public boolean isConnectable() {
        return this.connectable;
    }

    public void setConnectable(boolean Connectable) {
        this.connectable = Connectable;
    }

    public ScanRecord getScanRecord() {
        return this.scanRecord;
    }

    public void setScanRecord(ScanRecord scanRecord) {
        this.scanRecord = scanRecord;
    }

    public long getRssiUpdateTime() {
        return this.rssiUpdateTime;
    }

    public void setRssiUpdateTime(long rssiUpdateTime) {
        this.rssiUpdateTime = rssiUpdateTime;
    }
}
