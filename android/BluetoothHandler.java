package com.limfamily.onionalarm;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.util.UUID;

public class BluetoothHandler {
    private BluetoothAdapter adapter;
    private UUID _uuid;
    private BluetoothSocket socket;
    private Context context;
    private boolean setup;

    public BluetoothHandler(Context context) {
        this.context = context;
        setup = false;
    }

    // TODO: actually scan
    public String scan() {
        return "00:1A:7D:DA:71:13";
    }

    public void init(String addr) {
        //IntentFilter filter = new IntentFilter(BluetoothDevice.ACTION_FOUND);
        //context.registerReceiver(receiver, filter);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            ((Activity)context).requestPermissions(new String[]{android.Manifest.permission.ACCESS_COARSE_LOCATION}, 1);
        }
        adapter = BluetoothAdapter.getDefaultAdapter();
        if (adapter != null && adapter.isEnabled()) {
            _uuid = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
            BluetoothDevice device = adapter.getRemoteDevice(addr);
            Log.i(BluetoothProtocol.TAG, "Name: "+device.getName()+" "+device.getAddress());
            try {
                if (adapter.isDiscovering())
                    adapter.cancelDiscovery();
                //adapter.startDiscovery();
                //socket = device.createRfcommSocketToServiceRecord(_uuid);
                socket =(BluetoothSocket) device.getClass().getMethod("createInsecureRfcommSocket", new Class[]{int.class}).invoke(device,1);
                Log.d(BluetoothProtocol.TAG, "beginning connection");
                socket.connect();
                Log.d(BluetoothProtocol.TAG, "Connected");
                setup = true;
            } catch (IOException e) {
                Log.e(BluetoothProtocol.TAG, "Error creating socket ", e);
            } catch (InvocationTargetException | NoSuchMethodException | IllegalAccessException e) {
                Log.e(BluetoothProtocol.TAG, "Error reflecting. This should never happen", e);
            }
        }
        else {
            Log.e(BluetoothProtocol.TAG, "Error, bluetooth init failed");
        }
    }

    public boolean isSetup() {
        return setup;
    }

    public boolean setAlarm(int hour, int minute) {
        byte[] payload = new byte[]{(byte)hour, (byte)minute};
        return socketWrite(BluetoothProtocol.ID_SETALARM, payload);
    }

    public boolean cancelAlarm() {
        return socketWrite(BluetoothProtocol.ID_CANCELALARM, null);
    }

    public boolean shutdown() {
        return socketWrite(BluetoothProtocol.ID_SHUTDOWN, null);
    }

    private boolean socketWrite(byte type, byte[] payload) {
        if (!isSetup())
            return true;
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        try {
            outputStream.write(type);
            if (payload != null) outputStream.write(payload);
            byte[] data = outputStream.toByteArray();
            socket.getOutputStream().write(data);
            socket.getOutputStream().flush();
            Log.d(BluetoothProtocol.TAG, "Sent " + data.length + " bytes");
        } catch (IOException e) {
            Log.e(BluetoothProtocol.TAG, "An error occurred whilst creating the bytes to write to the socket");
            return true;
        }

        return false;
    }

    public void cleanup() {
        // Don't forget to unregister the ACTION_FOUND receiver.
        //context.unregisterReceiver(receiver);
        if (socket != null) {
            try {
                socket.close();
                socket.getOutputStream().close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private final BroadcastReceiver receiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (BluetoothDevice.ACTION_FOUND.equals(action)) {
                // Discovery has found a device. Get the BluetoothDevice
                // object and its info from the Intent.
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                String deviceName = device.getName();
                String deviceHardwareAddress = device.getAddress(); // MAC address
                Log.i(BluetoothProtocol.TAG, deviceName+" "+deviceHardwareAddress);
            }
        }
    };
}
