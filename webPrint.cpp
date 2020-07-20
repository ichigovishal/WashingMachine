#include "webPrint.h"
websockets::WebsocketsClient* webPrint::clientp{};
webPrint::webPrint(websockets::WebsocketsClient* client)
{
	webPrint::clientp = client;
	webPrint::clientp->stream();
}

webPrint::~webPrint()
{
	webPrint::clientp->end();

}
size_t webPrint::write(uint8_t c)
{
	char da = (const char)c;
	bool writen = webPrint::clientp->send(&da, 1);
	return (writen) ? 1 : 0;
}
size_t webPrint::write(const uint8_t* buffer, size_t length)
{
	size_t sented{ 0 };
	for (size_t i = 0; i < length; i++)
	{
		char da = (const char)buffer[i];
		bool writen = webPrint::clientp->send(&da, 1);
		sented + ((writen) ? 1 : 0);
	}
	return sented;
}