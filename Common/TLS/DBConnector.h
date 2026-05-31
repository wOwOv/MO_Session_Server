#pragma once
#include "C:/Program Files/MySQL/MySQL Server 8.0/include/mysql.h"

class DBConnector
{
public:
	DBConnector(const char* txtname);
	~DBConnector();
	bool Connect();
	void Disconnect();
	bool QuerySave( const WCHAR* String, ...);
	bool QuerySelect(const WCHAR* String, ...);
private:
	void Parsing(const char* txtname);
private:
	MYSQL _conn;
	MYSQL* _connection;
	MYSQL_RES* _sqlResult;
	MYSQL_ROW _sqlRow;
	int _queryStat;

private:
	char _host[32];
	char _user[64];
	char _passwd[64];
	char _db[64];
	unsigned int _port;
	ULONGLONG _limitTime;

};

class TLSDBConnector
{
private:
	struct SQLDATA
	{
		MYSQL _conn;
		MYSQL* _connection;
		MYSQL_RES* _sqlResult;
		MYSQL_ROW _sqlRow;
		int _queryStat;
	};
public:
	TLSDBConnector(const char* txtname);
	~TLSDBConnector();
	bool Connect();
	void Disconnect();
	bool QuerySave(const WCHAR* String, ...);
	bool QuerySelect(const WCHAR* String, ...);
	void GetQueryResult(MYSQL_RES** result);

private:
	void Parsing(const char* txtname);

private:
	DWORD _tlsIndex = 0;

	char _host[32];
	char _user[64];
	char _passwd[64];
	char _db[64];
	unsigned int _port;
	ULONGLONG _limitTime;
};