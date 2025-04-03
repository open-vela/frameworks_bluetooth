package com.openvela.bluetooth.callback;

import android.bluetooth.BluetoothDevice;

public interface BluetoothBondStateCallback {
    void onBonded(BluetoothDevice device);

    void onBondRemoved(BluetoothDevice device);
}
