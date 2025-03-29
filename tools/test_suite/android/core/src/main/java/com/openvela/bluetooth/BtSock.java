package com.openvela.bluetooth;

import static androidx.core.content.ContextCompat.getSystemService;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelUuid;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;
import java.util.zip.CRC32;

public class BtSock {
    private final String TAG = "BtSock";
    private int mSockRole;
    private final int SOCK_ROLE_UNKNOWN = 0;
    private final int SOCK_ROLE_SERVER = 1;
    private final int SOCK_ROLE_CLIENT = 2;

    private int mSockRxState;
    private final int SOCK_RX_STATE_IDLE = 0;
    private final int SOCK_RX_STATE_RECEIVING = 1;

    private int mSockTxState;
    private final int SOCK_TX_STATE_IDLE = 0;
    private final int SOCK_TX_STATE_SENDING = 0;
    private String mVar;
    public static final int MESSAGE_SOCK_LOGGING = 1;
    private BluetoothAdapter bluetoothAdapter;

    // It's used for Client or Server, for Server role, it means accepted socket
    private BluetoothSocket mSocket;
    // It's used only for Server
    private BluetoothServerSocket mServerSocket;

    // It's used for reading
    private BufferedInputStream mInputStream;

    // it's used for writing
    private BufferedOutputStream mOutputStream;
    // To Update UI
    private Handler mHandler;
    // Thread to sending data
    private TxThread mTxThread;

    // For Tput calculation
    private int mTotalSize;
    private long mStartTime;
    private long mStopTime;
    // BtSock index
    private int mIndex;
    // BtSock type
    private int mType;
    // cycles to do send operation
    private int mCycles;
    private String mStringToSend;
    private long mCrcValue;
    private StringBuffer mReceivedStrBuf;
    public static final int SOCK_TYPE_SPP_INSECURE = 0;
    public static final int SOCK_TYPE_SPP_SECURE = 1;
    public static final int SOCK_TYPE_L2CAP_BREDR_INSECURE = 2;
    public static final int SOCK_TYPE_L2CAP_BREDR_SECURE = 3;
    public static final int SOCK_TYPE_L2CAP_BLE_INSECURE = 4;
    public static final int SOCK_TYPE_L2CAP_BLE_SECURE = 5;

    public BtSock(int index, int type, Handler handler) {
        mIndex = index;
        mType = type;
        mHandler = handler;

        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter == null) {
            Log.e(TAG, "Device doesn't support Bluetooth");
            return;
        }

        mSockRole = SOCK_ROLE_UNKNOWN;
        mReceivedStrBuf = new StringBuffer();
    }

    // var means UUID, for SOCK_TYPE_SPP_xxx
    // var means PSM, for SOCK_TYPE_L2CAP_BLE_xxx
    // var means Channel, for SOCK_TYPE_L2CAP_BREDR_xxx
    public void register(String var) {
        if (mSockRole != SOCK_ROLE_UNKNOWN) {
            showLogs("Unexpected register, current role = " + mSockRole);
            return;
        }

        mSockRole = SOCK_ROLE_SERVER;
        mVar = var;

        // Register server with different API, based on mType
        boolean ret = registerServer(var);
        if (!ret)
            return;

        // Start AcceptThread to wait for a new connection from other clients
        AcceptThread thread = new AcceptThread();
        thread.start();
    }

    public void unregister() {
        if (mSockRole != SOCK_ROLE_SERVER) {
            showLogs("Unexpected unregister, current role = " + mSockRole);
            return;
        }

        // TODO:
        stopAdvertising();
        mSockRole = SOCK_ROLE_UNKNOWN;

    }

    public void connect(String bdAddr, String var) {
        if (mSockRole != SOCK_ROLE_UNKNOWN) {
            showLogs("Unexpected connect, current role = " + mSockRole);
            return;
        }

        mSockRole = SOCK_ROLE_CLIENT;

        BluetoothDevice device = bluetoothAdapter.getRemoteDevice(bdAddr);

        // Connect remote device with different API, based on mType
        connectRemote(device, var);

        // Start ConnectThread
        ConnectThread thread = new ConnectThread();
        thread.start();
        Log.i(TAG, "onClick: Connected Socket to" + bdAddr);
    }

    public void disconnect() {
        if (null != mOutputStream) try {
            mOutputStream.flush();
            mOutputStream.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        mOutputStream = null;

        if (null != mInputStream) try {
            mInputStream.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        mInputStream = null;

        if (null != mSocket) try {
            mSocket.getOutputStream().close();
            mSocket.getInputStream().close();
            mSocket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        mSocket = null;

        mSockRole = SOCK_ROLE_UNKNOWN;
    }

    public void send(String msgToSend, int cycles) {
        // TODO: Check whether it's busy now
        if ((mSockTxState != SOCK_TX_STATE_IDLE) || (mSockRxState != SOCK_RX_STATE_IDLE)) {
            showLogs("Unexpected send: it's busy now, mSockTxState = " + mSockTxState + ", mSockRxState = " + mSockRxState);
            return;
        }

        if ((null == mSocket) || (null == mOutputStream)) {
              return;
        }

        mCycles = cycles;
        mStringToSend = msgToSend;
        Log.d (TAG, "------- Role(" + mSockRole + ") Index(" + mIndex +  ") Send: mCycles = " + mCycles);
        if (mCycles <= 0) {
            return;
        }

        int num = 0;
        if (msgToSend.startsWith("VelaTest:")) { // Start Tput testing
            mSockTxState = SOCK_TX_STATE_SENDING;

            num = Integer.parseInt(msgToSend.substring(9));

            /* Option #1: when num = 12888, the time consuming is about 468ms
            for (int i = 0; i <= num; i++) {
                msgToSend = msgToSend + String.valueOf(i);
            } */

            // Option #2:
            int tmp = num;
            int begin = 0;
            int end = 10;
            int countDigits = 1;
            int numOfChar = msgToSend.length();
            //Log.d (TAG, "numOfChar init = " + numOfChar);
            do {
                if (num >= end)
                    numOfChar += (end - begin)  * countDigits;
                else if ((num >= begin) && (num < end))
                    numOfChar += (num - begin + 1) * countDigits;
                else
                    Log.e(TAG, "wrong case!!!");

                countDigits++;
                begin = end;
                end = 10 * begin;
                tmp /= 10;
            } while (tmp != 0);

            //Log.d (TAG, "numOfChar = " + numOfChar);

            msgToSend = "START:" + numOfChar;

            // Delay the sending in another Thread, after receiving ACK from PEER
            mTxThread = new TxThread(num);
        }

        writeStr(msgToSend);

        // Show logs on UI
        String str = "Sent: size = " + msgToSend.length() + " Bytes: \"" + msgToSend + "\"\r\n";
        showLogs(str);
    }

    // AcceptThread is used by Server to listen a connection from other clients
    private class AcceptThread extends Thread {
        public void run() {
            Log.d(TAG, "AcceptThread: started");

            while (true) {
                try {
                    // It's a blocking operation, so we shall do it in a background thread
                    Log.d(TAG, "before server.accept()");
                    mSocket = mServerSocket.accept();
                    Log.d(TAG, "after server.accept(), acceptSocket = " + mSocket);
                } catch (IOException e) {
                    e.printStackTrace();
                    //return;
                }

                if (null != mSocket) {
                    Log.i(TAG, "Accepted one connection...");

                    try {
                        mInputStream = new BufferedInputStream(mSocket.getInputStream());
                        mOutputStream = new BufferedOutputStream(mSocket.getOutputStream());

                        // Start receiving data
                        RxThread thread = new RxThread();
                        thread.start();

                        // Only accept one connection!!!

                        // Show logs on UI
                        String str = "Server: accepted one socket connection and started reading \r\n";
                        showLogs(str);
                        break;
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    }

    // ConnectThread is used by Client to connect other Servers
    private class ConnectThread extends Thread {
        public void run() {
            Log.d(TAG, "ConnectThread: started");

            // Create RFCOMM socket, as a Client
            try {
                // It's a blocking operation, so we shall do it in background
                Log.d(TAG, "onClick: before BluetoothSocket::connect");
                mSocket.connect();
                Log.d(TAG, "onClick: after BluetoothSocket::connect");

                Log.d(TAG, "onClick: before BluetoothSocket::getInputStream");
                InputStream iStream = mSocket.getInputStream();
                Log.e(TAG, "onClick: after BluetoothSocket::getInputStream, iStream = " + iStream);
                if (null == iStream) {
                    Log.e(TAG, "Failed to getInputStream");
                    return;
                }
                mInputStream = new BufferedInputStream(iStream);

                Log.d(TAG, "onClick: before BluetoothSocket::getOutputStream");
                OutputStream oStream = mSocket.getOutputStream();
                Log.e(TAG, "onClick: after BluetoothSocket::getOutputStream, iStream = " + oStream);
                if (null == oStream) {
                    Log.e(TAG, "Failed to getOutputStream");
                    return;
                }
                mOutputStream = new BufferedOutputStream(oStream);
            } catch (IOException e) {
                e.printStackTrace();
                return;
            }

            // Start receiving data
            RxThread thread = new RxThread();
            thread.start();

            // Show logs on UI
            String str = "Client: Connected one server and started reading\r\n";
            showLogs(str);
        }
    }

    // RxThread is used by Client or Server to Receive data (in the background) after connection
    private class RxThread extends Thread {
        public void run() {
            int readSize = 0;
            byte[] buffer = new byte[1000];
            String readStr;
            int totalSizeToReceive = 0;
            int totalReceived = 0;
            long duration = 0;
            String str;

            Log.d(TAG, "RxThread: started");

            try {
                while ((readSize = mInputStream.read(buffer, 0, buffer.length)) != -1) {
                    //Log.d(TAG, "Received: readSize = " + readSize + ", buffer = " + buffer);
                    readStr = new String(buffer, 0, readSize, "UTF-8");

                    if (readStr.startsWith("START:")) {
                        //showLogs("Received \"START:\"\r\n");
                        if ((mSockTxState != SOCK_TX_STATE_IDLE) || (mSockRxState != SOCK_RX_STATE_IDLE)) {
                            showLogs("SPP is busy for sending now, ignore Rx Tput test request");
                        }
                        totalSizeToReceive = Integer.parseInt(readStr.substring(6));

                        // Write ACK to remote
                        writeStr("START_ACK");

                        mSockRxState = SOCK_RX_STATE_RECEIVING;
                        mTotalSize = totalSizeToReceive;
                        mStartTime = System.currentTimeMillis();
                        totalReceived = 0;

                        mReceivedStrBuf.setLength(0);
                        continue;
                    } else if (readStr.startsWith("START_ACK")) {
                        showLogs("Received \"START_ACK\"\r\n");
                        if (mSockTxState == SOCK_TX_STATE_SENDING) {
                            // Received ACK, continue to send data in another thread
                            mTxThread.start();
                        }
                        continue;
                    } else if (readStr.startsWith("EOF")) {
                        showLogs("Received \"EOF\"\r\n");
                        // Calculate Tput
                        mStopTime = System.currentTimeMillis();
                        duration = mStopTime - mStartTime;

                        str = "Sent Total " + String.valueOf(mTotalSize) + " Bytes";
                        if (duration > 0)
                            str += ", Duration: " + duration + " ms, Average Tput = " + mTotalSize/duration + " kB/s";
                        str += "\r\n";

                        // Check CRC
                        long crcValue = Long.parseLong(readStr.substring(3));
                        str += "CRC checking: Send CRC = " + mCrcValue + ", Receive CRC = " + crcValue + "\r\n";
                        if (mCrcValue != crcValue) {
                            Log.e(TAG, "Received wrongly!!!");
                            mCycles = 0;
                        }

                        // Send message to UI
                        showLogs(str);

                        mStartTime = 0;
                        mStopTime = 0;
                        mTotalSize = 0;
                        mSockTxState = SOCK_TX_STATE_IDLE;

                        // Restart Tx, for Stress Test
                        if (mCycles > 0) {
                            send(mStringToSend, mCycles - 1);
                        }

                        continue;
                    }

                    Log.d(TAG, "Continue to handle string: totalReceived = " + totalReceived + ", readSize = " + readSize
                            + ", totalSizeToReceive = " + totalSizeToReceive);
                    if (mSockRxState == SOCK_RX_STATE_RECEIVING) {
                        totalReceived += readSize;
                        mReceivedStrBuf.append(readStr);
                        Log.d(TAG, "Updated totalReceived = " + totalReceived + ", readSize = " + readSize);

                        if (totalReceived >= totalSizeToReceive) {
                            mStopTime = System.currentTimeMillis();
                            duration = mStopTime - mStartTime;

                            str = "Received Total " + String.valueOf(totalReceived) + " Bytes)";
                            if (duration > 0)
                                str += ", Duration: " + duration + " ms, Average Tput = " + mTotalSize/duration + " kB/s";
                            str += "\r\n";

                            //showLogs(str);

                            // Calculate CRC
                            CRC32 crc = new CRC32();
                            crc.update(mReceivedStrBuf.toString().getBytes());
                            long crcValue = crc.getValue();

                            // Tell remote to stop sending and calculate Tput on remote side
                            writeStr("EOF" + crcValue);

                            mStartTime = 0;
                            mStopTime = 0;
                            mTotalSize = 0;
                            mSockRxState = SOCK_RX_STATE_IDLE;
                        }
                    } else {
                        str = "Received " + String.valueOf(readSize) + " Bytes: " + readStr +"\r\n";

                        // Send message to UI
                        //showLogs(str);
                    }
                }
            } catch (IOException e) {
                e.printStackTrace();
                //return;
            }

            if (mSockRole == SOCK_ROLE_SERVER) {
                showLogs("Socket unexpectedly disconnected, restart Accept Thread again");
                disconnect();
                register(mVar);
            }
        }
    }

    // TxThread is used by Client or Server to Send data (in the background) after connection
    private class TxThread extends Thread {
        int mNumToSend;

        public TxThread(int num) {
            mNumToSend = num;
        }

        public void run() {
            String msgToSend = "VelaTest:" + mNumToSend;
            int totalSize = 0;
            int start = 0;
            int last = 0;

            //Log.d(TAG, "TxThread: started");

            if (mSockTxState != SOCK_TX_STATE_SENDING)
                return;

            // Building string to be sent:
            long testTimeStart = System.currentTimeMillis();
            showLogs("Preparing data to be sent ...\r\n");

            /* Option#1: low efficiency!
            for (int i = 0; i <= mNumToSend; i++) {
                msgToSend = msgToSend + String.valueOf(i);
            }
             */

            // Option#2: Higher efficiency, but not thread-safety useage.
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i <= mNumToSend; i++) {
                sb.append(i);
            }
            msgToSend += sb.toString();

            // Calculate CRC
            CRC32 crc = new CRC32();
            crc.update(msgToSend.getBytes());
            mCrcValue = crc.getValue();

            long testTimeStop = System.currentTimeMillis();
            long duration = testTimeStop - testTimeStart;
            showLogs("Done. Time to generation string = " + duration + " ms\r\n");

            totalSize = msgToSend.length();

            mTotalSize = totalSize;
            mStartTime = System.currentTimeMillis();

            start = 0;
            int step = 1000;
            while (true) {
                last = start + step;
                if (last > totalSize) {
                    writeStr(msgToSend.substring(start));
                } else {
                    writeStr(msgToSend.substring(start, last));
                }

                start = last;
                if (start >= totalSize)
                    break;
            }

            // Show logs on UI
            String str = "Sent: size = " + totalSize + "Bytes\r\n";
            showLogs(str);
        }
    }

    // To show logs on UI
    private void showLogs(String str) {
        if (mCycles > 1)
            return;

        if (mSockRole == SOCK_ROLE_CLIENT)
            str = "\r\nClient(" + mIndex + "): " + str;
        else if (mSockRole == SOCK_ROLE_SERVER)
            str = "\r\nServer(" + mIndex + "): " + str;

        Bundle bData = new Bundle();
        bData.putString("log", str);

        Message msg = mHandler.obtainMessage();
        msg.what = BtSock.MESSAGE_SOCK_LOGGING;
        msg.setData(bData);
        mHandler.sendMessage(msg);
        Log.i(TAG, "showLogs: " + str);
    }

    private void writeStr(String msgToSend) {
        try {
            mOutputStream.write(msgToSend.getBytes());
            //mOutputStream.write('\r');
            //mOutputStream.write('\n');
            mOutputStream.flush();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private boolean registerServer(String var) {
        try {
            Log.d(TAG, "registerServer: mServerSocket = " + mServerSocket + ", type = " + mType + ", var = " + var);

            if (mServerSocket == null) {
                switch (mType) {
                    case SOCK_TYPE_SPP_INSECURE:
                        // var means UUID
                        mServerSocket = bluetoothAdapter.listenUsingInsecureRfcommWithServiceRecord("Vela BTS SPP Server", UUID.fromString(var));
                        break;

                    case SOCK_TYPE_SPP_SECURE:
                        // var means UUID
                        mServerSocket = bluetoothAdapter.listenUsingRfcommWithServiceRecord("Vela BTS SPP Server", UUID.fromString(var));
                        break;

                    case SOCK_TYPE_L2CAP_BLE_INSECURE:
                        mServerSocket = bluetoothAdapter.listenUsingInsecureL2capChannel();
                        mVar = String.valueOf(mServerSocket.getPsm());

                        startAdvertising();
                        break;

                    case SOCK_TYPE_L2CAP_BLE_SECURE:
                        mServerSocket = bluetoothAdapter.listenUsingL2capChannel();
                        mVar = String.valueOf(mServerSocket.getPsm());

                        startAdvertising();
                        break;

                    case SOCK_TYPE_L2CAP_BREDR_INSECURE:
                    case SOCK_TYPE_L2CAP_BREDR_SECURE:
                    default:
                        showLogs("Socket type Not supported: " + mType);
                        break;
                }
            }

            if (mServerSocket == null)
                return false;

            // Show logs on UI
            String str = "Registered Socket Server on Port/SCN/PSM = " + mServerSocket.getPsm() + "\r\n";
            showLogs(str);
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }

        return true;
    }

    private void connectRemote(BluetoothDevice device, String var) {
        Log.d(TAG, "onClick: Connect var = " + var);

        try {
            switch (mType) {
                case SOCK_TYPE_SPP_INSECURE:
                    mSocket = device.createInsecureRfcommSocketToServiceRecord(UUID.fromString(var));
                    break;

                case SOCK_TYPE_SPP_SECURE:
                    mSocket = device.createRfcommSocketToServiceRecord(UUID.fromString(var));
                    break;

                case SOCK_TYPE_L2CAP_BLE_INSECURE:
                    // var means PSM
                    mSocket = device.createInsecureL2capChannel(Integer.parseInt(var));
                    break;

                case SOCK_TYPE_L2CAP_BLE_SECURE:
                    // var means PSM
                    mSocket = device.createL2capChannel(Integer.parseInt(var));
                    break;

                // Fall Through
                case SOCK_TYPE_L2CAP_BREDR_INSECURE:
                    // var means Channel
                    //mSocket = device.createInsecureL2capSocket(Integer.parseInt(var));

                // Fall Through
                case SOCK_TYPE_L2CAP_BREDR_SECURE:
                    // var means Channel
                    //mSocket = device.createL2capSocket(Integer.parseInt(var));

                default:
                    showLogs("Socket type Not supported: " + mType);
                    break;
            }

            if (null == mSocket) {
                Log.e(TAG, "Failed to create socket");
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        Log.d(TAG, "onClick: Connected, socket = " + mSocket);
    }

    private void startAdvertising() {
        final UUID ADV_COC_SERVICE_UUID = UUID.fromString("00001234-0000-1000-8000-00805f9b34fb");

        Log.d(TAG, "startAdvertising: enter");

        BluetoothLeAdvertiser btAdvertiser = bluetoothAdapter.getBluetoothLeAdvertiser();
        AdvertiseData data = new AdvertiseData.Builder()
                .setIncludeDeviceName(true)
                .addServiceData(new ParcelUuid(ADV_COC_SERVICE_UUID), new byte[]{1, 2, 3})
                .addServiceUuid(new ParcelUuid(ADV_COC_SERVICE_UUID))
                //.addManufacturerData(0xFF00, new byte[]{1, 2, 3, 4, 5, 6})
                .build();
        AdvertiseSettings setting = new AdvertiseSettings.Builder()
                .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY)
                .setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_HIGH)
                .setConnectable(true)
                .setTimeout(0)
                .build();
        btAdvertiser.startAdvertising(setting, data, mAdvertiseCallback);
    }

    private final AdvertiseCallback mAdvertiseCallback = new AdvertiseCallback(){
        @Override
        public void onStartFailure(int errorCode) {
            // Implementation for API Test.
            super.onStartFailure(errorCode);
            showLogs("Start advertising: failed, errorCode = " + errorCode);
        }

        @Override
        public void onStartSuccess(AdvertiseSettings settingsInEffect) {
            // Implementation for API Test.
            super.onStartSuccess(settingsInEffect);
            showLogs("Start advertising: OK");
        }
    };

    private void stopAdvertising() {
        Log.d(TAG, "stopAdvertising: enter");

        BluetoothLeAdvertiser btAdvertiser = bluetoothAdapter.getBluetoothLeAdvertiser();
        btAdvertiser.stopAdvertising(mAdvertiseCallback);
    }
}
