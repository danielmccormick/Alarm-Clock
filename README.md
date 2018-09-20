# Alarm-Clock
Code for an omega2 alarm clock by Daniel McCormick and Thomas Lim

The idea in was pretty straightforwards: make a phone-controlled alarm clock for an omega2 microcontroller. In hindsight phones already have alarms, but this was a cool learning experience nontheless.

The omega2 is a microcontroller that runs WRT, we grabbed a bluetooth module and a small I/O circuit to make noise. The code for the Android companion app is under the android folder. 

The general concept is the phone sends a time to the omega2 over bluetooth (via RFCOMM protocol, the standard) 

Under rserver.c is the code we flashed onto the omega2 to make it actually work. 
