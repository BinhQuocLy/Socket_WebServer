#include "Server.h"

void main() {
	try {
		Server server("3000");
		std::thread th(&Server::run, &server);
		th.join();
	}
	catch (std::runtime_error& e) {
		std::cout << "Run-time error: " << e.what();
	}
}