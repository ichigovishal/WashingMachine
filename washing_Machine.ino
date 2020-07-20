 #include "wifi.h"
#include "MotorController.h"

custom_Websocket* socket;
open_file* file;
MotorController* controller;
bool send_back_state{ false };
void setup() {

	Serial.begin(115200);
	socket = new custom_Websocket;
	file = new open_file;
	wifi::main(socket, file);
	controller = new MotorController{ file, socket };
	
}

void send_back_loop(){
	if (send_back_state)
	{
		controller->waited_send_debug();
	}
}
void loop()
{
	socket->loop();
	controller->loop();
	send_back_loop();
	if (socket->num_user > 0 && socket->received)
	{
		// Get Data from socket
		ArduinoJson::DynamicJsonDocument* const data{ socket->get_data() };
		// To make sure user does not disconnect
		if (controller->active_status && controller->current_user == (*data)["user"].as<char*>() && (!(*data)["client"].as<int>() == controller->users_index))
		{
			controller->users_index = (*data)["client"].as<int>();
		}

		if ((*data)["type"].as<int>() == 101)// Change Port
		{
			JsonVariant motor_active_port = (*data)["motor_active_port"];
			JsonVariant motor_direction_port = (*data)["motor_direction_port"];
			JsonVariant inlet_valve_port = (*data)["inlet_valve_port"];
			JsonVariant outlet_valve_port = (*data)["outlet_valve_port"];
			JsonVariant water_sensor_port = (*data)["water_sensor_port"];
			JsonVariant sda_port = (*data)["sda_port"];
			JsonVariant scl_port = (*data)["scl_port"];
			if (!motor_active_port.isNull() && !motor_direction_port.isNull() && !inlet_valve_port.isNull() && !outlet_valve_port.isNull() && !water_sensor_port.isNull() && !sda_port.isNull() && !scl_port.isNull())
			{
				bool status = controller->setup_port(motor_active_port.as<int>(), motor_direction_port.as<int>(), inlet_valve_port.as<int>(), outlet_valve_port.as<int>(), water_sensor_port.as<int>(), sda_port.as<int>(), scl_port.as<int>());
				if (status)
				{
					socket->add_sent_data("status", true);
					socket->add_sent_data("client", (*data)["client"].as<int>());
					socket->sent_data();
				}
				else
				{
					socket->add_sent_data("error", "Ports value cannot be saved.");
					socket->add_sent_data("status", false);
					socket->add_sent_data("client", (*data)["client"].as<int>());
					socket->sent_data();
				}
			}
			else
			{
				socket->add_sent_data("error", "Value missing.");
				socket->add_sent_data("status", false);
				socket->add_sent_data("client", (*data)["client"].as<int>());
				socket->sent_data();
			}

		}
		else if ((*data)["type"].as<int>() == 102) // Active Request
		{
			JsonVariant user = (*data)["user"];
			JsonVariant users_index = (*data)["client"];
			JsonVariant interval_duration = (*data)["interval_duration"];
			JsonVariant interval_time = (*data)["interval_time"];
			JsonVariant needed_water_level = (*data)["needed_water_level"];
			if (!user.isNull() && !interval_duration.isNull() && !interval_time.isNull() && !needed_water_level.isNull())
			{
				if (interval_duration.as<int>() > 15)
				{
					interval_duration.set(15);
				}
				else if (interval_duration.as<int>() < 5)
				{
					interval_duration.set(5);
				}

				if (interval_time.as<int>() > 3)
				{
					interval_time.set(3);
				}

				if (needed_water_level.as<int>() > 3)
				{
					interval_time.set(3);
				}

				bool status = controller->activate(user.as<char*>(), users_index.as<int>(), interval_duration.as<int>(), interval_time.as<int>(), needed_water_level.as<int>());
				if (status)
				{
					send_back_state = false;
					socket->add_sent_data("status", true);

				}
				else
				{
					socket->add_sent_data("error", "Someone else is using it.");
					socket->add_sent_data("status", false);
				}
				socket->create_nested_data("operation_data");
				socket->add_nested_sent_data("user", controller->current_user);
				socket->add_nested_sent_data("users_index", controller->users_index);
				socket->add_nested_sent_data("remaining_intervals", controller->remaining_interval_duration);
				socket->add_nested_sent_data("remaining_time", controller->remaining_interval_time);
				socket->add_nested_sent_data("client", (*data)["client"].as<int>());
				socket->sent_data();

			}
			else
			{
				socket->add_sent_data("error", "Value missing.");
				socket->add_sent_data("status", false);
				socket->add_sent_data("client", (*data)["client"].as<int>());
				socket->sent_data();
			}

		}
		else if ((*data)["type"].as<int>() == 103)
		{
			JsonVariant state = (*data)["state"];
			if (!state.isNull())
			{
				controller->debug_on(state);
				socket->add_sent_data("status", state.as<bool>());
			}
			else
			{
				socket->add_sent_data("error", "Value missing.");
				socket->add_sent_data("status", false);
			}

			socket->add_sent_data("client", (*data)["client"].as<int>());
			socket->sent_data();
		}
		else if ((*data)["type"].as<int>() == 104) // Deactive Request
		{
			JsonVariant user = (*data)["user"];
			JsonVariant reason = (*data)["reason"];
			if (!user.isNull() && !reason.isNull())
			{
				if (user == controller->current_user)
				{
					bool status = controller->deactivate(reason.as<char*>());
					socket->add_sent_data("status", status);
				}
				else
				{
					socket->add_sent_data("error", "You cannot cancel someone else operation.");
					socket->add_sent_data("status", false);
				}
			}
		    else
		    {
			    socket->add_sent_data("error", "Value missing.");
			    socket->add_sent_data("status", false);
			    
		    }
			socket->add_sent_data("client", (*data)["client"].as<int>());
			socket->sent_data();
		}
		else if ((*data)["type"].as<int>() == 105) // Change operation values
		{
		JsonVariant user = (*data)["user"];
		JsonVariant min_water_level = (*data)["min_water_level"];
		JsonVariant extra_value_during_outvale = (*data)["extra_value_during_outvale"];
		JsonVariant time_for_last_drain_phase = (*data)["time_for_last_drain_phase"];
		JsonVariant time_for_motor_clockwise_phase = (*data)["time_for_motor_clockwise_phase"];
		JsonVariant time_for_motor_periodic_stop_phase = (*data)["time_for_motor_periodic_stop_phase"];
		JsonVariant time_for_motor_anticlockwise_phase = (*data)["time_for_motor_anticlockwise_phase"];
		JsonVariant time_for_motor_last_phase = (*data)["time_for_motor_last_phase"];
		JsonVariant wait_time_for_cyclic_debug_send = (*data)["wait_time_for_cyclic_debug_send"];
		JsonVariant offline_interval_duration = (*data)["offline_interval_duration"];
		JsonVariant offlin_water_level = (*data)["offlin_water_level"];
		JsonVariant sensor_level_check_time = (*data)["sensor_level_check_time"];
		JsonVariant water_level_check_time = (*data)["water_level_check_time"];

		int min_water_level_val;
		int extra_value_during_outvale_val;
		int time_for_last_drain_phase_val;
		int time_for_motor_clockwise_phase_val;
		int time_for_motor_periodic_stop_phase_val;
		int time_for_motor_anticlockwise_phase_val;
		int time_for_motor_last_phase_val;
		int wait_time_for_cyclic_debug_send_val;
		int offline_interval_duration_val;
		int offlin_water_level_val;
		int sensor_level_check_time_val;
		int water_level_check_time_val;

		if (!user.isNull())
		{
			if (!min_water_level.isNull())
			{
				min_water_level_val = min_water_level;
			}
			else
				min_water_level_val = -1;
	

			if (!extra_value_during_outvale.isNull())
			{
				extra_value_during_outvale_val = extra_value_during_outvale;
			}
			else
				extra_value_during_outvale_val = -1;
		

			if (!time_for_last_drain_phase.isNull())
			{
				time_for_last_drain_phase_val = time_for_last_drain_phase;
			}
			else
				time_for_last_drain_phase_val = -1;
			

			if (!time_for_motor_clockwise_phase.isNull())
			{
				time_for_motor_clockwise_phase_val = time_for_motor_clockwise_phase;
			}
			else
				time_for_motor_clockwise_phase_val = -1;
		
			if (!time_for_motor_periodic_stop_phase.isNull())
			{
				time_for_motor_periodic_stop_phase_val = time_for_motor_periodic_stop_phase;
			}
			else
				time_for_motor_periodic_stop_phase_val = -1;
		

			if (!time_for_motor_anticlockwise_phase.isNull())
			{
				time_for_motor_anticlockwise_phase_val = time_for_motor_anticlockwise_phase;
			}
			else
				time_for_motor_anticlockwise_phase_val = -1;
	

			if (!time_for_motor_last_phase.isNull())
			{
				time_for_motor_last_phase_val = time_for_motor_last_phase;
			}
			else
				time_for_motor_last_phase_val = -1;
	
			if (!wait_time_for_cyclic_debug_send.isNull())
			{
				wait_time_for_cyclic_debug_send_val = wait_time_for_cyclic_debug_send;
			}
			else
				wait_time_for_cyclic_debug_send_val = -1;

			if (!offline_interval_duration.isNull())
			{
				offline_interval_duration_val = offline_interval_duration;
			}
			else
				offline_interval_duration_val = -1;

			if (!offlin_water_level.isNull())
			{
				offlin_water_level_val = offlin_water_level;
			}
			else
				offlin_water_level_val = -1;

			if (!sensor_level_check_time.isNull())
			{
				sensor_level_check_time_val = sensor_level_check_time;
			}
			else
				sensor_level_check_time_val = -1;

			if (!water_level_check_time.isNull())
			{
				water_level_check_time_val = water_level_check_time;
			}
			else
				water_level_check_time_val = -1;
			
			bool status = controller->setup_motor_critical_values(
				min_water_level_val,
				extra_value_during_outvale_val,
				time_for_last_drain_phase_val,
				time_for_motor_clockwise_phase_val,
				time_for_motor_periodic_stop_phase_val,
				time_for_motor_anticlockwise_phase_val,
				time_for_motor_last_phase_val,
				wait_time_for_cyclic_debug_send_val,
				offline_interval_duration_val,
				offlin_water_level_val,
				sensor_level_check_time_val,
				water_level_check_time_val
				);
			socket->add_sent_data("status", status);

		}
		else
		{
			socket->add_sent_data("error", "Value missing.");
			socket->add_sent_data("status", false);

		}
		socket->add_sent_data("client", (*data)["client"].as<int>());
		socket->sent_data();
		}
		else if ((*data)["type"].as<int>() == 104) // Deactive Request
		{
		JsonVariant user = (*data)["user"];
		JsonVariant reason = (*data)["reason"];
		if (!user.isNull() && !reason.isNull())
		{
			if (user == controller->current_user)
			{
				bool status = controller->deactivate(reason.as<char*>());
				socket->add_sent_data("status", status);
			}
			else
			{
				socket->add_sent_data("error", "You cannot cancel someone else operation.");
				socket->add_sent_data("status", false);
			}
		}
		else
		{
			socket->add_sent_data("error", "Value missing.");
			socket->add_sent_data("status", false);

		}
		socket->add_sent_data("client", (*data)["client"].as<int>());
		socket->sent_data();
		}
		else if ((*data)["type"].as<int>() == 106) // Remote control
		{
		JsonVariant user = (*data)["user"];
		JsonVariant turn_off = (*data)["turn_off"];
		JsonVariant turn_on_inlet = (*data)["turn_on_inlet"];
		JsonVariant turn_off_inlet = (*data)["turn_off_inlet"];
		JsonVariant turn_off_outlet = (*data)["turn_off_outlet"];
		JsonVariant turn_on_outlet = (*data)["turn_on_outlet"];
		JsonVariant turn_on = (*data)["turn_on"];
		JsonVariant turn_clockwise = (*data)["turn_clockwise"];
		JsonVariant turn_anticlockwise = (*data)["turn_anticlockwise"];
		JsonVariant reset = (*data)["reset"];
		JsonVariant send_back = (*data)["send_back"];



		if (!user.isNull())
		{
			bool status{ false };
			if ((!turn_off.isNull()) && turn_off.as<bool>())
			{
				controller->turn_off();
				status = true;
			}


			if ((!turn_on_inlet.isNull()) && turn_on_inlet.as<bool>())
			{
				controller->turn_on_inlet();
				status = true;
			}


			if ((!turn_off_inlet.isNull()) && turn_off_inlet.as<bool>())
			{
				controller->turn_off_inlet();
				status = true;
			}

			if ((!turn_on_outlet.isNull()) && turn_on_outlet.as<bool>())
			{
				controller->turn_on_outlet();
				status = true;
			}

			if ((!turn_off_outlet.isNull()) && turn_off_outlet.as<bool>())
			{
				controller->turn_off_outlet();
				status = true;
			}


			if ((!turn_on.isNull()) && turn_on.as<bool>())
			{
				controller->turn_on();
				status = true;
			}

			if ((!turn_clockwise.isNull()) && turn_clockwise.as<bool>())
			{
				controller->turn_clockwise();
				status = true;
			}


			if ((!turn_anticlockwise.isNull()) && turn_anticlockwise.as<bool>())
			{
				controller->turn_anticlockwise();
				status = true;
			}


			if ((!reset.isNull()) && reset.as<bool>())
			{
				ESP.restart();
				status = true;
			}

			if ((!send_back.isNull()) && send_back.as<bool>())
			{
				send_back_state = true;
				status = true;
				controller->debug_on(true);
			}
			else if ((!send_back.isNull()) && !send_back.as<bool>())
			{
				send_back_state = false;
				status = false;
				controller->debug_on(false);
			}

			socket->add_sent_data("status", status);

		}
		else
		{
			socket->add_sent_data("error", "Value missing.");
			socket->add_sent_data("status", false);

		}
		socket->add_sent_data("client", (*data)["client"].as<int>());
		socket->sent_data();
		}
	}
}
