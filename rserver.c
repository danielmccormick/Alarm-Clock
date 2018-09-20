include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include "oled-exp.h"
#include <time.h>
#include <string.h>
#include "bluetooth/bluetooth.h"
#include "bluetooth/rfcomm.h"

enum msg_type { SETALARM = 0, CANCELALARM = 1, SHUTDOWN = 2};

int msg_lens[3] = {2, 0, 0};

struct blue_hndlr {
	struct sockaddr_rc local_addr; // Address of Omega2
	struct sockaddr_rc remote_addr; // Address of Phone
	int socket;
	int client;
	socklen_t rem_len;
};

struct nice_time { //makes an easy struct to deconstruct
	int hours;
	int minutes;
	int seconds;
};

void displayTime(struct nice_time *); // Displays the current time until the alarm goes off
void displayEnding(int); // End behaviour (post alarm)
void oled_off(); // Turns oled on
void oled_on(); // Turns oled off
int adjustTime(struct nice_time *, int); // Adjusts the time
void oled_setup(); // Sets up the oled
int alarm_loop(); // Alarm timer loop
void get_time_until_from_time(struct nice_time *, int, int); // Calculutes time until alarm goes off
int blue_loop(); // Loop for recieving 
void alarm_trigger(); // Triggers the alarm
void oled_flash(int); // Flashes screen
int ba2str(const bdaddr_t *, char *); // Changes bluetooth address to string

void displayTime(struct nice_time *time) { // Since the Omega2 doesn't have a very pretty way of describing the time we deconstruct the time left
	printf("Displaying time\n");
	oledClear();
	char i = 0;
	char message[9];
	message[i++] = (time->hours/10) + '0'; // This is a pretty hacky way of making a string
	message[i++] = (time->hours%10) + '0';
	message[i++] = ':';
	message[i++] = (time->minutes/10) + '0';
	message[i++] = (time->minutes%10) + '0';
	message[i++] = ':';
	message[i++] = (time->seconds/10) + '0';
	message[i++] = (time->seconds%10) + '0';	
	message[i] = 0;
	oledWrite(message); 
}

void displayEnding(int value){ // power is arbitrary, 0 for first, 1 for invert colours, else is revert colours
	int status;
	if (!value) {
		uint8_t *buffer = malloc(OLED_EXP_WIDTH*OLED_EXP_HEIGHT/8 * sizeof *buffer); // malloc
		status = oledReadLcdFile("image.lcd", buffer);
		if (status == EXIT_SUCCESS) {
			status = oledDraw(buffer, OLED_EXP_WIDTH*OLED_EXP_HEIGHT/8);
		}
		return;
	}
	else if (value == 1) {
		status = oledSetDisplayMode(1);
		return;
	}
	else if (value == 5){
		status = oledSetDisplayMode(0);
		return;
	}
	else {
		status = oledSetDisplayMode(0);
	}
	
}
int setPower(int power) { // 0 for min, 255 for max
	int status;
	printf("Now setting power to %d\n", power);
	status = oledSetBrightness(power);
}

void oled_off(){
	printf("Powering OLED off\n");
	oledSetDisplayPower(0);
	return;
}

void oled_on(){
	printf("Powering OLED on\n");
	oledSetDisplayPower(1);
	return;
}


int adjustTime(struct nice_time *time, int difference) { // hopefully this should be fine .. in hindsight there should be a better way to do this (maybe a library instead of a case-by case situation)
	if (!time->hours && !time->minutes && !time->seconds) {
		return -1; //Out of time
	} 
	if (difference > time->seconds) {
		if (time->minutes == 0 && time->hours > 0) {
			time->hours --;
			time->minutes += 60;
		}
		else if (time->minutes > 0) {
			time->minutes--;
			time->seconds+= 60;
		}
		else if (time->minutes <= 0 && time->hours <= 0) {
			time->hours = 0;
			time->minutes = 0;
			time->seconds = 0;
			printf("Time's up!\n");
			return 1;
		}		
	}
	time->seconds -= difference;
	printf("New Time:%d:%d:%d\n", hours, minutes, seconds); // Debug to PC, doesn't really make a difference
	return 0;
}

void oled_setup() { // Initialize the oled
	printf("Preparing oled\n");
	oledDriverInit();
	oledSetDisplayPower(1);
	oledWrite("Hello, World!");
}

int alarm_loop(time_t *last_time, struct nice_time *alarm) {
	// Alarm is disabled
	if (alarm->hours == -1) {
		return 0;
	}

	if (alarm->hours > 0 || alarm->minutes > 0 || alarm->seconds > 0) {
		time_t cur_time = time(NULL);
		int deltaT = (difftime(cur_time, *last_time));
		if (deltaT) {
			*last_time = cur_time;
			adjustTime(alarm, deltaT);
			displayTime(alarm);
		}
	}
	else {
		return 1;
	}
	return 0;
}

void get_time_until_from_time(struct nice_time *alarm, int hours, int minutes) {
	time_t now = time(NULL);
	struct tm *timeinfo = localtime(&now);
	printf("cur time: %d:%d:%d\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	int now_hr = timeinfo->tm_hour;
	if (now_hr < 0) {
	       	now_hr += 24;
	}
	int now_min = timeinfo->tm_min;
	int now_sec = timeinfo->tm_sec;
	if (now_sec) {
		alarm->seconds = 60-now_sec;
		alarm->minutes = minutes-now_min-1;
	}
	else {
 		alarm->seconds = 0;
		alarm->minutes = minutes-now_min;
	}
	alarm->hours = hours-now_hr;

	if (alarm->minutes < 0) {
		alarm->minutes += 60;
		alarm->hours--;
	}
	if (alarm->hours < 0) {
		alarm->hours += 24;
	}
}

int blue_loop(struct blue_hndlr *blu, char* buf, struct nice_time* alarm) {
	memset(buf, 0, sizeof(buf));

	// read data from the client
	int bytes_avail;
	ioctl(blu->client, FIONREAD, &bytes_avail);
 	if (!bytes_avail) {
		return 0;
	}

	int bytes_read = read(blu->client, buf, sizeof(buf));
	if(bytes_read > 0) {
		printf("Received %d bytes\n", bytes_read);

		// Check the type
		enum msg_type type = buf[0];
		int min_len = msg_lens[type]+1;
		
		if (bytes_read < min_len) {
			printf("Not enough bytes were passed, required: %d\n", min_len);
			return 0;
		}

		switch (type) {
			case SETALARM:;
				char hour = (buf)[1];
				char minute = (buf)[2];
				if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
					printf("Invalid alarm setting %d:%d\n", hour, minute);
					break;
				}
				printf("Setting alarm for hour %d and minute %d\n", hour, minute);
				get_time_until_from_time(alarm, hour, minute);
				displayTime(alarm);
				break;
			case CANCELALARM:
				alarm->hours = -1;
				oledWrite("Alarm cancelled!");
				break;

			case SHUTDOWN:
				return 1;
				break;
		}
	}
	return 0;
}

void alarm_trigger(struct nice_time *alarm) { // Basically will flash the display
	printf("Alarm triggered\n");
	oled_flash(5);
	oledSetDisplayPower(1);
	oledClear();
	oledWrite("Hello again");
	alarm->hours = -1;
}

void oled_flash(int duration) { // This will flash the time to the oled
	printf("Flashing oled\n");
	displayEnding(0);
	int cond = 0;
	time_t cur_time, start_time = time(NULL);
	while (difftime(cur_time, start_time) < duration) {
		cur_time = time(NULL);
		int deltaT = (difftime(cur_time, start_time));
		if (deltaT) {
			if (cond == 0) {
				displayEnding(1);
				cond = 1;
			}
			else if (cond == 1){
				displayEnding(5);
				cond = 0;
			}
		}

	}
	printf("Done flashing OLED\n");
}

void loop(struct blue_hndlr *blu) {
	char buf[128] = { 0 };
	int bytes_read;
	int fl_cont = 1;
	struct nice_time alarm; 
	alarm.hours = -1; // Alarm disabled
	alarm.minutes = 0;
	alarm.seconds = 0;
	time_t last_time = time(NULL);
	while (fl_cont) {
		// Update alarm status
		if (alarm_loop(&last_time, &alarm)) {
			alarm_trigger(&alarm);
		}

		// Parse bluetooth messages
		if (blue_loop(blu, buf, &alarm)) {	
			fl_cont = 0;
		}
		usleep(10000);
	}
}

int main(int argc, char **argv) {

	struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
	struct blue_hndlr blu;

	oled_setup();

	blu.socket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	blu.rem_len = sizeof(rem_addr);

	// bind socket to port 1 of the first available 
	// local bluetooth adapter
	blu.local_addr.rc_family = AF_BLUETOOTH;
	blu.local_addr.rc_bdaddr = *BDADDR_ANY;
	blu.local_addr.rc_channel = (uint8_t) 1;

	bind(blu.socket, (struct sockaddr *)&blu.local_addr, sizeof(blu.local_addr));

	printf("Listening...\n");

 	// put socket into listening mode
 	listen(blu.socket, 1);

	// accept one connection
	blu.client = accept(blu.socket, (struct sockaddr *)&blu.remote_addr, &blu.rem_len);
	printf("Accepted connection...\n");
	printf("Entering main loop\n"):
	loop(&blu);
	oled_off();

	// close connection
	close(blu.client); // Maintaining the bluetooth connection also probably isn't fantastic either ...
	close(blu.socket);
	return 0;
}
