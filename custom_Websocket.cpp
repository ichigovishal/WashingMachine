#include "custom_Websocket.h"


void custom_Websocket::add_index(int num)
{
	custom_Websocket::data["client"] = num;
}


void custom_Websocket::pollAllClients() {
	for (websockets::WebsocketsClient& client : custom_Websocket::Clients) {
		client.poll();
	}
}

void custom_Websocket::Events_Listener(websockets::WebsocketsClient& client, websockets::WebsocketsEvent event, String payload)
{
	switch (event) {
	case websockets::WebsocketsEvent::ConnectionOpened:
		custom_Websocket::num_user++;
		Serial.println("Client is connected.");
		break;
	case websockets::WebsocketsEvent::GotPing:
		// Dispatched when a ping frame arrives
		break;
	case websockets::WebsocketsEvent::GotPong:
		// Dispatched when a pong frame arrives
		break;
	case websockets::WebsocketsEvent::ConnectionClosed:
		// Dispatched when the connection is closed (either 
		// by the user or after some error or event)
		Serial.println("Client is disconnected.");
		--custom_Websocket::num_user;
		break;
	}
}

void  custom_Websocket::webSocketEvents(websockets::WebsocketsClient& client, websockets::WebsocketsMessage message)
{
	
	if(message.isText())
	{
		ArduinoJson::deserializeJson(custom_Websocket::data, message.data());
		custom_Websocket::received = true;
	}
}
 ArduinoJson::JsonObject custom_Websocket::objects;
 websockets::WebsocketsServer custom_Websocket::server;
 ArduinoJson::DynamicJsonDocument custom_Websocket::data(1024);
 ArduinoJson::DynamicJsonDocument custom_Websocket::buffer(1024);
 bool custom_Websocket::received{ false };
 unsigned int custom_Websocket::num_user{ 0 };
 std::vector<websockets::WebsocketsClient> custom_Websocket::Clients{};

 void custom_Websocket::loop()
 {
	 if(custom_Websocket::server.available()) {

		 // if there is a client that wants to connect
		 if (custom_Websocket::server.poll()) {
			 //accept the connection and register callback
			 Serial.print("Accepting a new client!");
			 Serial.println();
		     websockets::WebsocketsClient client = custom_Websocket::server.accept();
			 client.onMessage(custom_Websocket::webSocketEvents);
			 client.onEvent(custom_Websocket::Events_Listener);
			 custom_Websocket::Clients.push_back(client);
			 int num = custom_Websocket::Clients.size();
			 custom_Websocket::add_index(num);
			 custom_Websocket::num_user++;
		 }

		 // check for updates in all clients
		 custom_Websocket::pollAllClients(); 
	 }
 }

 custom_Websocket::custom_Websocket()
 {
	 this->server.listen(8080);
 }

 custom_Websocket::~custom_Websocket()
 {
	 
 }

 void custom_Websocket::create_nested_data(const char* name)
 {
	 this->objects = this->buffer.createNestedObject(name);
 }


void custom_Websocket::add_sent_data(const char* name, const char* data)
{
	this->buffer[name] = data;
}

void custom_Websocket::add_sent_data(const char* name, const unsigned int data)
{
	this->buffer[name] = data;
}

void custom_Websocket::add_nested_sent_data(const char* name, const char* data)
{
	this->objects[name] = data;
}

void custom_Websocket::add_nested_sent_data(const char* name, const int data)
{
	this->objects[name] = data;
}

void custom_Websocket::add_nested_sent_data(const char* name, String data)
{
	this->objects[name] = data;
}

void custom_Websocket::add_nested_sent_data(const char* name, bool data)
{
	this->objects[name] = data;
}

void custom_Websocket::add_nested_sent_data(const char* name, std::vector<char*>& data_value)
{

	JsonArray arr = this->objects.createNestedArray(name);
	for (auto i : data_value)
	{
		arr.add(i);
	}
	data_value.clear();
}

void custom_Websocket::add_nested_sent_data(const char* name, const unsigned int data)
{
	this->objects[name] = data;
}

void custom_Websocket::add_sent_data(const char* name, const int data)
{
	this->buffer[name] = data;
}

void custom_Websocket::add_sent_data(const char* name,  bool data)
{
	this->buffer[name] = data;
}
void custom_Websocket::add_sent_data(const char* name, String data)
{
	this->buffer[name] = data;
}
void custom_Websocket::add_sent_data(const char* name, std::vector<char*>& data_value)
{
	JsonArray arr = this->buffer.createNestedArray(name);
	for (auto i : data_value)
	{
		arr.add(i);
	}
	data_value.clear();
}
void custom_Websocket::sent_data()
{
	if (!custom_Websocket::buffer["client"].as<int>())
	{
		for (auto& i: custom_Websocket::Clients)
		{
			webPrint webprint{ &i };
			ArduinoJson::serializeJson(custom_Websocket::buffer, webprint);
		}
	}
	else
	{
		int client = custom_Websocket::buffer["client"].as<int>();
		custom_Websocket::buffer.remove("client");
		webPrint webprint{ &(custom_Websocket::Clients[client]) };
		ArduinoJson::serializeJson(custom_Websocket::buffer, webprint);
	}
	custom_Websocket::objects.clear();
	custom_Websocket::buffer.clear();
}

ArduinoJson::DynamicJsonDocument* custom_Websocket::get_data()
{
	custom_Websocket::received = false;
	return &custom_Websocket::data;
}


