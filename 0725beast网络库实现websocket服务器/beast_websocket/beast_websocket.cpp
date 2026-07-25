#include "WebSocketServer.h"


int main() {
	net::io_context ioc;
	WebSocketServer server(ioc, 10086);
	server.StartAccept();
	ioc.run();

	return 0;
}