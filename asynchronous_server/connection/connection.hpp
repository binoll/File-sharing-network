// Copyright 2024 binoll
#pragma once

#include "../server.hpp"

class Connection {
 public:
	explicit Connection();

	~Connection();

	void waitConnection();

	static bool isConnect(std::pair<int32_t, int32_t>);

 private:
	void hasDataToRead(boost::coroutines::asymmetric_coroutine<std::pair<int32_t, int32_t>>::push_type&);

	static int32_t getPort();

	static bool checkConnection(int32_t);

	void handleClients();

	void processingClient(boost::coroutines::asymmetric_coroutine<void>::push_type&,
	                      std::pair<int32_t, int32_t>);

	static int64_t sendMessage(int32_t, const std::string&, int32_t);

	static int64_t receiveMessage(int32_t, std::string&, int32_t);

	static int64_t sendBytes(int32_t, const std::byte*, int64_t, int32_t);

	static int64_t receiveBytes(int32_t, std::byte*, int64_t, int32_t);

	static int64_t processResponse(std::string&);

	bool synchronization(std::pair<int32_t, int32_t>&);

	int64_t sendList(int32_t);

	int64_t sendFile(boost::coroutines::asymmetric_coroutine<int64_t>::push_type&, int32_t, const std::string&);

	std::vector<std::pair<int32_t, int32_t>> findSocket(const std::string&);

	std::vector<std::string> getListFiles();

	int64_t getSize(const std::string&);

	void updateStorage();

	void storeFiles(std::pair<int32_t, int32_t>, const std::string&, int64_t, const std::string&);

	void removeClients(std::pair<int32_t, int32_t> pair);

	static void split(const std::string&, std::vector<std::string>&);

	static std::string removeIndex(std::string);

	bool isFilenameChanged(const std::string&);

	int32_t socket_listen { };
	int32_t socket_communicate { };
	struct sockaddr_in addr_listen { };
	struct sockaddr_in addr_communicate { };
	std::multimap<std::pair<int32_t, int32_t>, FileInfo> storage;
	std::vector<std::pair<int32_t, int32_t>> sockets;
};
