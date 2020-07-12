#pragma once
#include <fstream>
#include <string>
#include <iostream>

class FileReader {
public:
	static void readContent(const char* path, std::string& content) {
		std::ifstream reader(path);
		if (reader.is_open()) {
			std::string buffer((std::istreambuf_iterator<char>(reader)),
				(std::istreambuf_iterator<char>()));
			reader.close();
			content = buffer;
		}
		else {
			std::cerr << "Cannot read file\n";
		}
	}
};