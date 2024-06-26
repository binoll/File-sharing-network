// Copyright 2024 binoll
#pragma once

#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <string>
#include <thread>
#include <numeric>
#include <vector>
#include <mutex>
#include <list>
#include <map>
#include <set>

#include <sys/socket.h>
#include <dirent.h>
#include <arpa/inet.h>

#include <boost/log/trivial.hpp>
#include <boost/crc.hpp>

#include <openssl/evp.h>

#define BUFFER_SIZE 1000

const std::vector<std::string> commands = {"list", "get", "error", "exist"};
