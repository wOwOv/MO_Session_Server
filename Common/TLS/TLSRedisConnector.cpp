#define _CRT_SECURE_NO_WARNINGS

#include "TLSRedisConnector.h"
#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")
#include <string>
#include "Logger.h"

TLSRedisConnector::TLSRedisConnector(const char* txtname)
{
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(version, &data);
	Parsing(txtname);
	_tlsIndex = TlsAlloc();
	if (_tlsIndex == TLS_OUT_OF_INDEXES)
	{
		DebugBreak();
	}
	printf("tlsredis created\n");
}

TLSRedisConnector::~TLSRedisConnector()
{
	WSACleanup();
	printf("tlsredis destroyed\n");
}

bool TLSRedisConnector::Connect()
{
	REDISDATA* rdata = (REDISDATA*)TlsGetValue(_tlsIndex);
	if (rdata == nullptr)
	{
		rdata = new REDISDATA;
		TlsSetValue(_tlsIndex, (LPVOID)rdata);
	}

	rdata->client.connect(_host, _port, nullptr, _timeout, _maxReconnect, _reconnectInterval);
	
}

void TLSRedisConnector::Disconnect()
{
	REDISDATA* rdata = (REDISDATA*)TlsGetValue(_tlsIndex);
	if (rdata != nullptr)
	{
		if (rdata->client.is_connected())
		{
			rdata->client.disconnect();
		}
	}
}

bool TLSRedisConnector::Set(char* key, char* value)
{
	REDISDATA* rdata = (REDISDATA*)TlsGetValue(_tlsIndex);
	if (rdata == nullptr)
	{
		rdata = new REDISDATA;
		TlsSetValue(_tlsIndex, (LPVOID)rdata);
	}
	if (!rdata->client.is_connected())
	{
		rdata->client.connect(_host, _port, nullptr, _timeout, _maxReconnect, _reconnectInterval);
		Sleep(1000);
	}
	rdata->client.set(key, value);
	rdata->client.sync_commit();
	return true;
}

bool TLSRedisConnector::SetEx(char* key, __int64 second, char* value)
{
	REDISDATA* rdata = (REDISDATA*)TlsGetValue(_tlsIndex);
	if (rdata == nullptr)
	{
		rdata = new REDISDATA;
		TlsSetValue(_tlsIndex, (LPVOID)rdata);
	}
	if (!rdata->client.is_connected())
	{
		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		WSAStartup(version, &data);

		rdata->client.connect(_host, _port, nullptr, _timeout, _maxReconnect, _reconnectInterval);
		Sleep(1000);
	}
	rdata->client.setex(key, second, value);
	rdata->client.sync_commit();
	return true;
}

bool TLSRedisConnector::Get(char* key,char* reply)
{
	REDISDATA* rdata = (REDISDATA*)TlsGetValue(_tlsIndex);
	if (rdata == nullptr)
	{
		rdata = new REDISDATA;
		TlsSetValue(_tlsIndex, (LPVOID)rdata);
	}
	if (!rdata->client.is_connected())
	{
		rdata->client.connect(_host, _port, nullptr, _timeout, _maxReconnect, _reconnectInterval);
		Sleep(1000);
	}
	cpp_redis::reply result;
	rdata->client.get(key, [&result](cpp_redis::reply& reply) {result = reply;	});
	rdata->client.sync_commit();
	rdata->reply = result;
	const std::string& s = result.as_string();
	strcpy(reply, s.c_str());
	return true;
}

void TLSRedisConnector::Del(char* key)
{
	REDISDATA* rdata = (REDISDATA*)TlsGetValue(_tlsIndex);
	if (rdata == nullptr)
	{
		rdata = new REDISDATA;
		TlsSetValue(_tlsIndex, (LPVOID)rdata);
	}
	if (!rdata->client.is_connected())
	{
		rdata->client.connect(_host, _port, nullptr, _timeout, _maxReconnect, _reconnectInterval);
		Sleep(1000);
	}
	rdata->client.del({ key });
	rdata->client.sync_commit();
}

void TLSRedisConnector::GetReply(char* result)
{
	REDISDATA* rdata = (REDISDATA*)TlsGetValue(_tlsIndex);
	if (rdata == nullptr)
	{
		LOG(L"Redis", LVERROR, L"Get Reply Before Get");
	}
	const std::string& s = rdata->reply.as_string();
	strcpy(result, s.c_str());
}


void TLSRedisConnector::Parsing(const char* txtname)
{
	//DBConnect_config.txt 파싱해오기

//txtfile open
	FILE* file;
	fopen_s(&file, txtname, "rb");
	if (file == NULL)
	{
		printf("fopen error\n");
	}
	//버퍼에 복사
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	char* buffer = new char[size];
	rewind(file);
	size_t frerror = fread(buffer, size, 1, file);
	if (frerror == 0)
	{
		printf("fread error\n");
	}
	fclose(file);
	char* ptr = buffer;
	char host[32];
	char port[16];
	char timeout[8];
	char MaxR[8];
	char Reconnect[8];

	int cnt = 0;

	//host parsing
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		host[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	host[cnt] = '\0';

	//port parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		port[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	port[cnt] = '\0';

	//timeout parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		timeout[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	timeout[cnt] = '\0';

	//MaxReconnect parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		MaxR[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	MaxR[cnt] = '\0';

	//ReconnectInterval parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		Reconnect[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	Reconnect[cnt] = '\0';

	//parsing한 것들 필요한 자료형으로 바꿔서 주기
	_host = host;
	printf("\n%s\n", _host.c_str());
	_port = atoi(port);
	_timeout = atoi(timeout);
	_maxReconnect = atoi(MaxR);
	_reconnectInterval = atoi(Reconnect);


	delete[] buffer;
}