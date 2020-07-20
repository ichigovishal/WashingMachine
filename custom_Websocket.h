

#ifndef custom_Websocket_H
#define custom_Websocket_H

#include "open_file.h"
#include <ArduinoWebsockets.h>
#include <string>
#include <ESP8266WiFi.h>
#include <vector>
#include <iterator>
#include "webPrint.h"

class custom_Websocket
{	
private:
	static ArduinoJson::DynamicJsonDocument data;
	static ArduinoJson::DynamicJsonDocument buffer;
	static void webSocketEvents(websockets::WebsocketsClient &client, websockets::WebsocketsMessage message);
	static std::vector<websockets::WebsocketsClient> Clients;
	static void pollAllClients();
	static void Events_Listener(websockets::WebsocketsClient& client, websockets::WebsocketsEvent event, String payload);
	static void add_index(int num);
	static ArduinoJson::JsonObject objects;
public:
	static void loop();
	static bool received;
	static websockets::WebsocketsServer server;
	static unsigned int num_user;

	custom_Websocket();
	~custom_Websocket();
	void create_nested_data(const char* name);
	void add_sent_data(const char* name, const char* data);
	void add_sent_data(const char* name, const int data);
	void add_sent_data(const char* name, String data);
	void add_sent_data(const char* name, bool data);
	void add_sent_data(const char* name, std::vector<char*>& data_value);
	void add_sent_data(const char* name, const unsigned int data);
	void add_nested_sent_data(const char* name, const char* data);
	void add_nested_sent_data(const char* name, const int data);
	void add_nested_sent_data(const char* name, String data);
	void add_nested_sent_data(const char* name, bool data);
	void add_nested_sent_data(const char* name, std::vector<char*>& data_value);
	void add_nested_sent_data(const char* name, const unsigned int data);
	static void sent_data();
	ArduinoJson::DynamicJsonDocument* get_data();
	
	

};
#endif //JSON_H
