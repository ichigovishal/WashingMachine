#ifndef OPEN_FILE_H
#define OPEN_FILE_H

#include <FS.h>
#include <ArduinoJson.h>
#include <vector>

class open_file
{
private:
	static ArduinoJson::DynamicJsonDocument data;
	
public:
	static ArduinoJson::DynamicJsonDocument buffer;
	open_file();
	~open_file();
    void add_data(const char* name, const char* data_value);
	void add_data(const char* name, bool data_value);
	void add_data(const char* name, int data_value);
	void add_data(const char* name, std::vector<char*> &data_value);
	static ArduinoJson::DynamicJsonDocument *read_data();
	static void save_data();

};
#endif //OPEN_FILE_H
