#pragma once
#include <mysql.h>
#include "Logger.h"
#include <strsafe.h>
#include <cstdint>

enum class DBErrorCategory : std::uint8_t {
	None,						//
	DuplicateKey,				//1062, 데이터 오류로 재시도 대상 아님
	Deadlock,					//1213, 나중에 제한 재시도 후보
	LockTimeout,				//1205, 나중에 제한 재시도 후보
	ConnectionLost,				//2006, 2013, 나중에 재연결 정책 검토 대상
	QueryFormatError,			//SQL 문자열 생성 실패
	Unknown,					//그 외 MySQL 오류
};

struct DBErrorInfo {
	DBErrorCategory category = DBErrorCategory::None;
	unsigned int mysqlError = 0;
};

class DBConnector
{
public:
	DBConnector(const char* txtname);
	~DBConnector();
	
	DBConnector(const DBConnector&) = delete;
	DBConnector& operator=(const DBConnector&) = delete;
	DBConnector(DBConnector&&) = delete;
	DBConnector& operator=(DBConnector&&) = delete;

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
			SetLastError(DBErrorCategory::QueryFormatError);
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
			SetLastError(DBErrorCategory::QueryFormatError);
			LOG(L"Database", LVSYSTEM, L"QuerySelect format error");
			return false;
		}

		return ExecuteSelectQuery(wquery);
	}
	void GetQueryResult(MYSQL_RES** result);

	DBErrorInfo GetLastError() const;

private:
	bool ExecuteSaveQuery(const WCHAR* wquery);
	bool ExecuteSelectQuery(const WCHAR* wquery);
	
	void Parsing(const char* txtname);

	void ClearLastError();
	void SetLastMysqlError(unsigned int mysqlError);
	void SetLastError(DBErrorCategory category, unsigned int mysqlError = 0);

private:
	MYSQL _conn;
	MYSQL* _connection;
	MYSQL_RES* _sqlResult;
	MYSQL_ROW _sqlRow;
	int _queryStat;
	DBErrorInfo _lastError;

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

	TLSDBConnector(const DBConnector&) = delete;
	TLSDBConnector& operator=(const DBConnector&) = delete;
	TLSDBConnector(DBConnector&&) = delete;
	TLSDBConnector& operator=(DBConnector&&) = delete;

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