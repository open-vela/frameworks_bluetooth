package com.openvela.bluetooth.callback;

public abstract class BluetoothDiscoveryCallback<T> {
    public static final int FAILED_ALREADY_STARTED = 1;
    public static final int FAILED_APPLICATION_REGISTRATION_FAILED = 2;
    public static final int FAILED_FEATURE_UNSUPPORTED = 4;
    public static final int FAILED_INTERNAL_ERROR = 3;
    public static final int FAILED_OUT_OF_HARDWARE_RESOURCES = 5;
    public static final int FAILED_SCANNING_TOO_FREQUENTLY = 6;

    public void onStart() {}

    public void onStop() {}

    public abstract void onDiscoveryResult(T device);

    public void onDiscoveryFailed(int errorCode) {}
}
