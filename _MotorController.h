#pragma once
#ifndef _MOTORCONTROLLER_H
#define _MOTORCONTROLLER_H
#include <ESP8266WiFi.h>
#include"custom_Websocket.h"
#include <iostream>
#include <string>
#include <time.h>
class _MotorController
{
private:
	// Debuge
	
	static bool debug;

	// Motor
	static int motor_anticlockwise_port;

	static int motor_clockwise_port;

	// Water Level Sensor
	static int water_level_sensor_port;

	// Timer Sensor Port
	static int timer_SDA_port;
	static int timer_SCL_port;

	// Invalve Port
	static int invalve_port;

	// OutValve Port
	static int outvalve_port;

protected:
	// Debug
	static custom_Websocket* socket;

	// Motor
	static bool motor_active_status;

	static bool motor_periodic_stop;

	static bool motor_clockwise_status;
	static bool motor_anticlockwise_status;

	// Invalve Port
	static bool invalve_status;

	// OutValve Port
	static bool outvalve_status;


	static void setup(int motor_anticlockwise, int motor_clockwise, int inlet_valve, int outlet_valve, int water_level_sensor_port_val, int timer_sensor_SDA_val, int timer_sensor_SCL_val);
	static int get_water_level();

	static int get_sensor();



	static void turn_periodic_on();

	

	_MotorController(custom_Websocket* socket_val);

	static void send_debug(char* data);
	static void send_debug(int int_data, char* data);
	static void send_debug(char*data, int int_data);
	static void send_debug(const char* first_data, const char* data);
	static void send_debug(char data);
	

	static int sensor_last_value;

	// Motor critical values
	static int min_water_level;
	static int extra_value_during_outvale;
	static int time_for_last_drain_phase; // In seconds
	static int time_for_motor_clockwise_phase;// In seconds
	static int time_for_motor_periodic_stop_phase;// In seconds
	static int time_for_motor_anticlockwise_phase;// In seconds
	static int time_for_motor_last_phase;// In seconds
	static int wait_time_for_cyclic_debug_send;// In seconds
	static int offline_interval_duration; 
	static int offlin_water_level;
	static int sensor_level_check_time;
	static int water_level_check_time;
	static int water_level_sensor_original_value;
	static int water_level_sensor_negate_value;

public:
	
	static bool debug_on(bool state)
	{
		debug = state;
		return state;
	}
	// Public motor fuctions for remote control

	static void turn_off();

	static void turn_on_inlet();
	static void turn_off_inlet();

	static void turn_on_outlet();

	static void turn_off_outlet();

	static void turn_on();

	static void turn_clockwise();

	static void turn_anticlockwise();

	// For remote debug
	static void waited_send_debug();
};

#endif //_MOTORCONTROLLER_H
