#include "wifi.h"
IPAddress local_IP(192, 168, 81, 22);
IPAddress gateway(192, 168, 81, 9);
IPAddress subnet(255, 255, 255, 0);


bool wifi::turnOn_api()
{
	Serial.print("Setting soft-AP configuration ... ");
	Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
	bool api_status = WiFi.softAP("washing_maching", "gogogogo", 12, false, 3);
	Serial.println("");
	Serial.println("Starting AP...");
	Serial.println("");
	if(api_status)
	{ 
		Serial.print("AP has been started and IP is ");
		Serial.println(WiFi.softAPIP());
		Serial.print("Connect to 'washing_maching'...");
		while (WiFi.softAPgetStationNum() < 1)
		{
			Serial.print(".");
			delay(500);
		}
		Serial.println(" ");
	}
	else
		Serial.println("AP has not been started.");
	return api_status;
}



bool wifi::connect_wifi(const char* SSID, const char* PASS)
{
	WiFi.begin(SSID, PASS);
	// connection with timeout
	int count = 0;
	Serial.print(" Connecting To '");
	Serial.print(SSID);
	Serial.print("' .");
	while ((WiFi.status() != WL_CONNECTED) && count < 17)
	{
		Serial.print(".");
		delay(500);
		count++;
	}
	Serial.println();
	if (WiFi.status() != WL_CONNECTED)
	{
		Serial.print("Failed to connect to ");
		Serial.println(SSID);
		return false;
	}
	else if (WiFi.status() == WL_CONNECTED)
	{
		Serial.print("Connected to ");
		Serial.print(SSID);
		Serial.print(" and device IP is ");
		Serial.println(WiFi.localIP());
		return true;
	}
	
}


void wifi::main(custom_Websocket* socket, open_file* file)
{
	ArduinoJson::DynamicJsonDocument* _data {file->read_data()};
	bool wifi_status = wifi::connect_wifi((*_data)["SSID"].as<const char*>(), (*_data)["PASS"].as<const char*>()); // Connect to a WiFi network
	if (!wifi_status)
	{
		bool api_status = wifi::turnOn_api();
		if(api_status)
		{
			bool setup_finished{false};
			Serial.println("Waiting For WebSocket Client.");
			Serial.println();
			do 
			{
				socket->loop();
				if (socket->num_user > 0)
				{	
					socket->add_sent_data("setup", true);
					socket->sent_data();
				}
					
				if (socket->received)
				{
					ArduinoJson::DynamicJsonDocument* const data{ socket->get_data() };
					Serial.println("Send your wifi credential");
					if ((*data)["type"].as<int>() == 100) // Wifi Setup
					{
						JsonVariant SSID = (*data)["SSID"];
						JsonVariant PASS = (*data)["PASS"];
						if (!SSID.isNull() && !PASS.isNull())
						{
							file->add_data("SSID", SSID.as<char*>());
							file->add_data("PASS", PASS.as<char*>());
							file->save_data();
							socket->add_sent_data("status", true);
							socket->add_sent_data("client", (*data)["client"].as<int>());
							socket->sent_data();
							setup_finished = true;
							Serial.println("Reseting..");
							ESP.restart();
						}
						else
						{
							socket->add_sent_data("status", false);
							socket->add_sent_data("client", (*data)["client"].as<int>());
							socket->add_sent_data("error", "Value missing.");
							socket->sent_data();
						}
						
					}
				}
			} 
			while (!setup_finished);
		}
	}
}
