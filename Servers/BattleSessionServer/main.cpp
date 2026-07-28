#include "FighterServer.h"
#include "CrashDump.h"
#include "Logger.h"

CrashDump dumpit;

int main()
{
	Logger* logger = Logger::GetInstance();
	logger->SetDirectory(L"Debug");
	logger->SetLogLevel(LVSYSTEM);

	
	auto server = std::make_unique<FighterServer>(); 
	if (!server->FighterServerStart("FighterServerConfig.cnf"))
	{
		return 1;
	}

	while (!server->IsShutDownRequested())
	{
		server->ShowServerInfo();
		server->ServerControl();
		Sleep(1000);
	}
	LOG(L"SYSTEM", LVSYSTEM, L"FighterServer ShutDown");
}