#ifndef WEB_PRINT_H
#define WEB_PRINT_H

#include <ArduinoWebsockets.h>

struct webPrint {
	// Writes one byte, returns the number of bytes written (0 or 1)
	static websockets::WebsocketsClient *clientp;
	webPrint(websockets::WebsocketsClient *client);
	~webPrint();
	size_t write(uint8_t c);
	// Writes several bytes, returns the number of bytes written
	size_t write(const uint8_t* buffer, size_t length);
};
#endif