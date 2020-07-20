#include "MotorController.h"



// Internal Vars
open_file *MotorController::file{ 0 };
DynamicJsonDocument *MotorController::data{ 0 };

int MotorController::interval_duration{ 30 };
int MotorController::no_of_interval{ 3 };
char* MotorController::current_user{};
int MotorController::users_index{};
bool MotorController::active_state{ false };
bool MotorController::first_interval_cycle{ true };
bool  MotorController::last_interval_cycle{ false };
bool MotorController::first_rotation_cycle{ true };
int MotorController::moter_start_time{ 0 };
int MotorController::current_cycle{ 0 };
int MotorController::rotation_interval{ 30 };
int  MotorController::rotation_start_time{ 0 };
int MotorController::needed_water_level{9};
bool MotorController::empty_water{ false };
bool MotorController::full_water{ false };
int MotorController::last_nobe_value{ 0 };
bool MotorController::last_water_drain { false };
int MotorController::last_water_drain_start_time{ 0 };

// Get Status Vars
bool MotorController::active_status{ false };
int MotorController::remaining_interval_duration{};
int MotorController::remaining_interval_time{};




MotorController::MotorController(open_file* file_val, custom_Websocket *socket)
	:_MotorController(socket)
{
	MotorController::file = file_val;
	MotorController::data = MotorController::file->read_data();
	MotorController::water_level_sensor_negate_value = (*MotorController::data)["water_level_sensor_negate_value"].as<int>();
	MotorController::setup_port();
	//MotorController::setup_motor_critical_values(1, 0, 45, 5, 1, 5, 1, 2, 2, 3, 3000, 3000);
	MotorController::setup_motor_critical_values();
	if ((!(*MotorController::data)["full_water"].as<bool>()) && MotorController::water_level_sensor_original_value > -3 && (*MotorController::data)["empty_water"].as<bool>())
	{
		Serial.println("Setting water level sensor..........");
		MotorController::water_level_sensor_negate_value = 0;
		do 
		{
			MotorController::water_level_sensor_negate_value++;
		} while (MotorController::water_level_sensor_negate_value == MotorController::water_level_sensor_original_value);
		file->add_data("water_level_sensor_negate_value", MotorController::water_level_sensor_negate_value);
		file->save_data();
	}
	if (MotorController::get_water_level() < 1)
	{
		MotorController::empty_water = true;
	}
}

// Intervals' Section


bool MotorController::water_in()
{
	if (MotorController::get_water_level() < MotorController::min_water_level && !MotorController::invalve_status)
	{
		MotorController::turn_on_inlet();
	}
	else if (MotorController::get_water_level() > MotorController::needed_water_level && MotorController::invalve_status)
	{
		MotorController::turn_off_inlet();
		MotorController::full_water = true;
		MotorController::data = MotorController::file->read_data();
		file->add_data("full_water", MotorController::full_water); 
		file->save_data();
	}
	
}

bool MotorController::water_out()
{

	if (MotorController::get_water_level() >= MotorController::min_water_level && !MotorController::outvalve_status)
	{
		MotorController::turn_on_outlet();
		MotorController::waited_send_debug();
	}
	else if ((MotorController::get_water_level() + MotorController::extra_value_during_outvale) < MotorController::min_water_level && MotorController::outvalve_status)
	{
		MotorController::waited_send_debug();
		
		if (!MotorController::last_water_drain)
		{
			MotorController::waited_send_debug();
			MotorController::last_water_drain = true;
			MotorController::last_water_drain_start_time = (unsigned int)(millis() / 1000);

		}
		else if (MotorController::last_water_drain && ((unsigned int)((unsigned int)(millis() / 1000) - MotorController::last_water_drain_start_time) > MotorController::time_for_last_drain_phase))
		{
			MotorController::waited_send_debug();
			MotorController::last_water_drain = false;
			MotorController::turn_off_outlet();
			MotorController::empty_water = true;
			MotorController::full_water = false;
			MotorController::data = MotorController::file->read_data();
			file->add_data("full_water", MotorController::full_water);
			file->add_data("empty_water", MotorController::empty_water);
			file->save_data();
		}
		
	}

}

bool MotorController::controller()
{
	if (MotorController::active_state && MotorController::first_interval_cycle)	
	{
		
		if (!MotorController::empty_water)
		{
			MotorController::water_out();
			MotorController::waited_send_debug();
		}
		else
		{
			MotorController::send_data_back();
			MotorController::current_cycle = 0;
			MotorController::first_interval_cycle = false;
			MotorController::last_interval_cycle = false;
		}
			
		
	}
	else if (MotorController::active_state && !MotorController::first_interval_cycle && !MotorController::last_interval_cycle)
	{
		// During Intervals
		if (!MotorController::motor_active_status)
		{
			if (!MotorController::empty_water)
			{
				MotorController::waited_send_debug();
				MotorController::water_out();
			}
			else
			{
				if (!MotorController::full_water)
				{
					MotorController::water_in();
					MotorController::waited_send_debug();
				}
				else
				{
					MotorController::turn_on();
					MotorController::moter_start_time = (unsigned int)(millis() / 60000);
					MotorController::empty_water = false;
					MotorController::send_data_back();
				}
			}
		}
		else if(MotorController::motor_active_status)
		{
			// Inside Intervals
			if (!MotorController::motor_clockwise_status && MotorController::first_rotation_cycle)
			{
			
				if ((!MotorController::motor_clockwise_status && ((unsigned int)((unsigned int)(millis() / 1000) - MotorController::time_for_motor_last_phase)) > 12 && MotorController::motor_periodic_stop) || !MotorController::motor_periodic_stop)
			    {
				    MotorController::turn_clockwise();
				    MotorController::rotation_start_time = (int)(millis() / 1000);
				    MotorController::first_rotation_cycle = false;
				    MotorController::send_debug("Current Cycle: ", MotorController::current_cycle);
			    }
	
			}
			else if (MotorController::motor_clockwise_status && ((unsigned int)((unsigned int)(millis() / 1000) - rotation_start_time)) > MotorController::time_for_motor_clockwise_phase)
			{
				MotorController::send_data_back();
				MotorController::turn_periodic_on();
				if ((int)(millis() / 60000) - MotorController::moter_start_time > MotorController::interval_duration)
				{
					MotorController::turn_off();
					MotorController::current_cycle++;
					if (MotorController::current_cycle == MotorController::no_of_interval)
					{
						MotorController::send_data_back();
						MotorController::last_interval_cycle = true;
						MotorController::send_debug("Entering last interval period.");	
					}
				}
			}
			else if (!MotorController::motor_clockwise_status && ((unsigned int)((unsigned int)(millis() / 1000) - rotation_start_time)) > MotorController::time_for_motor_periodic_stop_phase  && MotorController::motor_periodic_stop)
			{
				MotorController::turn_anticlockwise();
				MotorController::send_debug("Turning anticlockwise");
			}
			else if (!MotorController::motor_clockwise_status && ((unsigned int)((unsigned int)(millis() / 1000) - rotation_start_time)) > MotorController::time_for_motor_anticlockwise_phase && !MotorController::first_rotation_cycle)
			{
				MotorController::first_rotation_cycle = true;
				MotorController::turn_periodic_on();
				MotorController::send_debug("Current Cycle: ", MotorController::current_cycle);
			}
				


		}
	}
	else if (MotorController::active_state && MotorController::last_interval_cycle && !MotorController::first_interval_cycle)
	{
		static int time{ 0 };
		if (time == 0)
		{
			MotorController::send_debug("Enter the Last Cycle ");
			time++;
		}
		if (!MotorController::empty_water)
		{
			MotorController::waited_send_debug();
			MotorController::water_out();
		}
		else
		{
			MotorController::waited_send_debug();
			MotorController::send_data_back();
			MotorController::last_interval_cycle = false;
			MotorController::first_interval_cycle = true;
			MotorController::active_state = false;
			MotorController::active_status = false;
			time = 0;
		}
	}
}




void MotorController::loop()	
{ 
	if (!MotorController::active_state)
	{

		int nobe_value = MotorController::get_sensor();		
		if (!(MotorController::last_nobe_value == nobe_value) && nobe_value > 4 && !(nobe_value == 15))
		{
			delay(2000);
			nobe_value = MotorController::get_sensor();
			bool status = MotorController::activate("offline", -1, nobe_value, MotorController::offline_interval_duration, MotorController::offlin_water_level);
			if (status)
			{
				MotorController::send_debug("Offline user has started the machine and duration of interval ", nobe_value);
				file->add_data("last_nobe_value", nobe_value);
				file->save_data();
				MotorController::last_nobe_value = nobe_value;
			}

		}
		
		
	}

	
	else if (MotorController::active_state)
	{

		int nobe_value = MotorController::get_sensor();
		if ((nobe_value == 0 && !(MotorController::last_nobe_value == 0)) || (MotorController::users_index != -1 && !(MotorController::last_nobe_value == nobe_value)))
		{
			delay(2000);
			nobe_value = MotorController::get_sensor();
			bool status = MotorController::deactivate("physically cancellation.");

			if (!status)
			{
				file->add_data("last_nobe_value", nobe_value);
				file->save_data();
				MotorController::last_nobe_value = nobe_value;
			}

		}


	}	
	MotorController::controller();
}

bool MotorController::activate(char* user_val, int users_no, int interval_duration_val, int interval_time_val, int needed_water_level_val)
{
	bool status;	
	if (!MotorController::active_state)
	{
		MotorController::interval_duration = interval_duration_val;
		MotorController::no_of_interval = interval_time_val;
		MotorController::current_user = user_val;
		MotorController::users_index = users_no;
		MotorController::active_state = true;
		MotorController::needed_water_level = needed_water_level_val;
		status = true;
	}
	else
		status = false;

	MotorController::remaining_interval_duration = MotorController::no_of_interval - MotorController::current_cycle - 1;
	MotorController::remaining_interval_time = ((unsigned int)MotorController::interval_duration - (unsigned int)(millis()/60000));
	
	return status;
}

bool MotorController::activate(const char* user_val, int users_no, int interval_duration_val, int interval_time_val, int needed_water_level_val)
{
	bool status;

	if (!MotorController::active_state)
	{
		MotorController::interval_duration = interval_duration_val;
		MotorController::no_of_interval = interval_time_val;
		MotorController::current_user = (char*)user_val;
		MotorController::users_index = users_no;
		MotorController::active_state = true;
		MotorController::needed_water_level = needed_water_level_val;
		status = true;
	}
	else
		status = false;

	MotorController::remaining_interval_duration = MotorController::no_of_interval - MotorController::current_cycle - 1;
	MotorController::remaining_interval_time = ((unsigned int)MotorController::interval_duration - (unsigned int)(millis() / 60000)) + MotorController::moter_start_time;

	return status;
}

bool MotorController::deactivate(const char* reason)
{
	MotorController::send_debug("Turning off opertion because of ", reason);
	MotorController::first_interval_cycle = false;
	MotorController::first_rotation_cycle = false;

	if (MotorController::invalve_status)
	{
		MotorController::turn_off_inlet();
		
	}
	MotorController::turn_clockwise();
	MotorController::turn_periodic_on();
	MotorController::turn_off();
	MotorController::current_cycle = 0;
	MotorController::send_data_back();
	MotorController::last_interval_cycle = true;
	if (MotorController::get_water_level() >= MotorController::min_water_level || MotorController::outvalve_status)
	{
		MotorController::empty_water = false;
		MotorController::full_water = true;
	}
	else
	{
		MotorController::empty_water = true;
		MotorController::full_water = false;
	}
	
	return MotorController::active_status;
}

bool MotorController::setup_port(int motor_activation, int motor_direction, int inlet_valve, int outlet_valve, int water_level_sensor, int timer_sensor_SDA , int timer_sensor_SCL )
{
	MotorController::data = MotorController::file->read_data();
	if (!motor_activation)
	{
		MotorController::setup((*data)["motor_activation"], (*data)["motor_direction"], (*data)["inlet_valve"], (*data)["outlet_valve"], (*data)["water_level_sensor"], (*data)["timer_sensor_SDA"], (*data)["timer_sensor_SCL"]);
		MotorController::last_nobe_value = MotorController::sensor_last_value = (*data)["last_nobe_value"];
		return true;
	}
	else
	{
		if (!motor_activation == 0)
			file->add_data("motor_activation", motor_activation);

		if (!motor_direction == 0)
			file->add_data("motor_direction", motor_direction);

		if (!inlet_valve == 0)
			file->add_data("inlet_valve", inlet_valve);

		if (!outlet_valve == 0)
			file->add_data("outlet_valve", outlet_valve);

		if (!water_level_sensor == 0)
			file->add_data("water_level_sensor", water_level_sensor);

		if (!timer_sensor_SDA == 0)
			file->add_data("timer_sensor_SDA", timer_sensor_SDA);

		if (!timer_sensor_SCL == 0)
			file->add_data("timer_sensor_SCL", timer_sensor_SCL);

		file->save_data();
		return false;
	}
	
}

void MotorController::send_data_back()
{
	if (!(MotorController::users_index < 0))
	{
		MotorController::socket->add_sent_data("client", MotorController::users_index);
		MotorController::socket->create_nested_data("operation_data");
		MotorController::socket->add_nested_sent_data("user", MotorController::current_user);
		MotorController::socket->add_nested_sent_data("remaining_time", ((unsigned int)MotorController::interval_duration - (unsigned int)(millis() / 60000) + MotorController::moter_start_time));
		MotorController::socket->add_nested_sent_data("remaining_intervals", MotorController::no_of_interval - MotorController::current_cycle - 1);
		MotorController::socket->add_nested_sent_data("current_cycle", MotorController::current_cycle);
		MotorController::socket->add_nested_sent_data("status", MotorController::active_state);
		MotorController::socket->sent_data();
	}
}

bool MotorController::setup_motor_critical_values(
	int min_water_level_val,
	int extra_value_during_outvale_val,
	int time_for_last_drain_phase_val,
	int time_for_motor_clockwise_phase_val,
	int time_for_motor_periodic_stop_phase_val,
	int time_for_motor_anticlockwise_phase_val,
	int time_for_motor_last_phase_val,
	int wait_time_for_cyclic_debug_send_val,
	int offline_interval_duration_val,
	int offlin_water_level_val,
	int sensor_level_check_time_val,
	int water_level_check_time_val
)
{
	MotorController::data = MotorController::file->read_data();

	bool should_save{ false };
	if (min_water_level_val == -1)
	{
		JsonVariant min_water_level = (*data).getMember("min_water_level");
		if (!min_water_level.isNull())
		{
			MotorController::min_water_level = min_water_level;
		}
	}
		
	else
	{

		file->add_data("min_water_level", min_water_level_val);
		MotorController::min_water_level = min_water_level_val;
		should_save = true;
	}


	if (extra_value_during_outvale_val == -1)
	{
		JsonVariant extra_value_during_outvale = (*data).getMember("extra_value_during_outvale");
		if (!extra_value_during_outvale.isNull())
		{
			MotorController::extra_value_during_outvale = extra_value_during_outvale;
		}
	}
	else
	{
		file->add_data("extra_value_during_outvale", extra_value_during_outvale_val);
		MotorController::extra_value_during_outvale = extra_value_during_outvale_val;
		should_save = true;

	}


	if (time_for_last_drain_phase_val == -1)
	{
		JsonVariant time_for_last_drain_phase = (*data).getMember("time_for_last_drain_phase");
		if (!time_for_last_drain_phase.isNull())
		{
			MotorController::time_for_last_drain_phase = time_for_last_drain_phase; // In seconds
		}
	}

	else
	{
		file->add_data("time_for_last_drain_phase", time_for_last_drain_phase_val);
		MotorController::time_for_last_drain_phase = time_for_last_drain_phase_val; // In seconds
		should_save = true;
	}


	if (time_for_motor_clockwise_phase_val == -1)
	{
		JsonVariant time_for_motor_clockwise_phase = (*data).getMember("time_for_motor_clockwise_phase");
		if (!time_for_motor_clockwise_phase.isNull())
		{
			MotorController::time_for_motor_clockwise_phase = time_for_motor_clockwise_phase; // In seconds
		}
	}
	else
	{
		file->add_data("time_for_motor_clockwise_phase", time_for_motor_clockwise_phase_val);
		MotorController::time_for_motor_clockwise_phase = time_for_motor_clockwise_phase_val; // In seconds
		should_save = true;
	}


	if (time_for_motor_periodic_stop_phase_val == -1)
	{
		JsonVariant time_for_motor_periodic_stop_phase = (*data).getMember("time_for_motor_periodic_stop_phase");
		if (!time_for_motor_periodic_stop_phase.isNull())
		{
			MotorController::time_for_motor_periodic_stop_phase = time_for_motor_periodic_stop_phase; // In seconds
		}
	}
	else
	{
		file->add_data("time_for_motor_periodic_stop_phase", time_for_motor_periodic_stop_phase_val + MotorController::time_for_motor_clockwise_phase);
		MotorController::time_for_motor_periodic_stop_phase = time_for_motor_periodic_stop_phase_val + MotorController::time_for_motor_clockwise_phase; // In seconds
		should_save = true;
	}


	if (time_for_motor_anticlockwise_phase_val == -1)
	{
		JsonVariant time_for_motor_anticlockwise_phase = (*data).getMember("time_for_motor_anticlockwise_phase");
		if (!time_for_motor_anticlockwise_phase.isNull())
		{
			MotorController::time_for_motor_anticlockwise_phase = time_for_motor_anticlockwise_phase; // In seconds
		}
	}
	else
	{
		file->add_data("time_for_motor_anticlockwise_phase", time_for_motor_anticlockwise_phase_val + MotorController::time_for_motor_periodic_stop_phase);
		MotorController::time_for_motor_anticlockwise_phase = time_for_motor_anticlockwise_phase_val + MotorController::time_for_motor_periodic_stop_phase; // In seconds
		should_save = true;
	}


	if (time_for_motor_last_phase_val == -1)
	{
		JsonVariant time_for_motor_last_phase = (*data).getMember("time_for_motor_last_phase");
		if (!time_for_motor_last_phase.isNull())
		{
			MotorController::time_for_motor_last_phase = time_for_motor_last_phase; // In seconds
		}
	}
	else
	{
		file->add_data("time_for_motor_last_phase", time_for_motor_last_phase_val + MotorController::time_for_motor_anticlockwise_phase);
		MotorController::time_for_motor_last_phase = time_for_motor_last_phase_val + MotorController::time_for_motor_anticlockwise_phase; // In seconds
		should_save = true;
	}
	if (wait_time_for_cyclic_debug_send_val == -1)
	{
		JsonVariant wait_time_for_cyclic_debug_send = (*data).getMember("wait_time_for_cyclic_debug_send");
		if (!wait_time_for_cyclic_debug_send.isNull())
		{
			MotorController::wait_time_for_cyclic_debug_send = wait_time_for_cyclic_debug_send; // In miliseconds
		}
	}
	else
	{
		file->add_data("wait_time_for_cyclic_debug_send", wait_time_for_cyclic_debug_send_val);
		MotorController::wait_time_for_cyclic_debug_send = wait_time_for_cyclic_debug_send_val; // In seconds
		should_save = true;
	}


	if (offline_interval_duration_val == -1)
	{
		JsonVariant offline_interval_duration = (*data).getMember("offline_interval_duration");
		if (!offline_interval_duration.isNull())
		{
			MotorController::offline_interval_duration = offline_interval_duration; 
		}
	}
	else
	{
		file->add_data("offline_interval_duration", offline_interval_duration_val);
		MotorController::offline_interval_duration = offline_interval_duration_val; 
		should_save = true;
	}


	if (offlin_water_level_val == -1)
	{
		JsonVariant offlin_water_level = (*data).getMember("offlin_water_level");
		if (!offlin_water_level.isNull())
		{
			MotorController::offlin_water_level = offlin_water_level;
		}
	}
	else
	{
		file->add_data("offlin_water_level", offlin_water_level_val);
		MotorController::offlin_water_level = offlin_water_level_val;
		should_save = true;
	}


	if (sensor_level_check_time_val == -1)
	{
		JsonVariant sensor_level_check_time = (*data).getMember("sensor_level_check_time");
		if (!sensor_level_check_time.isNull())
		{
			MotorController::sensor_level_check_time = sensor_level_check_time;
		}
	}
	else
	{
		file->add_data("sensor_level_check_time", sensor_level_check_time_val);
		MotorController::sensor_level_check_time = sensor_level_check_time_val; 
		should_save = true;
	}


	if (water_level_check_time_val == -1)
	{
		JsonVariant water_level_check_time = (*data).getMember("water_level_check_time");
		if (!water_level_check_time.isNull())
		{
			MotorController::water_level_check_time = water_level_check_time;
		}
	}
	else
	{

		file->add_data("water_level_check_time", water_level_check_time_val);
		MotorController::water_level_check_time = water_level_check_time_val;
	}



	if (should_save)
	{
		file->save_data();

		return true;
	}
	else {
		return false;
	}
}

