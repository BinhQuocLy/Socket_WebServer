#include "Server.h"

void main() {
	try {
		Server server("3000");
		server.run();
	}
	catch (std::runtime_error& e) {
		std::cout << "Run-time error: " << e.what();
	}
}