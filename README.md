# Alarm-Clock
Code for an omega2 alarm clock by Daniel McCormick and Thomas Lim

The idea in was pretty straightforwards: make a phone-controlled alarm clock for an omega2 microcontroller. In hindsight phones that support bluetooth also  already have alarms, so this was more of a fun project and a really thorough idea,  but this was a cool learning experience nontheless.

The omega2 is a microcontroller that runs WRT, we grabbed a bluetooth module and a small I/O circuit to make noise.

The general concept is the phone sends a time to the omega2 over bluetooth (via RFCOMM protocol, the standard), the omega2 will continuously check the time, and when the time has changed, update it in relative real-timeto the display on the omega2 (from a destructured nice\_time unit). When the time is up, it effectively freaks out, as the display inverts and a current is run to make an obnoxious buzzing sound.

Under rserver.c is the code we flashed onto the omega2 to make it actually work. The code for the Android companion app is under the android folder.

Since this repository was more or less a dump, there's no guarantees about stability - and it's therefore a use at your own risk. 
