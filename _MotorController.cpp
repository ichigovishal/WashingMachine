#include "_MotorController.h"
#include "Wire.h"
//#include "PCF8591.h"
byte ana0 , ana1, ana2, ana3;
int PCF8591_I2C_ADDRESS = 0x48; // I2C bus address
//PCF8591* pcf8591;
int _MotorController::water_level_check_time{ 3000 };
int _MotorController::sensor_level_check_time{3000 };

custom_Websocket* _MotorController::socket;
bool _MotorController::debug{ false };

bool _MotorController::motor_active_status{ false };

bool _MotorController::motor_anticlockwise_status{ false };

bool _MotorController::motor_clockwise_status{ false };

bool _MotorController::motor_periodic_stop{ false };

// Invalve Port
bool _MotorController::invalve_status{ false };

// OutValve Port
bool _MotorController::outvalve_status{ false };

int _MotorController::water_level_sensor_port;
int _MotorController::timer_SDA_port;
int _MotorController::timer_SCL_port;
int _MotorController::motor_anticlockwise_port;
int _MotorController::motor_clockwise_port;
int _MotorController::invalve_port;
int _MotorController::outvalve_port;

// Motor critical values
int  _MotorController::min_water_level{0};
int  _MotorController::extra_value_during_outvale{ 0 };
int  _MotorController::time_for_last_drain_phase{ 0 }; // In seconds
int  _MotorController::time_for_motor_clockwise_phase{ 0 };// In seconds
int  _MotorController::time_for_motor_periodic_stop_phase{ 0 };// In seconds
int  _MotorController::time_for_motor_anticlockwise_phase{ 0 };// In seconds
int  _MotorController::time_for_motor_last_phase{ 0 };// In seconds
int  _MotorController::wait_time_for_cyclic_debug_send{ 0 };// In miliseconds
int  _MotorController::offline_interval_duration{ 0 };
int  _MotorController::offlin_water_level{ 0 };
int _MotorController::water_level_sensor_original_value{ 0 };
int _MotorController::water_level_sensor_negate_value{ 111 };

unsigned int persion;
void _MotorController::setup(int motor_anticlockwise, int motor_clockwise, int inlet_valve, int outlet_valve, int water_level_sensor_port_val, int timer_sensor_SDA_val, int timer_sensor_SCL_val)
{
	// a
	_MotorController::water_level_sensor_port = water_level_sensor_port_val ;
	_MotorController::timer_SDA_port = timer_sensor_SDA_val ;
	_MotorController::timer_SCL_port = timer_sensor_SCL_val;
	_MotorController::motor_anticlockwise_port = motor_anticlockwise;
	_MotorController::motor_clockwise_port = motor_clockwise;
	_MotorController::invalve_port = inlet_valve ;
	_MotorController::outvalve_port = outlet_valve;
	

	//Motor Controller
	pinMode(motor_anticlockwise, OUTPUT);
	pinMode(motor_clockwise, OUTPUT);
	digitalWrite(motor_anticlockwise, 0);
	digitalWrite(motor_clockwise, 0);

	//Water Pump
	pinMode(inlet_valve, OUTPUT);
	pinMode(outlet_valve, OUTPUT);
	digitalWrite(inlet_valve, 0);
	digitalWrite(outlet_valve, 0);

	//Timer Senser
	Wire.pins(_MotorController::timer_SDA_port, _MotorController::timer_SCL_port);// just to make sure
	Wire.begin(_MotorController::timer_SDA_port, _MotorController::timer_SCL_port);// the SDA and SCL
	//Wire.pins(4, 5);// just to make sure
	//Wire.begin(4, 5);// the SDA and SCL
	//pcf8591 = new PCF8591(PCF8591_I2C_ADDRESS, _MotorController::timer_SDA_port, _MotorController::timer_SCL_port);
	//pcf8591->begin();
	
	//Water level
	for (size_t i = 0; i <= water_level_check_time; i++)
	{
		_MotorController::get_water_level();
	}
}

int _MotorController::get_water_level()
{
	static int times{ 0 };
	static int value{ 0 };
	static int last_value{ 0 };
	if (times == water_level_check_time)
	{
		times = 0;
		last_value = value / water_level_check_time;
		_MotorController::water_level_sensor_original_value = last_value;
		value = 0;

	}
	else
	{
		value += analogRead(_MotorController::water_level_sensor_port);
		times++;
	}
	return (int)last_value - _MotorController::water_level_sensor_negate_value;
	//return analogRead(_MotorController::water_level_sensor_port);
}

int _MotorController::sensor_last_value{ 0 };

int _MotorController::get_sensor()
{	
	static int times{ 0 };
	static int value{ 0 };
	Wire.beginTransmission(PCF8591_I2C_ADDRESS); // wake up PCF8591
	Wire.write(0x00); // control byte: reads ADC0 then auto-increment
	Wire.endTransmission(); // end tranmission
	Wire.requestFrom(PCF8591_I2C_ADDRESS, 1);
	ana0 = Wire.read();
	//ana0 = Wire.read();
	

	if (times == sensor_level_check_time)
	{
		times = 0;
		sensor_last_value = value / sensor_level_check_time;
		value = 0;

	}
	else
	{

		value += (((float)ana0) / 255.00) * 15.00;
		times++;
	}
	return (int)sensor_last_value;
	//return ana0;
	
}

void _MotorController::turn_on()
{
	_MotorController::send_debug("Turning On Motor.");
	_MotorController::motor_active_status = true;
}

void _MotorController::turn_clockwise()
{
	_MotorController::send_debug("Turning On Clockwise Motor Rotation.");
	digitalWrite(motor_clockwise_port, 1);
	_MotorController::motor_clockwise_status = true;
	_MotorController::motor_periodic_stop = false;
}

void _MotorController::turn_anticlockwise()
{
	_MotorController::send_debug("Turning On Anticlockwise Motor Rotation.");
	digitalWrite(motor_anticlockwise_port, 1);
	_MotorController::motor_anticlockwise_status = true;
	_MotorController::motor_periodic_stop = false;
}

void _MotorController::turn_periodic_on()	
{
	_MotorController::motor_periodic_stop = true;
	_MotorController::send_debug("Turning On 'periodic Motor Rotation off'.");
	digitalWrite(motor_anticlockwise_port, 0);
	_MotorController::motor_anticlockwise_status = false;
	digitalWrite(motor_clockwise_port, 0);
	_MotorController::motor_clockwise_status = false;
}

void _MotorController::turn_off()
{
	_MotorController::send_debug("Turning Off Motor.");

	digitalWrite(motor_anticlockwise_port, 0);

	_MotorController::motor_anticlockwise_status = false;

	digitalWrite(motor_clockwise_port, 0);

	_MotorController::motor_clockwise_status = false;

	_MotorController::motor_active_status = false;

	_MotorController::motor_periodic_stop = false;
}

void _MotorController::turn_on_inlet()
{
	_MotorController::send_debug("Turning On Water Inlet.");
	digitalWrite(invalve_port, 1);
	_MotorController::invalve_status = true;
}

void _MotorController::turn_off_inlet()
{
	_MotorController::send_debug("Turning Off Water Inlet.");
	digitalWrite(invalve_port, 0);
	_MotorController::invalve_status = false;
}

void _MotorController::turn_on_outlet()
{
	_MotorController::send_debug("Turning On Water Outlet.");
	digitalWrite(outvalve_port, 1);
	_MotorController::outvalve_status = true;
}

void _MotorController::turn_off_outlet()
{

	_MotorController::send_debug("Turning Off Water Outlet.");
	digitalWrite(outvalve_port, 0);
	_MotorController::outvalve_status = false;
}

void _MotorController::send_debug(char* data)
{
	Serial.println(data);
	if (socket->num_user > 0 && _MotorController::debug)
	{
		socket->create_nested_data("debug_data");
		socket->add_nested_sent_data("invale_status", _MotorController::invalve_status);
		socket->add_nested_sent_data("outvale_status", _MotorController::outvalve_status);
		socket->add_nested_sent_data("motor_status", _MotorController::motor_active_status);
		socket->add_nested_sent_data("motor_clockwise_status", _MotorController::motor_clockwise_status);
		socket->add_nested_sent_data("motor_anticlockwise_status", _MotorController::motor_anticlockwise_status);
		socket->add_nested_sent_data("water_level", _MotorController::get_water_level());
		socket->add_nested_sent_data("water_level_sensor_original_value", _MotorController::water_level_sensor_original_value);
		socket->add_nested_sent_data("water_level_sensor_negate_value", _MotorController::water_level_sensor_negate_value);
		socket->add_nested_sent_data("timer_SDA_port", _MotorController::timer_SDA_port);
		socket->add_nested_sent_data("timer_SCL_port", _MotorController::timer_SCL_port);
		socket->add_nested_sent_data("invalve_port", _MotorController::invalve_port);
		socket->add_nested_sent_data("outvalve_port", _MotorController::outvalve_port);
		socket->add_nested_sent_data("motor_clockwise_port", _MotorController::motor_clockwise_port);
		socket->add_nested_sent_data("motor_anticlockwise_port", _MotorController::motor_anticlockwise_port);
		socket->add_nested_sent_data("min_water_level", _MotorController::min_water_level);
		socket->add_nested_sent_data("extra_value_during_outvale", _MotorController::extra_value_during_outvale);
		socket->add_nested_sent_data("time_for_last_drain_phase", _MotorController::time_for_last_drain_phase);
		socket->add_nested_sent_data("time_for_motor_clockwise_phase", _MotorController::time_for_motor_clockwise_phase);
		socket->add_nested_sent_data("time_for_motor_periodic_stop_phase", _MotorController::time_for_motor_periodic_stop_phase);
		socket->add_nested_sent_data("time_for_motor_anticlockwise_phase", _MotorController::time_for_motor_anticlockwise_phase);
		socket->add_nested_sent_data("time_for_motor_last_phase", _MotorController::time_for_motor_last_phase);
		socket->add_nested_sent_data("wait_time_for_cyclic_debug_send", _MotorController::wait_time_for_cyclic_debug_send);
		socket->add_nested_sent_data("offline_interval_duration", _MotorController::offline_interval_duration);
		socket->add_nested_sent_data("offlin_water_level", _MotorController::offlin_water_level);
		socket->add_nested_sent_data(" water_level_check_time", _MotorController::water_level_check_time);
		socket->add_nested_sent_data("sensor_level_check_time", _MotorController::sensor_level_check_time);
		socket->add_nested_sent_data("message", data);
		time_t now = time(nullptr);
		socket->add_nested_sent_data("time", ctime(&now));
		socket->sent_data();
	}
}



void _MotorController::send_debug(char* data, int int_data)
{
	Serial.print(data);
	Serial.println(int_data);
	if (socket->num_user > 0 && _MotorController::debug)
	{
		socket->create_nested_data("debug_data");
		socket->add_nested_sent_data("invale_status", _MotorController::invalve_status);
		socket->add_nested_sent_data("outvale_status", _MotorController::outvalve_status);
		socket->add_nested_sent_data("motor_status", _MotorController::motor_active_status);
		socket->add_nested_sent_data("motor_clockwise_status", _MotorController::motor_clockwise_status);
		socket->add_nested_sent_data("motor_anticlockwise_status", _MotorController::motor_anticlockwise_status);
		socket->add_nested_sent_data("water_level", _MotorController::get_water_level());
		socket->add_nested_sent_data("water_level_sensor_original_value", _MotorController::water_level_sensor_original_value);
		socket->add_nested_sent_data("water_level_sensor_negate_value", _MotorController::water_level_sensor_negate_value);
		socket->add_nested_sent_data("timer_SDA_port", _MotorController::timer_SDA_port);
		socket->add_nested_sent_data("timer_SCL_port", _MotorController::timer_SCL_port);
		socket->add_nested_sent_data("invalve_port", _MotorController::invalve_port);
		socket->add_nested_sent_data("outvalve_port", _MotorController::outvalve_port);
		socket->add_nested_sent_data("motor_clockwise_port", _MotorController::motor_clockwise_port);
		socket->add_nested_sent_data("motor_anticlockwise_port", _MotorController::motor_anticlockwise_port);
		socket->add_nested_sent_data("min_water_level", _MotorController::min_water_level);
		socket->add_nested_sent_data("extra_value_during_outvale", _MotorController::extra_value_during_outvale);
		socket->add_nested_sent_data("time_for_last_drain_phase", _MotorController::time_for_last_drain_phase);
		socket->add_nested_sent_data("time_for_motor_clockwise_phase", _MotorController::time_for_motor_clockwise_phase);
		socket->add_nested_sent_data("time_for_motor_periodic_stop_phase", _MotorController::time_for_motor_periodic_stop_phase);
		socket->add_nested_sent_data("time_for_motor_anticlockwise_phase", _MotorController::time_for_motor_anticlockwise_phase);
		socket->add_nested_sent_data("time_for_motor_last_phase", _MotorController::time_for_motor_last_phase);
		socket->add_nested_sent_data("wait_time_for_cyclic_debug_send", _MotorController::wait_time_for_cyclic_debug_send);
		socket->add_nested_sent_data("offline_interval_duration", _MotorController::offline_interval_duration);
		socket->add_nested_sent_data("offlin_water_level", _MotorController::offlin_water_level);
		socket->add_nested_sent_data(" water_level_check_time", _MotorController::water_level_check_time);
		socket->add_nested_sent_data("sensor_level_check_time", _MotorController::sensor_level_check_time);
		socket->add_nested_sent_data("message", data + String(int_data));
		time_t now = time(nullptr);
		socket->add_nested_sent_data("time", ctime(&now));
		socket->sent_data();
	}
}

void _MotorController::send_debug(int int_data, char* data)
{
	Serial.print(int_data);
	Serial.println(data);
	if (socket->num_user > 0 && _MotorController::debug)
	{
		socket->create_nested_data("debug_data");
		socket->add_nested_sent_data("invale_status", _MotorController::invalve_status);
		socket->add_nested_sent_data("outvale_status", _MotorController::outvalve_status);
		socket->add_nested_sent_data("motor_status", _MotorController::motor_active_status);
		socket->add_nested_sent_data("motor_clockwise_status", _MotorController::motor_clockwise_status);
		socket->add_nested_sent_data("motor_anticlockwise_status", _MotorController::motor_anticlockwise_status);
		socket->add_nested_sent_data("water_level", _MotorController::get_water_level());
		socket->add_nested_sent_data("water_level_sensor_original_value", _MotorController::water_level_sensor_original_value);
		socket->add_nested_sent_data("water_level_sensor_negate_value", _MotorController::water_level_sensor_negate_value);
		socket->add_nested_sent_data("timer_SDA_port", _MotorController::timer_SDA_port);
		socket->add_nested_sent_data("timer_SCL_port", _MotorController::timer_SCL_port);
		socket->add_nested_sent_data("invalve_port", _MotorController::invalve_port);
		socket->add_nested_sent_data("outvalve_port", _MotorController::outvalve_port);
		socket->add_nested_sent_data("motor_clockwise_port", _MotorController::motor_clockwise_port);
		socket->add_nested_sent_data("motor_anticlockwise_port", _MotorController::motor_anticlockwise_port);
		socket->add_nested_sent_data("min_water_level", _MotorController::min_water_level);
		socket->add_nested_sent_data("extra_value_during_outvale", _MotorController::extra_value_during_outvale);
		socket->add_nested_sent_data("time_for_last_drain_phase", _MotorController::time_for_last_drain_phase);
		socket->add_nested_sent_data("time_for_motor_clockwise_phase", _MotorController::time_for_motor_clockwise_phase);
		socket->add_nested_sent_data("time_for_motor_periodic_stop_phase", _MotorController::time_for_motor_periodic_stop_phase);
		socket->add_nested_sent_data("time_for_motor_anticlockwise_phase", _MotorController::time_for_motor_anticlockwise_phase);
		socket->add_nested_sent_data("time_for_motor_last_phase", _MotorController::time_for_motor_last_phase);
		socket->add_nested_sent_data("wait_time_for_cyclic_debug_send", _MotorController::wait_time_for_cyclic_debug_send);
		socket->add_nested_sent_data("offline_interval_duration", _MotorController::offline_interval_duration);
		socket->add_nested_sent_data("offlin_water_level", _MotorController::offlin_water_level);
		socket->add_nested_sent_data(" water_level_check_time", _MotorController::water_level_check_time);
		socket->add_nested_sent_data("sensor_level_check_time", _MotorController::sensor_level_check_time);
		socket->add_nested_sent_data("message", String(int_data) + data);
		time_t now = time(nullptr);
		socket->add_nested_sent_data("time", ctime(&now));
		socket->sent_data();
	}
}

void _MotorController::send_debug(const char* first_data, const char* data)
{
	char buff[strlen(first_data) + strlen(data) + 1];
	strcpy(buff, first_data);
	strcat(buff, data);
	Serial.println(buff);
	if (socket->num_user > 0 && _MotorController::debug)
	{
		socket->create_nested_data("debug_data");
		socket->add_nested_sent_data("invale_status", _MotorController::invalve_status);
		socket->add_nested_sent_data("outvale_status", _MotorController::outvalve_status);
		socket->add_nested_sent_data("motor_status", _MotorController::motor_active_status);
		socket->add_nested_sent_data("motor_clockwise_status", _MotorController::motor_clockwise_status);
		socket->add_nested_sent_data("motor_anticlockwise_status", _MotorController::motor_anticlockwise_status);
		socket->add_nested_sent_data("water_level", _MotorController::get_water_level());
		socket->add_nested_sent_data("water_level_sensor_original_value", _MotorController::water_level_sensor_original_value);
		socket->add_nested_sent_data("water_level_sensor_negate_value", _MotorController::water_level_sensor_negate_value);
		socket->add_nested_sent_data("timer_SDA_port", _MotorController::timer_SDA_port);
		socket->add_nested_sent_data("timer_SCL_port", _MotorController::timer_SCL_port);
		socket->add_nested_sent_data("invalve_port", _MotorController::invalve_port);
		socket->add_nested_sent_data("outvalve_port", _MotorController::outvalve_port);
		socket->add_nested_sent_data("motor_clockwise_port", _MotorController::motor_clockwise_port);
		socket->add_nested_sent_data("motor_anticlockwise_port", _MotorController::motor_anticlockwise_port);
		socket->add_nested_sent_data("min_water_level", _MotorController::min_water_level);
		socket->add_nested_sent_data("extra_value_during_outvale", _MotorController::extra_value_during_outvale);
		socket->add_nested_sent_data("time_for_last_drain_phase", _MotorController::time_for_last_drain_phase);
		socket->add_nested_sent_data("time_for_motor_clockwise_phase", _MotorController::time_for_motor_clockwise_phase);
		socket->add_nested_sent_data("time_for_motor_periodic_stop_phase", _MotorController::time_for_motor_periodic_stop_phase);
		socket->add_nested_sent_data("time_for_motor_anticlockwise_phase", _MotorController::time_for_motor_anticlockwise_phase);
		socket->add_nested_sent_data("time_for_motor_last_phase", _MotorController::time_for_motor_last_phase);
		socket->add_nested_sent_data("wait_time_for_cyclic_debug_send", _MotorController::wait_time_for_cyclic_debug_send);
		socket->add_nested_sent_data("offline_interval_duration", _MotorController::offline_interval_duration);
		socket->add_nested_sent_data("offlin_water_level", _MotorController::offlin_water_level);
		socket->add_nested_sent_data(" water_level_check_time", _MotorController::water_level_check_time);
		socket->add_nested_sent_data("sensor_level_check_time", _MotorController::sensor_level_check_time);
		socket->add_nested_sent_data("message", buff);
		time_t now = time(nullptr);
		socket->add_nested_sent_data("time", ctime(&now));
		socket->sent_data();
	}
}

void _MotorController::send_debug(char data)
{
	Serial.println(data);
	if (socket->num_user > 0 && _MotorController::debug)
	{
		socket->create_nested_data("debug_data");
		socket->add_nested_sent_data("invale_status", _MotorController::invalve_status);
		socket->add_nested_sent_data("outvale_status", _MotorController::outvalve_status);
		socket->add_nested_sent_data("motor_status", _MotorController::motor_active_status);
		socket->add_nested_sent_data("motor_clockwise_status", _MotorController::motor_clockwise_status);
		socket->add_nested_sent_data("motor_anticlockwise_status", _MotorController::motor_anticlockwise_status);
		socket->add_nested_sent_data("water_level", _MotorController::get_water_level());
		socket->add_nested_sent_data("water_level_sensor_original_value", _MotorController::water_level_sensor_original_value);
		socket->add_nested_sent_data("water_level_sensor_negate_value", _MotorController::water_level_sensor_negate_value);
		socket->add_nested_sent_data("timer_SDA_port", _MotorController::timer_SDA_port);
		socket->add_nested_sent_data("timer_SCL_port", _MotorController::timer_SCL_port);
		socket->add_nested_sent_data("invalve_port", _MotorController::invalve_port);
		socket->add_nested_sent_data("outvalve_port", _MotorController::outvalve_port);
		socket->add_nested_sent_data("motor_clockwise_port", _MotorController::motor_clockwise_port);
		socket->add_nested_sent_data("motor_anticlockwise_port", _MotorController::motor_anticlockwise_port);
		socket->add_nested_sent_data("min_water_level", _MotorController::min_water_level);
		socket->add_nested_sent_data("extra_value_during_outvale", _MotorController::extra_value_during_outvale);
		socket->add_nested_sent_data("time_for_last_drain_phase", _MotorController::time_for_last_drain_phase);
		socket->add_nested_sent_data("time_for_motor_clockwise_phase", _MotorController::time_for_motor_clockwise_phase);
		socket->add_nested_sent_data("time_for_motor_periodic_stop_phase", _MotorController::time_for_motor_periodic_stop_phase);
		socket->add_nested_sent_data("time_for_motor_anticlockwise_phase", _MotorController::time_for_motor_anticlockwise_phase);
		socket->add_nested_sent_data("time_for_motor_last_phase", _MotorController::time_for_motor_last_phase);
		socket->add_nested_sent_data("wait_time_for_cyclic_debug_send", _MotorController::wait_time_for_cyclic_debug_send);
		socket->add_nested_sent_data("offline_interval_duration", _MotorController::offline_interval_duration);
		socket->add_nested_sent_data("offlin_water_level", _MotorController::offlin_water_level);
		socket->add_nested_sent_data(" water_level_check_time", _MotorController::water_level_check_time);
		socket->add_nested_sent_data("sensor_level_check_time", _MotorController::sensor_level_check_time);
		socket->add_nested_sent_data("message", data);
		time_t now = time(nullptr);
		socket->add_nested_sent_data("time", ctime(&now));
		socket->sent_data();
	}
}

void _MotorController::waited_send_debug()
{
	
		static unsigned int wait_time{ (millis() / 1000) };
		
		if (((unsigned int)((unsigned int)(millis() / 1000) - wait_time)) > _MotorController::wait_time_for_cyclic_debug_send)
		{
				if (socket->num_user > 0 && _MotorController::debug)
				{
					socket->create_nested_data("debug_data");
					socket->add_nested_sent_data("invale_status", _MotorController::invalve_status);
					socket->add_nested_sent_data("outvale_status", _MotorController::outvalve_status);
					socket->add_nested_sent_data("motor_status", _MotorController::motor_active_status);
					socket->add_nested_sent_data("motor_clockwise_status", _MotorController::motor_clockwise_status);
					socket->add_nested_sent_data("motor_anticlockwise_status", _MotorController::motor_anticlockwise_status);
					socket->add_nested_sent_data("water_level", _MotorController::get_water_level());
					socket->add_nested_sent_data("water_level_sensor_original_value", _MotorController::water_level_sensor_original_value);
					socket->add_nested_sent_data("water_level_sensor_negate_value", _MotorController::water_level_sensor_negate_value);
					socket->add_nested_sent_data("timer_SDA_port", _MotorController::timer_SDA_port);
					socket->add_nested_sent_data("timer_SCL_port", _MotorController::timer_SCL_port);
					socket->add_nested_sent_data("invalve_port", _MotorController::invalve_port);
					socket->add_nested_sent_data("outvalve_port", _MotorController::outvalve_port);
					socket->add_nested_sent_data("motor_clockwise_port", _MotorController::motor_clockwise_port);
					socket->add_nested_sent_data("motor_anticlockwise_port", _MotorController::motor_anticlockwise_port);
					socket->add_nested_sent_data("min_water_level", _MotorController::min_water_level);
					socket->add_nested_sent_data("extra_value_during_outvale", _MotorController::extra_value_during_outvale);
					socket->add_nested_sent_data("time_for_last_drain_phase", _MotorController::time_for_last_drain_phase);
					socket->add_nested_sent_data("time_for_motor_clockwise_phase", _MotorController::time_for_motor_clockwise_phase);
					socket->add_nested_sent_data("time_for_motor_periodic_stop_phase", _MotorController::time_for_motor_periodic_stop_phase);
					socket->add_nested_sent_data("time_for_motor_anticlockwise_phase", _MotorController::time_for_motor_anticlockwise_phase);
					socket->add_nested_sent_data("time_for_motor_last_phase", _MotorController::time_for_motor_last_phase);
					socket->add_nested_sent_data("offline_interval_duration", _MotorController::offline_interval_duration);
					socket->add_nested_sent_data(" water_level_check_time", _MotorController::water_level_check_time);
					socket->add_nested_sent_data("sensor_level_check_time", _MotorController::sensor_level_check_time);
					socket->add_nested_sent_data("offlin_water_level", _MotorController::offlin_water_level);
					socket->add_nested_sent_data("wait_time_for_cyclic_debug_send", _MotorController::wait_time_for_cyclic_debug_send);

					time_t now = time(nullptr);
					socket->add_nested_sent_data("time", ctime(&now));
					socket->sent_data();
				}
				wait_time = (millis() / 1000);
		}
		
}



_MotorController::_MotorController(custom_Websocket *socket_val)
{
	_MotorController::socket = socket_val;
	configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
	Serial.println("\nWaiting for time");
	while (!time(nullptr)) {
		Serial.print(".");
		delay(1000);	
	}
	
	Serial.println("");
	if (time(nullptr))
	{
		Serial.println("Connected to time server.");
	}
}