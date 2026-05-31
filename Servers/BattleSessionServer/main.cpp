#include "FighterServer.h"
#include "CrashDump.h"
#include "Logger.h"

CrashDump dumpit;

int main()
{
	Logger* logger = Logger::GetInstance();
	logger->SetDirectory(L"Debug");
	logger->SetLogLevel(LVSYSTEM);

	ContentsServer* server = new FighterServer;
	server->Start("FighterServerConfig.cnf");
	FighterServer* fserver = (FighterServer*)server;

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
		printf("FightResourceCapacity: %d\n", fserver->GetFightPoolCapacity());
		printf("FightResourceUsing: %d\n", fserver->GetFightPoolUsingCount());
		printf("PlayerPoolCapacity: %d\n", fserver->GetPlayerPoolCapacity());
		printf("PlayerPoolUsing: %d\n", fserver->GetPlayerPoolUsingCount());
		printf("ControlPoolCapacity: %d\n", fserver->GetControlPoolCapacity());
		printf("ControlPoolUsing: %d\n", fserver->GetControlPoolUsingCount());
		printf("Player: %d\n", fserver->GetPlayerCount());
		printf("****************************************\n\n");

	}
}