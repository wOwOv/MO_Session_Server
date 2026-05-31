#include "DBConnector.h"
#include "C:/Program Files/MySQL/MySQL Server 8.0/include/mysql.h"
#include "C:/Program Files/MySQL/MySQL Server 8.0/include/errmsg.h"
#pragma comment(lib, "mysqlclient.lib")
#include <iostream>
#include "Logger.h"
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

	_connection = mysql_real_connect(&_conn, "127.0.0.1", "root", "ditto1234", "dbtest", 2170, (char*)NULL, 0);
	_queryStat = mysql_set_server_option(_connection, MYSQL_OPTION_MULTI_STATEMENTS_ON);
	if (_connection == NULL)
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

bool DBConnector::QuerySave(const WCHAR* String, ...)
{
	WCHAR wquery[4096];
	char cquery[4096];

	va_list va;
	va_start(va, String);
	HRESULT result = StringCchVPrintfW(wquery, 4096, String, va);
	va_end(va);

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
			mysql_free_result(_sqlResult);  // 결과 필요 없으니 바로 해제
		}
	} while (mysql_next_result(_connection) == 0);

	return true;
}

bool DBConnector::QuerySelect(const WCHAR* String, ...)
{
	WCHAR wquery[4096];
	char cquery[4096];

	va_list va;
	va_start(va, String);
	HRESULT result = StringCchVPrintfW(wquery, 4096, String, va);
	va_end(va);

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
		LOG(L"Database", LVSYSTEM, L"Mysql query time : %d / query : %s", time,wquery);
	}

	_sqlResult = mysql_store_result(_connection);

	return true;
}

void DBConnector::Parsing(const char* txtname)
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
	char port[5];
	char limitTime[5];
	int cnt = 0;

	//host parsing
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		_host[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	_host[cnt] = '\0';

	//user parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		_user[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	_user[cnt] = '\0';

	//passwd parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		_passwd[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	_passwd[cnt] = '\0';

	//db parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		_db[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	_db[cnt] = '\0';

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

	//limitTime parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		limitTime[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	limitTime[cnt] = '\0';



	//parsing한 것들 숫자로 바꿔서 주기
	_port = atoi(port);
	_limitTime = atoi(limitTime);


	delete[] buffer;

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

bool TLSDBConnector::QuerySave(const WCHAR* String, ...)
{
	SQLDATA* sqldata = (SQLDATA*)TlsGetValue(_tlsIndex);
	if (sqldata == nullptr)
	{
		sqldata = new SQLDATA;

		mysql_init(&sqldata->_conn);

		sqldata->_connection = mysql_real_connect(& sqldata->_conn, _host, _user, _passwd, _db, _port, (char*)NULL, 0);
		sqldata->_queryStat = mysql_set_server_option(sqldata->_connection, MYSQL_OPTION_MULTI_STATEMENTS_ON);
		if (sqldata->_connection == NULL)
		{
			// mysql_errno(&_MySQL);
			LOG(L"Database", LVSYSTEM, L"Mysql connection error : %s", mysql_error(&sqldata->_conn));
			return false;
		}
		TlsSetValue(_tlsIndex, (LPVOID)sqldata);
	}

	WCHAR wquery[4096];
	char cquery[4096];

	va_list va;
	va_start(va, String);
	HRESULT result = StringCchVPrintfW(wquery, 4096, String, va);
	va_end(va);

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
			mysql_free_result(sqldata->_sqlResult);  // 결과 필요 없으니 바로 해제
		}
	} while (mysql_next_result(sqldata->_connection) == 0);

	return true;
}

bool TLSDBConnector::QuerySelect(const WCHAR* String, ...)
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
			// mysql_errno(&_MySQL);
			LOG(L"Database", LVSYSTEM, L"Mysql connection error : %s", mysql_error(&sqldata->_conn));
			return false;
		}
		TlsSetValue(_tlsIndex, (LPVOID)sqldata);
	}

	WCHAR wquery[4096];
	char cquery[4096];

	va_list va;
	va_start(va, String);
	HRESULT result = StringCchVPrintfW(wquery, 4096, String, va);
	va_end(va);

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
	char port[5];
	char limitTime[5];
	int cnt = 0;

	//host parsing
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		_host[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	_host[cnt] = '\0';

	//user parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		_user[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	_user[cnt] = '\0';

	//passwd parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		_passwd[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	_passwd[cnt] = '\0';

	//db parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		_db[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	_db[cnt] = '\0';

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

	//limitTime parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		limitTime[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	limitTime[cnt] = '\0';

	

	//parsing한 것들 숫자로 바꿔서 주기
	_port = atoi(port);
	_limitTime = atoi(limitTime);


	delete[] buffer;
}

