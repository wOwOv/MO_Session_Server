#pragma once
#include <mysql.h>
#include "Logger.h"
#include <strsafe.h>

class DBConnector
{
public:
	DBConnector(const char* txtname);
	~DBConnector();
	
	bool Connect();
	void Disconnect();

	bool BeginTransaction();
	bool Commit();
	bool Rollback();

	template <typename... Args>
	bool QuerySave(const WCHAR* String, Args&&... args)
	{
		WCHAR wquery[4096];
		HRESULT result = StringCchPrintfW(wquery, _countof(wquery), String, args...);
		if (FAILED(result))
		{
			LOG(L"Database", LVSYSTEM, L"QuerySave format error");
			return false;
		}

		return ExecuteSaveQuery(wquery);
	}
	template <typename... Args>
	bool QuerySelect(const WCHAR* String, Args&&... args)
	{
		WCHAR wquery[4096];
		HRESULT result = StringCchPrintfW(wquery, _countof(wquery), String, args...);
		if (FAILED(result))
		{
			LOG(L"Database", LVSYSTEM, L"QuerySelect format error");
			return false;
		}

		return ExecuteSelectQuery(wquery);
	}
	void GetQueryResult(MYSQL_RES** result);

private:
	bool ExecuteSaveQuery(const WCHAR* wquery);
	bool ExecuteSelectQuery(const WCHAR* wquery);
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
	unsigned int _limitTime;

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

	bool BeginTransaction();
	bool Commit();
	bool Rollback();
	
	template <typename... Args>
	bool QuerySave(const WCHAR* String, Args&&... args)
	{
		WCHAR wquery[4096];
		HRESULT result = StringCchPrintfW(wquery, _countof(wquery), String, args...);
		if (FAILED(result))
		{
			LOG(L"Database", LVSYSTEM, L"QuerySave format error");
			return false;
		}

		return ExecuteSaveQuery(wquery);
	}
	template <typename... Args>
	bool QuerySelect(const WCHAR* String, Args&&... args)
	{
		WCHAR wquery[4096];
		HRESULT result = StringCchPrintfW(wquery, _countof(wquery), String, args...);
		if (FAILED(result))
		{
			LOG(L"Database", LVSYSTEM, L"QuerySelect format error");
			return false;
		}

		return ExecuteSelectQuery(wquery);
	}
	void GetQueryResult(MYSQL_RES** result);

private:
	bool ExecuteSaveQuery(const WCHAR* wquery);
	bool ExecuteSelectQuery(const WCHAR* wquery);
	void Parsing(const char* txtname);

private:
	DWORD _tlsIndex = 0;

	char _host[32];
	char _user[64];
	char _passwd[64];
	char _db[64];
	unsigned int _port;
	unsigned int _limitTime;
};