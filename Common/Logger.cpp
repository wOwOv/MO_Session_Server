#include "Logger.h"
#include <strsafe.h>
#include <chrono>
#include <mutex>
#include <windows.h>



unsigned long long _logCount = 0;
LOG_LEVEL _logLevel = LVDEBUG;
std::mutex key;

namespace { constexpr int LOG_MAX = 1024; }

Logger* Logger::_Logger = nullptr;
std::mutex Logger::_key;


Logger* Logger::GetInstance()
{
	//더블 체킹 락 패턴으로 멀티 스레드 환경에서도
	//싱글톤 객체의 유일성을 보장함.
	static Logger instance;
	return &instance;
}

void Logger::Log(const WCHAR* Type, LOG_LEVEL Level, const WCHAR* String, ...)
{
	if (_Level > Level)
	{
		return;
	}
	time_t now = std::time(nullptr);
	tm ymd;
	localtime_s(&ymd, &now);

	WCHAR filename[256];
	StringCchPrintfW(filename, 256, L".\\%ls\\%ls_%02d_%02d.txt", _Directory, Type, ymd.tm_year + 1900, ymd.tm_mon + 1);


	WCHAR log[LOG_MAX];
	WCHAR buffer[LOG_MAX * 5];

	va_list va;
	va_start(va, String);
	HRESULT result = StringCchVPrintfW(log, LOG_MAX, String, va);
	va_end(va);

	HRESULT lastresult;
	switch (Level)
	{
	case LVDEBUG:
		lastresult= StringCchPrintfW(buffer, LOG_MAX * 5, L"[%s] [%d-%02d-%02d %02d:%02d:%02d] [%lld] [DEBUG] %ls \n",
			Type, ymd.tm_year + 1900, ymd.tm_mon + 1, ymd.tm_mday, ymd.tm_hour, ymd.tm_min, ymd.tm_sec, InterlockedIncrement(&_logCount), log);
		break;
	case LVERROR:
		lastresult = StringCchPrintfW(buffer, LOG_MAX * 5, L"[%s] [%d-%02d-%02d %02d:%02d:%02d] [%lld] [ERROR] %ls \n",
			Type, ymd.tm_year + 1900, ymd.tm_mon + 1, ymd.tm_mday, ymd.tm_hour, ymd.tm_min, ymd.tm_sec, InterlockedIncrement(&_logCount), log);
		break;
	case LVSYSTEM:
		lastresult = StringCchPrintfW(buffer, LOG_MAX * 5, L"[%s] [%d-%02d-%02d %02d:%02d:%02d] [%lld] [SYSTEM] %ls \n",
			Type, ymd.tm_year + 1900, ymd.tm_mon + 1, ymd.tm_mday, ymd.tm_hour, ymd.tm_min, ymd.tm_sec, InterlockedIncrement(&_logCount), log);
		break;
	}

	if (lastresult != S_OK)
	{
		if (lastresult == STRSAFE_E_INVALID_PARAMETER)
		{
			Log(L"ERROR", LVERROR, L"LOG : STRSAFE_E_INVALID_PARAMETER");
		}
		if (lastresult == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			Log(L"ERROR", LVERROR, L"LOG : STRSAFE_E_INSUFFICIENT_BUFFER");
		}
	
	}


	FILE* file;
	_wfopen_s(&file, filename, L"a+, ccs=UTF-8");


	if (file != nullptr)
	{
		fwprintf(file, L"%ls", buffer);
		fclose(file);
	}
}

void Logger::SetDirectory(const WCHAR* Directory)
{
	_Directory = const_cast<WCHAR*>(Directory);
	CreateDirectory(Directory, NULL);
}

void Logger::SetLogLevel(LOG_LEVEL Level)
{
	_Level = Level;
}

Logger::Logger()
{
	_Directory = nullptr;
}
