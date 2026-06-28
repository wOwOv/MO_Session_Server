#include "DBConnector.h"
#include "C:/Program Files/MySQL/MySQL Server 8.0/include/mysql.h"
#include "C:/Program Files/MySQL/MySQL Server 8.0/include/errmsg.h"
#pragma comment(lib, "mysqlclient.lib")
#include <iostream>
#include "Logger.h"
#include <Parser.h>
#include <strsafe.h>
#include <windows.h>

DBConnector::DBConnector(const char* txtname)
{
	Parsing(txtname);
}

DBConnector::~DBConnector()
{
}



bool DBConnector::Connect()
{
	mysql_init(&_conn);
	
	_connection = mysql_real_connect(&_conn, _host, _user, _passwd, _db, _port, (char*)NULL, 0);
	_queryStat = mysql_set_server_option(_connection, MYSQL_OPTION_MULTI_STATEMENTS_ON);
	if (_connection == nullptr)
	{
		// mysql_errno(&_MySQL);
		LOG(L"Database", LVSYSTEM, L"Mysql connection error : %s", mysql_error(&_conn));
		return false;
	}
	return true;
}

void DBConnector::Disconnect()
{
	mysql_close(_connection);
}

bool DBConnector::BeginTransaction()
{
	return mysql_query(_connection, "BEGIN") == 0;
}

bool DBConnector::Commit()
{
	return mysql_query(_connection, "COMMIT") == 0;
}

bool DBConnector::Rollback()
{
	return mysql_query(_connection, "ROLLBACK") == 0;
}

void DBConnector::GetQueryResult(MYSQL_RES** result)
{
	*result = _sqlResult;
}

bool DBConnector::ExecuteSaveQuery(const WCHAR* wquery)
{
	char cquery[4096];

	WideCharToMultiByte(CP_UTF8, 0, wquery, -1, cquery, 4096, NULL, NULL);

	ULONGLONG start = GetTickCount64();
	_queryStat = mysql_query(_connection, cquery);
	ULONGLONG time = GetTickCount64() - start;
	if (_queryStat != 0)
	{
		LOG(L"Database", LVSYSTEM, L"Mysql query error : %s", mysql_error(&_conn));
		return false;
	}

	if (time >= _limitTime)
	{
		LOG(L"Database", LVSYSTEM, L"Mysql query time : %d / query : %s", time, wquery);
	}

	do {
		_sqlResult = mysql_store_result(_connection);
		if (_sqlResult) {
			mysql_free_result(_sqlResult);
		}
	} while (mysql_next_result(_connection) == 0);

	return true;
}

bool DBConnector::ExecuteSelectQuery(const WCHAR* wquery)
{
	char cquery[4096];

	WideCharToMultiByte(CP_UTF8, 0, wquery, -1, cquery, 4096, NULL, NULL);

	ULONGLONG start = GetTickCount64();
	_queryStat = mysql_query(_connection, cquery);
	ULONGLONG time = GetTickCount64() - start;
	if (_queryStat != 0)
	{
		LOG(L"Database", LVSYSTEM, L"Mysql query error : %s", mysql_error(&_conn));
		return false;
	}

	if (time >= _limitTime)
	{
		LOG(L"Database", LVSYSTEM, L"Mysql query time : %d / query : %s", time, wquery);
	}

	_sqlResult = mysql_store_result(_connection);

	return true;
}

void DBConnector::Parsing(const char* txtname)
{
	//DBInfo.txt ÆÄ½ÌÇØ¿À±â
	Parser parser;
	parser.LoadFile(txtname);

	parser.GetString("DB_INFO", "host", _host, sizeof(_host));
	parser.GetString("DB_INFO", "user", _user, sizeof(_user));
	parser.GetString("DB_INFO", "passwd", _passwd, sizeof(_passwd));
	parser.GetString("DB_INFO", "db", _db, sizeof(_db));

	parser.GetValue("DB_INFO", "port", (int*)&_port);
	parser.GetValue("DB_INFO", "limitTime", (int*)&_limitTime);
}

TLSDBConnector::TLSDBConnector(const char* txtname)
{
	Parsing(txtname);

	_tlsIndex = TlsAlloc();
	if (_tlsIndex == TLS_OUT_OF_INDEXES)
	{
		DebugBreak();
	}
}

TLSDBConnector::~TLSDBConnector()
{
}

bool TLSDBConnector::Connect()
{
	SQLDATA* sqldata = (SQLDATA*)TlsGetValue(_tlsIndex);
	if (sqldata == nullptr)
	{
		sqldata = new SQLDATA;
		TlsSetValue(_tlsIndex, (LPVOID)sqldata);
	}

	mysql_init(&sqldata->_conn);

	sqldata->_connection = mysql_real_connect(&sqldata->_conn, _host, _user, _passwd, _db, _port, (char*)NULL, 0);
	sqldata->_queryStat = mysql_set_server_option(sqldata->_connection, MYSQL_OPTION_MULTI_STATEMENTS_ON);
	if (sqldata->_connection == NULL)
	{
		// mysql_errno(&_MySQL);
		LOG(L"Database", LVSYSTEM, L"Mysql connection error : %s", mysql_error(&sqldata->_conn));
		return false;
	}
	return true;
}

void TLSDBConnector::Disconnect()
{
	SQLDATA* sqldata = (SQLDATA*)TlsGetValue(_tlsIndex);
	mysql_close(sqldata->_connection);
}

bool TLSDBConnector::BeginTransaction()
{
	SQLDATA* sqldata = (SQLDATA*)TlsGetValue(_tlsIndex);
	if (sqldata == nullptr)
	{
		sqldata = new SQLDATA;

		mysql_init(&sqldata->_conn);

		sqldata->_connection = mysql_real_connect(&sqldata->_conn, _host, _user, _passwd, _db, _port, (char*)NULL, 0);
		sqldata->_queryStat = mysql_set_server_option(sqldata->_connection, MYSQL_OPTION_MULTI_STATEMENTS_ON);
		if (sqldata->_connection == NULL)
		{
			LOG(L"Database", LVSYSTEM, L"Mysql connection error : %s", mysql_error(&sqldata->_conn));
			return false;
		}
		TlsSetValue(_tlsIndex, (LPVOID)sqldata);
	}


	return mysql_query(sqldata->_connection, "BEGIN") == 0;
}

bool TLSDBConnector::Commit()
{
	SQLDATA* sqldata = (SQLDATA*)TlsGetValue(_tlsIndex);
	if (sqldata == nullptr)
	{
		sqldata = new SQLDATA;

		mysql_init(&sqldata->_conn);

		sqldata->_connection = mysql_real_connect(&sqldata->_conn, _host, _user, _passwd, _db, _port, (char*)NULL, 0);
		sqldata->_queryStat = mysql_set_server_option(sqldata->_connection, MYSQL_OPTION_MULTI_STATEMENTS_ON);
		if (sqldata->_connection == NULL)
		{
			LOG(L"Database", LVSYSTEM, L"Mysql connection error : %s", mysql_error(&sqldata->_conn));
			return false;
		}
		TlsSetValue(_tlsIndex, (LPVOID)sqldata);
	}


	return mysql_query(sqldata->_connection, "COMMIT") == 0;
}

bool TLSDBConnector::Rollback()
{
	SQLDATA* sqldata = (SQLDATA*)TlsGetValue(_tlsIndex);
	if (sqldata == nullptr)
	{
		sqldata = new SQLDATA;

		mysql_init(&sqldata->_conn);

		sqldata->_connection = mysql_real_connect(&sqldata->_conn, _host, _user, _passwd, _db, _port, (char*)NULL, 0);
		sqldata->_queryStat = mysql_set_server_option(sqldata->_connection, MYSQL_OPTION_MULTI_STATEMENTS_ON);
		if (sqldata->_connection == NULL)
		{
			LOG(L"Database", LVSYSTEM, L"Mysql connection error : %s", mysql_error(&sqldata->_conn));
			return false;
		}
		TlsSetValue(_tlsIndex, (LPVOID)sqldata);
	}


	return mysql_query(sqldata->_connection, "ROLLBACK") == 0;
}

bool TLSDBConnector::ExecuteSaveQuery(const WCHAR* wquery)
{
	SQLDATA* sqldata = (SQLDATA*)TlsGetValue(_tlsIndex);
	if (sqldata == nullptr)
	{
		sqldata = new SQLDATA;

		mysql_init(&sqldata->_conn);

		sqldata->_connection = mysql_real_connect(&sqldata->_conn, _host, _user, _passwd, _db, _port, (char*)NULL, 0);
		sqldata->_queryStat = mysql_set_server_option(sqldata->_connection, MYSQL_OPTION_MULTI_STATEMENTS_ON);
		if (sqldata->_connection == NULL)
		{
			LOG(L"Database", LVSYSTEM, L"Mysql connection error : %s", mysql_error(&sqldata->_conn));
			return false;
		}
		TlsSetValue(_tlsIndex, (LPVOID)sqldata);
	}

	char cquery[4096];

	WideCharToMultiByte(CP_UTF8, 0, wquery, -1, cquery, 4096, NULL, NULL);

	ULONGLONG start = GetTickCount64();
	sqldata->_queryStat = mysql_query(sqldata->_connection, cquery);
	ULONGLONG time = GetTickCount64() - start;
	if (sqldata->_queryStat != 0)
	{
		LOG(L"Database", LVSYSTEM, L"Mysql query error : %s", mysql_error(&sqldata->_conn));
		return false;
	}

	if (time >= _limitTime)
	{
		LOG(L"Database", LVSYSTEM, L"Mysql query time : %d / query : %s", time, wquery);
	}

	do {
		sqldata->_sqlResult = mysql_store_result(sqldata->_connection);
		if (sqldata->_sqlResult) {
			mysql_free_result(sqldata->_sqlResult);
		}
	} while (mysql_next_result(sqldata->_connection) == 0);

	return true;
}

bool TLSDBConnector::ExecuteSelectQuery(const WCHAR* wquery)
{
	SQLDATA* sqldata = (SQLDATA*)TlsGetValue(_tlsIndex);
	if (sqldata == nullptr)
	{
		sqldata = new SQLDATA;

		mysql_init(&sqldata->_conn);

		sqldata->_connection = mysql_real_connect(&sqldata->_conn, _host, _user, _passwd, _db, _port, (char*)NULL, 0);
		sqldata->_queryStat = mysql_set_server_option(sqldata->_connection, MYSQL_OPTION_MULTI_STATEMENTS_ON);
		if (sqldata->_connection == NULL)
		{
			LOG(L"Database", LVSYSTEM, L"Mysql connection error : %s", mysql_error(&sqldata->_conn));
			return false;
		}
		TlsSetValue(_tlsIndex, (LPVOID)sqldata);
	}

	char cquery[4096];

	WideCharToMultiByte(CP_UTF8, 0, wquery, -1, cquery, 4096, NULL, NULL);

	ULONGLONG start = GetTickCount64();
	sqldata->_queryStat = mysql_query(sqldata->_connection, cquery);
	ULONGLONG time = GetTickCount64() - start;
	if (sqldata->_queryStat != 0)
	{
		LOG(L"Database", LVSYSTEM, L"Mysql query error : %s", mysql_error(&sqldata->_conn));
		return false;
	}

	if (time >= _limitTime)
	{
		LOG(L"Database", LVSYSTEM, L"Mysql query time : %d / query : %s", time, wquery);
	}

	sqldata->_sqlResult = mysql_store_result(sqldata->_connection);

	return true;
}

void TLSDBConnector::GetQueryResult(MYSQL_RES** result)
{
	SQLDATA* sqldata = (SQLDATA*)TlsGetValue(_tlsIndex);
	*result=sqldata->_sqlResult;
}

void TLSDBConnector::Parsing(const char* txtname)
{
	//DBInfo.txt ÆÄ½ÌÇØ¿À±â
	Parser parser;
	parser.LoadFile(txtname);

	parser.GetString("DB_INFO", "host", _host, sizeof(_host));
	parser.GetString("DB_INFO", "user", _user, sizeof(_user));
	parser.GetString("DB_INFO", "passwd", _passwd, sizeof(_passwd));
	parser.GetString("DB_INFO", "db", _db, sizeof(_db));

	parser.GetValue("DB_INFO", "port", (int*)&_port);
	parser.GetValue("DB_INFO", "limitTime", (int*)&_limitTime);
}

