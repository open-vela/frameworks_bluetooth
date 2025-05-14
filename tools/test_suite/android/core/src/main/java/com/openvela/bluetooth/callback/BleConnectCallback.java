package com.openvela.bluetooth.callback;

public abstract class BleConnectCallback<T> {
    public static final int FAILED_DEVICE_NOT_FOUND = 1000;
    public static final int FAILED_TIMEOUT = 1001;

    public void onConnectionChanged(String address, int newState) {}

    public void onConnectFailed(String address, int errorCode) {}

    public void onServicesDiscovered(String address) {}

}
