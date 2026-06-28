#include "FighterServer.h"
#include "CrashDump.h"
#include "Logger.h"

CrashDump dumpit;

int main()
{
	Logger* logger = Logger::GetInstance();
	logger->SetDirectory(L"Debug");
	logger->SetLogLevel(LVSYSTEM);

	FighterServer* server = new FighterServer;
	server->FighterServerStart("FighterServerConfig.cnf");

	while (!server->IsShutDownRequested())
	{
		server->ShowServerInfo();
		server->ServerControl();
		Sleep(1000);
	}
	LOG(L"SYSTEM", LVSYSTEM, L"FighterServer ShutDown");
	delete server;
}