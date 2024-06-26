// Copyright 2024 binoll
#include "client.hpp"
#include "commandline/commandline.hpp"

int32_t main() {
	std::string current_path = ".";
	CommandLine command_line(current_path);

	command_line.run();

	return EXIT_SUCCESS;
}
