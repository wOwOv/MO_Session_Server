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

	while (1)
	{
		Sleep(1000);
		printf("****************************************\n");
		printf("Session: %d\n", server->GetSessionCount());
		printf("BufferCapacity: %d\n", server->GetSBufferCapacity());
		printf("BufferUsing: %d\n", server->GetSBufferUsingCount());
		printf("RecvTPS: %d\n", server->GetRecvMessageTPS());
		printf("SendTPS: %d\n", server->GetSendMessageTPS());
		printf("\n\n");
		printf("FightResourceCapacity: %d\n", server->GetFightPoolCapacity());
		printf("FightResourceUsing: %d\n", server->GetFightPoolUsingCount());
		printf("PlayerPoolCapacity: %d\n", server->GetPlayerPoolCapacity());
		printf("PlayerPoolUsing: %d\n", server->GetPlayerPoolUsingCount());
		printf("ControlPoolCapacity: %d\n", server->GetControlPoolCapacity());
		printf("ControlPoolUsing: %d\n", server->GetControlPoolUsingCount());
		printf("Player: %d\n", server->GetPlayerCount());
		printf("****************************************\n\n");

	}
}