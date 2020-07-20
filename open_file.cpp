#include "open_file.h"
#include "FS.h"

ArduinoJson::DynamicJsonDocument open_file::buffer(2048);
ArduinoJson::DynamicJsonDocument open_file::data(2048);


open_file::open_file()
{
	SPIFFS.begin();
}
open_file::~open_file()
{
	SPIFFS.end();
}

void open_file::add_data(const char* name, const char* data_value)
{

	JsonVariant variant = this->buffer.getOrAddMember(name);
	variant.set(data_value);
}

void open_file::add_data(const char* name, bool data_value)
{
	JsonVariant variant = this->buffer.getOrAddMember(name);
	variant.set(data_value);
}

void open_file::add_data(const char* name, int data_value)
{
	JsonVariant variant = this->buffer.getOrAddMember(name);
	variant.set(data_value);
}

void open_file::add_data(const char* name, std::vector<char*> &data_value)
{
	JsonArray arr = this->buffer.createNestedArray(name);
	for (auto i: data_value)
	{
		arr.add(i);
	}
	data_value.clear();
}

ArduinoJson::DynamicJsonDocument *open_file::read_data()
{
	File file = SPIFFS.open("/config.json", "r+");
	
	if (!file) {
		Serial.println("File cannot be open for a reading data.");
		open_file::data["quality"] = false;
	}
	else {
		ArduinoJson::deserializeJson(open_file::data, file);
		 open_file::buffer = open_file::data;
		open_file::data["quality"] = true;
		Serial.println("File for reading data, has been opened.");
	}
	file.close();
	return &(open_file::data);
}

void open_file::save_data()
{

	// Open file for writing
	File file = SPIFFS.open("/config.json", "w+");
	if (!file) {
		Serial.println("File cannot be open for a writing data.");
		return;
	}
	ArduinoJson::serializeJson(open_file::buffer, file);
	file.close();
	Serial.println("Data has been written in file.");
}
