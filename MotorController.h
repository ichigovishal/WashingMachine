#pragma once
#ifndef MOTORCONTROLLER_H
#define MOTORCONTROLLER_H
#include "_MotorController.h"
#include "open_file.h"
#include <sstream>
#include <string>
class MotorController : public _MotorController
{
private:

	static bool active_state;
	static int interval_duration;
	static int rotation_interval;
	static open_file *file;

	static DynamicJsonDocument* data;

	static int last_nobe_value;

	static bool water_in();
	static bool water_out();
	static bool controller();
	
	// Needed Water Level
	static int needed_water_level;

	// Intervals' Section
	static int no_of_interval;
	static bool first_interval_cycle;
	static bool last_interval_cycle;
	static int current_cycle;
	

	// During Interval Section
	static int moter_start_time;
	static bool full_water;
	static bool empty_water;

	// Inside Interval Section
	static int rotation_start_time;
	static bool first_rotation_cycle;

	// Last Drain
	static bool last_water_drain;
	static int last_water_drain_start_time;

public:
	MotorController(open_file* file_val, custom_Websocket *socket);
	static void loop();
	static bool activate(char* user_val, int users_no, int interval_duration_val = 15, int interval_time_val = 3, int needed_water_level_val = 2);
	static bool activate(const char* user_val, int users_no, int interval_duration_val = 15, int interval_time_val = 3, int needed_water_level_val = 2);
	static bool deactivate(const char* reason);

	static bool active_status;
	static int remaining_interval_duration;
	static int remaining_interval_time;

	static char* current_user;
	static int users_index;

	bool setup_port(int motor_activation = 0, int motor_direction = 0, int inlet_valve = 0, int outlet_valve = 0, int water_level_sensor = 0, int timer_sensor_SDA = 0, int timer_sensor_SCL = 0);
	
	static void send_data_back();

	static bool setup_motor_critical_values(
		int min_water_level_val = -1,
		int extra_value_during_outvale_val = -1,
		int time_for_last_drain_phase_val = -1,
		int time_for_motor_clockwise_phase_val = -1,
		int time_for_motor_periodic_stop_phase_val = -1,
		int time_for_motor_anticlockwise_phase_val = -1,
		int time_for_motor_last_phase_val = -1,
		int wait_time_for_cyclic_debug_send_val = -1,
		int offline_interval_duration_val = -1,
		int offlin_water_level_val = -1,
		int sensor_level_check_time_val = -1,
		int water_level_check_time_val = -1	
	);

	
};

#endif //MOTORCONTROLLER_H