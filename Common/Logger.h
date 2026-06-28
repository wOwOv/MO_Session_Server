#pragma once

#include <windows.h>
#include <mutex>

#define LOG(Type, Level, Format, ...) \
    Logger::GetInstance()->Log(Type, Level, Format, __VA_ARGS__)

enum LOG_LEVEL {LVDEBUG=0,LVERROR,LVSYSTEM};

class Logger
{
public:

	static Logger* GetInstance();

	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;
	Logger(Logger&&) = delete;
	Logger& operator=(Logger&&) = delete;

	void Log(const WCHAR* Type, LOG_LEVEL Level, const WCHAR* String, ...);
	void LogHex(WCHAR* Type, LOG_LEVEL Level, WCHAR* Category, WCHAR* Log, BYTE* Byte, int ByteLen);

	void SetDirectory(const WCHAR* Directory);
	void SetLogLevel(LOG_LEVEL Level);

private:
	Logger();

	static Logger* _Logger;
	static std::mutex _key;


private:
	WCHAR* _Directory;
	LOG_LEVEL _Level=LVSYSTEM;
	unsigned long long _LogCount=0;

	
};