#include "connection.hpp"

Connection::Connection() {
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(getPort());
}

Connection::~Connection() {
	close(sockfd);
}

void Connection::waitConnection() {
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0) {
		std::cerr << "[-] Error: Failed to create socket." << std::endl;
		return;
	}
	if (bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
		std::cerr << "[-] Error: Failed to bind the socket." << std::endl;
		return;
	}
	if (listen(sockfd, BACKLOG) < 0) {
		std::cerr << "[-] Error: Failed to listen." << std::endl;
		return;
	}
	std::cout << "[*] Wait: server is listening for connections " << inet_ntoa(addr.sin_addr) << ':'
			<< htons(addr.sin_port) << std::endl;

	while (true) {
		int32_t sockfd_listen;
		int32_t sockfd_communicate;
		struct sockaddr_in addr_listen { };
		struct sockaddr_in addr_communicate { };
		socklen_t addr_listen_len = sizeof(addr_listen);
		socklen_t addr_communicate_len = sizeof(addr_communicate);

		sockfd_listen = accept(sockfd, reinterpret_cast<struct sockaddr*>(&addr_listen), &addr_listen_len);
		addr.sin_port = htons(getPort());
		sockfd_communicate = accept(sockfd, reinterpret_cast<struct sockaddr*>(&addr_communicate),
		                            &addr_communicate_len);
		if (sockfd_listen < 0 || sockfd_communicate < 0) { continue; }
		std::cout << "[+] Success: Client connected: " << inet_ntoa(addr_listen.sin_addr)
				<< ':' << htons(addr_listen.sin_port) << ' ' << inet_ntoa(addr_listen.sin_addr) << ':'
				<< htons(addr_communicate.sin_port) << '.' << std::endl;
		std::thread(&Connection::handleClients, this, sockfd_listen, sockfd_communicate).detach();
	}
}

bool Connection::isConnect(int32_t sockfd_listen, int32_t sockfd_communicate) {
	return checkConnection(sockfd_listen) && checkConnection(sockfd_communicate);
}

int32_t Connection::getPort() {
	sockaddr_in addr { };
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = 0;

	int32_t fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		std::cerr << "[-] Error: Failed to create the socket." << std::endl;
		return -1;
	}
	if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
		std::cerr << "[-] Error: Failed bind socket." << std::endl;
		close(fd);
		return -1;
	}
	socklen_t addr_len = sizeof(addr);
	if (getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &addr_len) < 0) {
		std::cerr << "[-] Error: Failed get socket name." << std::endl;
		close(fd);
		return -1;
	}
	int32_t port = ntohs(addr.sin_port);
	close(fd);
	return port;
}

void Connection::handleClients(int32_t sockfd_listen, int32_t sockfd_communicate) {
	const std::string& command_list = commands_client[0];
	const std::string& command_get = commands_client[1] + ':';
	const std::string& command_exit = commands_client[2];

	synchronizationStorage(sockfd_listen);
	while (isConnect(sockfd_listen, sockfd_communicate)) {
		std::string command = receiveData(sockfd_listen, MSG_DONTWAIT);

		if (command == command_list) {
			sendListFiles(sockfd_communicate);
		} else if (command.find(command_get) != std::string::npos) {
			std::vector<std::string> tokens;
			split(command, ':', tokens);
			std::string filename = tokens[1];
			sendFile(sockfd_communicate, filename);
		} else if (command == command_exit) {
			std::cout << "[+] Success: Client disconnected." << std::endl;
			break;
		}
	}
	close(sockfd_listen);
	close(sockfd_communicate);
	removeClientFiles(sockfd_listen);
}

std::string Connection::receiveData(int32_t fd, int32_t flags) {
	std::byte buffer[BUFFER_SIZE];
	int64_t bytes;
	std::string receive_data;

	bytes = recv(fd, buffer, BUFFER_SIZE, flags);
	if (bytes > 0) {
		receive_data = std::string(reinterpret_cast<char*>(buffer), bytes);
	}
	return receive_data;
}

int64_t Connection::sendData(int32_t fd, const std::string& command, int32_t flags) {
	int64_t bytes;

	bytes = send(fd, command.c_str(), command.size(), flags);
	return bytes;
}

void Connection::synchronizationStorage(int32_t sockfd) {
	std::string response;
	uint64_t pos_start;

	response = receiveData(sockfd, MSG_WAITFORONE);
	pos_start = response.find(start_marker);

	if (pos_start == std::string::npos) {
		std::cerr << "[-] Error: Failed update storage." << std::endl;
		return;
	}
	response.erase(pos_start, start_marker.length());

	do {
		std::istringstream iss(response);
		std::string data;
		std::string filename;
		std::string hash;
		int64_t size;

		while (std::getline(iss, data, ' ') && data != end_marker) {
			std::istringstream file_stream(data);
			std::getline(file_stream, filename, ':');
			std::string size_str;
			std::getline(file_stream, size_str, ':');
			size = static_cast<int64_t>(std::stoull(size_str));
			std::getline(file_stream, hash);
			if (!filename.empty() && !hash.empty()) {
				storeFiles(sockfd, filename, size, hash);
			}
		}
		if (data == end_marker) {
			break;
		}
	} while (!(response = receiveData(sockfd, MSG_WAITFORONE)).empty());
	updateFilesWithSameHash();
}

bool Connection::checkConnection(int32_t fd) {
	int32_t optval;
	socklen_t optlen = sizeof(optval);
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen) == -1 || optval != 0) {
		return false;
	}
	return true;
}

int64_t Connection::sendListFiles(int32_t fd) {
	std::vector<std::string> files = getListFiles();
	std::string list;
	int64_t bytes = 0;
	int64_t send_bytes;

	send_bytes = sendData(fd, start_marker, MSG_CONFIRM);
	if (send_bytes < 0) {
		std::cerr << "[-] Error: Failed send the list of files." << std::endl;
		return -1;
	}

	for (const auto& file : files) {
		list += file + ' ';
		if (list.size() + file.size() > BUFFER_SIZE) {
			send_bytes = sendData(fd, list, MSG_CONFIRM);
			if (send_bytes < 0) {
				std::cerr << "[-] Error: Failed send the list of files." << std::endl;
				return -1;
			}
			bytes += send_bytes;
			list.clear();
		}
	}

	send_bytes = sendData(fd, list, MSG_CONFIRM);
	if (send_bytes < 0) {
		std::cerr << "[-] Error: Failed send the list of files." << std::endl;
		return -1;
	}
	bytes += send_bytes;

	send_bytes = sendData(fd, end_marker, MSG_CONFIRM);
	if (send_bytes < 0) {
		std::cerr << "[-] Error: Failed send the list of files." << std::endl;
		return -1;
	}
	return bytes;
}

int64_t Connection::sendFile(int32_t fd, const std::string& filename) {
	std::multimap<int32_t, FileInfo> files = getFilename(filename);
	std::byte buffer[BUFFER_SIZE];
	int64_t size = getSize(filename);
	int64_t send_bytes;
	int64_t read_bytes;
	int64_t bytes = 0;
	int32_t client_fd = findFd(filename);

	if (files.empty()) {
		std::cerr << "[-] Error: Failed send the file." << std::endl;
		return -1;
	}

	send_bytes = sendData(fd, start_marker, MSG_CONFIRM);
	if (send_bytes < 0) {
		std::cerr << "[-] Error: Failed send the file." << std::endl;
		return -1;
	}

	uint64_t offset = 0;
	do {
		read_bytes = getFile(client_fd, filename, buffer, offset, BUFFER_SIZE);
		if (read_bytes > 0) {
			std::string data(reinterpret_cast<char*>(buffer), read_bytes);
			send_bytes = sendData(fd, data, MSG_CONFIRM);
			if (send_bytes != read_bytes) {
				std::cerr << "[-] Error: Failed send part of the file: " << filename << '.' << std::endl;
				return -1;
			}
			bytes += send_bytes;
			offset += read_bytes;
		}
	} while (bytes < size);

	send_bytes = sendData(fd, end_marker, MSG_CONFIRM);
	if (send_bytes < 0) {
		std::cerr << "[-] Error: Failed send part of the file: " << filename << '.' << std::endl;
		return -1;
	}
	std::cout << "[+] Success: File sent successfully: " << filename << '.' << std::endl;
	return bytes;
}

int64_t Connection::getFile(int32_t fd, const std::string& filename, std::byte* buffer,
                            uint64_t offset, uint64_t max_size) {
	const std::string& command_part = commands_server[2] + ':' + std::to_string(offset) + ':' +
			std::to_string(max_size) + ':' + filename;
	int64_t bytes = 0;
	int64_t send_bytes;
	uint64_t pos;
	std::string response;
	std::lock_guard<std::mutex> lock(mutex);

	send_bytes = sendData(fd, command_part, MSG_CONFIRM);
	if (send_bytes < 0) {
		std::cerr << "[-] Error: Failed get part of the file: " << filename << '.' << std::endl;
		return -1;
	}

	response = receiveData(fd, MSG_WAITFORONE);
	pos = response.find(start_marker);
	if (pos != std::string::npos) {
		response.erase(pos, start_marker.length());
	}

	do {
		pos = response.find(end_marker);
		if (pos == std::string::npos) {
			std::memcpy(buffer, response.data(), std::min(response.size(), static_cast<std::size_t>(BUFFER_SIZE)));
			bytes += static_cast<int64_t>(response.size());
			continue;
		}
		response.erase(pos, end_marker.length());
		std::memcpy(buffer, response.data(), std::min(response.size(), static_cast<std::size_t>(BUFFER_SIZE)));
		bytes += static_cast<int64_t>(response.size());
	} while (!(response = receiveData(fd, MSG_WAITFORONE)).empty());
	return bytes;
}

int32_t Connection::findFd(const std::string& filename) {
	for (const auto& it : storage) {
		if (it.second.filename == filename) {
			return it.first;
		}
	}
	return -1;
}

std::vector<std::string> Connection::getListFiles() {
	std::lock_guard<std::mutex> lock(mutex);
	std::vector<std::string> list;

	for (const auto& entry : storage) {
		list.push_back(entry.second.filename);
	}
	return list;
}

std::multimap<int32_t, FileInfo> Connection::getFilename(const std::string& filename) {
	std::lock_guard<std::mutex> lock(mutex);
	std::multimap<int32_t, FileInfo> result;

	for (const auto& entry : storage) {
		if (entry.second.filename == filename) {
			result.insert(std::pair(entry.first, entry.second));
		}
	}
	return result;
}

int64_t Connection::getSize(const std::string& filename) {
	std::lock_guard<std::mutex> lock(mutex);
	for (const auto& it : storage) {
		if (it.second.filename == filename) {
			return it.second.size;
		}
	}
	return -1;
}

void Connection::updateFilesWithSameHash() {
	std::lock_guard<std::mutex> lock(mutex);

	for (auto first = storage.begin(); first != storage.end(); ++first) {
		for (auto second = std::next(first); second != storage.end(); ++second) {
			if (first->second.hash == second->second.hash) {
				second->second.filename = first->second.filename;
				break;
			}
		}
	}
}

void Connection::storeFiles(int32_t fd, const std::string& filename, int64_t size, const std::string& hash) {
	std::lock_guard<std::mutex> lock(mutex);
	FileInfo data {size, hash, filename};
	storage.insert(std::pair(fd, data));
	std::cout << "[+] Success: Stored the file: " << filename << '.' << std::endl;
}

void Connection::removeClientFiles(int32_t fd) {
	std::lock_guard<std::mutex> lock(mutex);
	auto range = storage.equal_range(fd);

	storage.erase(range.first, range.second);
	storage.erase(fd);
}

void Connection::split(const std::string& str, char delim, std::vector<std::string>& tokens) {
	std::string token;
	std::istringstream tokenStream(str);

	while (std::getline(tokenStream, token, delim)) {
		tokens.push_back(token);
	}
}
