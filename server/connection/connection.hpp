#pragma once

#include "../server.hpp"

class Connection {
 public:
	explicit Connection(int32_t);

	~Connection();

	void waitConnection();

	[[nodiscard]] bool isConnect(int32_t) const;

 private:
	void handleClients(int32_t, sockaddr_in);

	static std::string receiveData(int32_t, int32_t);

	static int64_t sendData(int32_t, const std::string&, int32_t);

	void synchronizationStorage(int32_t);

	static bool checkConnection(int32_t);

	int64_t sendListFiles(int32_t);

	int64_t sendFile(int32_t, const std::string&);

	int64_t getFile(int32_t, const std::string&, std::byte*, uint64_t, uint64_t);

	int32_t findFd(const std::string&);

	std::vector<std::string> getListFiles();

	std::multimap<int32_t, FileInfo> getFilename(const std::string&);

	int64_t getSize(const std::string&);

	void updateFilesWithSameHash();

	void storeFiles(int32_t, const std::string&, int64_t, const std::string&);

	void removeClientFiles(int32_t);

	static void split(const std::string&, char, std::vector<std::string>&);

	std::multimap<int32_t, FileInfo> storage;
	struct sockaddr_in addr_communicate { };
	int32_t sockfd_communicate { };
	int32_t server_port;
	std::mutex mutex;
};
