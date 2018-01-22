package com.limfamily.onionalarm;

import android.app.AlarmManager;
import android.os.Build;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.TimePicker;

import java.util.Calendar;

public class MainActivity extends AppCompatActivity {
    private BluetoothHandler bluetoothHandler;
    //Making our alarm manager
    AlarmManager alarm_manager;
    TimePicker alarm_timepicker;
    TextView update_text;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        //
        bluetoothHandler = new BluetoothHandler(this);
        String addr = bluetoothHandler.scan();
        bluetoothHandler.init(addr);

        //Initialize our alarm manager
        alarm_manager=(AlarmManager) getSystemService(ALARM_SERVICE);

        //initialize our timepicker
        alarm_timepicker = (TimePicker) findViewById(R.id.timePicker);

        //initialize our text update box
        update_text = (TextView) findViewById(R.id.update_text);

        // create an instance of a calendar
        final Calendar calendar = Calendar.getInstance();

        //Initialize start button
        Button alarm_on =(Button) findViewById(R.id.alarm_on);

        //Create an onClick Listener to set alarm
        alarm_on.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                //Setting calendar instance with the hour and minute that we picked
                //On the time picker
                //calendar.set(Calendar.HOUR_OF_DAY, alarm_timepicker.getHour());
                //calendar.set(Calendar.MINUTE, alarm_timepicker.getMinute());
                if(Build.VERSION.SDK_INT < 23){
                    int getHour = alarm_timepicker.getCurrentHour();
                    int getMinute = alarm_timepicker.getCurrentMinute();

                    calendar.set(Calendar.HOUR_OF_DAY, alarm_timepicker.getCurrentHour());
                    calendar.set(Calendar.MINUTE, alarm_timepicker.getCurrentMinute());
                } else{
                    int getHour = alarm_timepicker.getHour();
                    int getMinute = alarm_timepicker.getMinute();

                    calendar.set(Calendar.HOUR_OF_DAY, alarm_timepicker.getHour());
                    calendar.set(Calendar.MINUTE, alarm_timepicker.getMinute());
                }
                int hour;
                int minute;
                //get the int values of the hour and minute
                if(Build.VERSION.SDK_INT < 23){
                    hour = alarm_timepicker.getCurrentHour();
                    minute = alarm_timepicker.getCurrentMinute();

                    //Convert the int values to string
                    String hour_string= String.valueOf(hour);
                    String minute_string= String.valueOf(minute);
                    setAlarmText("Alarm set to: " + hour_string + ":" + minute_string);
                } else{

                    hour = alarm_timepicker.getHour();
                    minute = alarm_timepicker.getMinute();

                    //Convert the int values to string
                    String hour_string = String.valueOf(hour);
                    String minute_string = String.valueOf(minute);
                    setAlarmText("Alarm set to: " + hour_string + ":" + minute_string);
                }

                //method that changes the update text Textbox
                setAlarmText("Alarm on!");

                if (bluetoothHandler.isSetup())
                    bluetoothHandler.setAlarm(hour, minute);
            }
        });

        //Initialize stop button
        Button alarm_off =(Button) findViewById(R.id.alarm_off);

        //Create an onclick Listener to stop the alarm/undo alarm setting
        alarm_off.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //method that changes the update text Textbox
                bluetoothHandler.cancelAlarm();
                setAlarmText("Alarm Off!");
            }
        });
        //Initialize stop button
        Button shutdown =(Button) findViewById(R.id.shutdown);

        //Create an onclick Listener to stop the alarm/undo alarm setting
            shutdown.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //method that changes the update text Textbox
                bluetoothHandler.shutdown();
                bluetoothHandler.cleanup();
                setAlarmText("Shutdown!");
            }
        });
    }

    private void setAlarmText(String output) {
        update_text.setText(output);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        bluetoothHandler.cleanup();
    }
}
