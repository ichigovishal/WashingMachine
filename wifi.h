#ifndef WIFI_H
#define WIFI_H

#include "custom_Websocket.h"

namespace wifi
{
	bool turnOn_api();
	bool connect_wifi(const char* SSID, const char* PASS);
	void main(custom_Websocket* socket, open_file* file);

};
#endif //WIFI_H

