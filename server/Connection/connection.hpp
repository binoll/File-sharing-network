#pragma once

#include "../server.hpp"

struct FileInfo {
	std::string hash;
	std::string filename;
};

class Connection {
 public:
	explicit Connection(size_t);

	~Connection();

	void createConnection();

 private:
	void handleClient(ssize_t);

	std::string receive(ssize_t);

	void updateStorage(ssize_t);

	void sendMsgToClient(ssize_t fd, const std::string& msg);

	void sendFile(ssize_t, const std::string&);

	std::vector<std::string> getFileList();

	void storeFile(ssize_t, const std::string&, const std::string&);

	void sendFileList(ssize_t);

	void eraseFilesWithSameHash();

	void split(const std::string&, char, std::vector<std::string>&);

	ssize_t socket_fd;
	struct sockaddr_in addr;
	size_t port;
	std::unordered_map<ssize_t, FileInfo> storage;
	std::unique_ptr<std::thread> client_thread;
	const std::string start_marker = marker[0];
	const std::string end_marker = marker[1];
};
