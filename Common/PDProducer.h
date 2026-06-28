#ifndef __PDPRODUCER__
#define __PDPRODUCER__

#include <Pdh.h>
#pragma comment(lib,"Pdh.lib")
#include <string>

#define df_PDH_ETHERNET_MAX 8

class HwPDProducer
{
private:
	struct st_ETHERNET
	{
		bool _bUse;
		WCHAR _szName[128];
		PDH_HCOUNTER _pdh_Counter_Network_RecvBytes;
		PDH_FMT_COUNTERVALUE _received;
		PDH_HCOUNTER _pdh_Counter_Network_SendBytes;
		PDH_FMT_COUNTERVALUE _sent;
	};
public:
	HwPDProducer();
	~HwPDProducer();



	double GetCpuTotal(void) { return _cpuV.doubleValue; }
	double GetAvailableM(void) { return _availableV.doubleValue; }
	double GetNonpagedM(void) { return _nonpagedV.doubleValue; }
	double GetSent(void) { return _sent; }
	double GetReceived(void) { return _received; }

private:
static unsigned __stdcall TimerThread(LPVOID arg);
void SetNetworkQuery();

private:
	HANDLE _timer;
	PDH_HQUERY _query;
		
private:
	PDH_HCOUNTER _cpuTotal;
	PDH_FMT_COUNTERVALUE _cpuV;
	PDH_HCOUNTER _availableM;
	PDH_FMT_COUNTERVALUE _availableV;
	PDH_HCOUNTER _nonpagedM;
	PDH_FMT_COUNTERVALUE _nonpagedV;
	st_ETHERNET _EthernetStruct[df_PDH_ETHERNET_MAX];
	double _received;
	double _sent;
};



class PcPDProducer
{
public:
	PcPDProducer(HANDLE hProcess = INVALID_HANDLE_VALUE);
	~PcPDProducer();

	double GetPcCpu(void) { return _pcV.doubleValue; }
	double GetUserM(void) { return _userV.doubleValue; }
	double GetNonpagedM(void) { return _nonpagedV.doubleValue; }
	double GetUsingM(void) { return _usingV.doubleValue; }

	void UpdateCpuTime(void);
	float ProcessorTotal(void) { return _fProcessorTotal; }
	float ProcessorUser(void) { return _fProcessorUser; }
	float ProcessorKernel(void) { return _fProcessorKernel; }
	float ProcessTotal(void) { return _fProcessTotal; }
	float ProcessUser(void) { return _fProcessUser; }
	float ProcessKernel(void) { return _fProcessKernel; }




private:
	std::wstring GetCurrentProcessName();



private:
	static unsigned __stdcall TimerThread(LPVOID arg);


private:
	HANDLE _timer;
	PDH_HQUERY _query;

private:
	PDH_HCOUNTER _pcTotal;
	PDH_FMT_COUNTERVALUE _pcV;
	PDH_HCOUNTER _userM;
	PDH_FMT_COUNTERVALUE _userV;
	PDH_HCOUNTER _nonpagedM;
	PDH_FMT_COUNTERVALUE _nonpagedV;
	PDH_HCOUNTER _usingM;
	PDH_FMT_COUNTERVALUE _usingV;


	HANDLE _hProcess;
	int _iNumberOfProcessors;
	float _fProcessorTotal;
	float _fProcessorUser;
	float _fProcessorKernel;
	float _fProcessTotal;
	float _fProcessUser;
	float _fProcessKernel;
	ULARGE_INTEGER _ftProcessor_LastKernel;
	ULARGE_INTEGER _ftProcessor_LastUser;
	ULARGE_INTEGER _ftProcessor_LastIdle;
	ULARGE_INTEGER _ftProcess_LastKernel;
	ULARGE_INTEGER _ftProcess_LastUser;
	ULARGE_INTEGER _ftProcess_LastTime;
};


#endif