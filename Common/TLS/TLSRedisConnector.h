#pragma once
#include <string>
#include <cpp_redis/cpp_redis>

class TLSRedisConnector
{
private:
	struct REDISDATA
	{
		cpp_redis::client client;
		cpp_redis::reply reply;
	};
public:
	TLSRedisConnector(const char* txtname);
	~TLSRedisConnector();
	bool Connect();
	void Disconnect();
	bool Set(char* key,char* value);
	bool SetEx(char* key,__int64 second,char* value);
	bool Get(char* key,char* reply);
	void Del(char* key);
	void GetReply(char* result);

private:
	void Parsing(const char* txtname);

private:
	DWORD _tlsIndex = 0;

	std::string _host;
	size_t _port;
	unsigned int _timeout;
	int _maxReconnect;
	unsigned int _reconnectInterval;};

