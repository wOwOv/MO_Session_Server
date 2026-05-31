#include "PDProducer.h"

#include <process.h>
#include <psapi.h> 
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#include <string>
#include <strsafe.h>



HwPDProducer::HwPDProducer()
{
	PdhOpenQuery(NULL, NULL, &_query);
	PdhAddCounter(_query, L"\\Processor(_Total)\\% Processor Time", NULL, &_cpuTotal);
	PdhAddCounter(_query, L"\\Memory\\Available MBytes", NULL, &_availableM);
	PdhAddCounter(_query, L"\\Memory\\Pool Nonpaged Bytes", NULL, &_nonpagedM);
	SetNetworkQuery();

	PdhCollectQueryData(_query);
	_timer = (HANDLE)_beginthreadex(NULL, 0, &TimerThread, this, 0, NULL);

}

HwPDProducer::~HwPDProducer()
{
}

unsigned __stdcall HwPDProducer::TimerThread(LPVOID arg)
{
	HwPDProducer* producer = (HwPDProducer*)arg;

	while (1)
	{
		Sleep(1000);

		// 1초마다 갱신
		PdhCollectQueryData(producer->_query);

		// 갱신 데이터 얻음
		PdhGetFormattedCounterValue(producer->_cpuTotal, PDH_FMT_DOUBLE, NULL, &producer->_cpuV);
		
		PdhGetFormattedCounterValue(producer->_availableM, PDH_FMT_DOUBLE, NULL, &producer->_availableV);

		PdhGetFormattedCounterValue(producer->_nonpagedM, PDH_FMT_DOUBLE, NULL, &producer->_nonpagedV);

		unsigned long long tempreceived = 0;
		unsigned long long tempsent = 0;

		for (int iCnt = 0; iCnt < df_PDH_ETHERNET_MAX; iCnt++)
		{
			if (producer->_EthernetStruct[iCnt]._bUse)
			{
				PDH_STATUS Status;
				Status = PdhGetFormattedCounterValue(producer->_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes,PDH_FMT_DOUBLE, NULL, &producer->_EthernetStruct[iCnt]._received);
				if (Status == 0)
				{
				tempreceived += (unsigned long long)producer->_EthernetStruct[iCnt]._received.doubleValue;
				}
				Status = PdhGetFormattedCounterValue(producer->_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes,PDH_FMT_DOUBLE, NULL, &producer->_EthernetStruct[iCnt]._sent);
				if (Status == 0)
				{
					tempsent += (unsigned long long) producer->_EthernetStruct[iCnt]._sent.doubleValue;
				}
			}
		}
		producer->_received = (double)tempreceived;
		producer->_sent = (double)tempsent;

	}
}

void HwPDProducer::SetNetworkQuery()
{
	int iCnt = 0;
	bool bErr = false;
	WCHAR* szCur = NULL;
	WCHAR* szCounters = NULL;
	WCHAR* szInterfaces = NULL;
	DWORD dwCounterSize = 0, dwInterfaceSize = 0;
	WCHAR szQuery[1024] = { 0, };

	PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);
	szCounters = new WCHAR[dwCounterSize];
	szInterfaces = new WCHAR[dwInterfaceSize];

	if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS)
	{
		delete[] szCounters;
		delete[] szInterfaces;
	}

	iCnt = 0;
	szCur = szInterfaces;

	for (; *szCur != L'\0' && iCnt < df_PDH_ETHERNET_MAX; szCur += wcslen(szCur) + 1, iCnt++)
	{
		_EthernetStruct[iCnt]._bUse = true;
		_EthernetStruct[iCnt]._szName[0] = L'\0';
		wcscpy_s(_EthernetStruct[iCnt]._szName, szCur);
		szQuery[0] = L'\0';
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur);
		PdhAddCounter(_query, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes);
		szQuery[0] = L'\0';
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur);
		PdhAddCounter(_query, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes);
	}

}

PcPDProducer::PcPDProducer(HANDLE hProcess)
{
	std::wstring procName = GetCurrentProcessName();

	PdhOpenQuery(NULL, NULL, &_query);

	std::wstring pcPath = L"\\Process(" + procName + L")\\% Processor Time";
	std::wstring userPath = L"\\Process(" + procName + L")\\Private Bytes";
	std::wstring nonPath = L"\\Process(" + procName + L")\\Pool Nonpaged Bytes";
	std::wstring usingPath= L"\\Process(" + procName + L")\\Working Set - Private";
	PdhAddCounter(_query,pcPath.c_str(), NULL, &_pcTotal);
	PdhAddCounter(_query, userPath.c_str(), NULL, &_userM);
	PdhAddCounter(_query, nonPath.c_str(), NULL, &_nonpagedM);
	PdhAddCounter(_query, usingPath.c_str(), NULL, &_usingM);

	PdhCollectQueryData(_query);

	if (hProcess == INVALID_HANDLE_VALUE)
	{
		_hProcess = GetCurrentProcess();
	}

	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	_iNumberOfProcessors = SystemInfo.dwNumberOfProcessors;

	_fProcessorTotal = 0;
	_fProcessorUser = 0;
	_fProcessorKernel = 0;
	_fProcessTotal = 0;
	_fProcessUser = 0;
	_fProcessKernel = 0;
	_ftProcessor_LastKernel.QuadPart = 0;
	_ftProcessor_LastUser.QuadPart = 0;
	_ftProcessor_LastIdle.QuadPart = 0;
	_ftProcess_LastUser.QuadPart = 0;
	_ftProcess_LastKernel.QuadPart = 0;
	_ftProcess_LastTime.QuadPart = 0;
	UpdateCpuTime();








	_timer = (HANDLE)_beginthreadex(NULL, 0, &TimerThread, this, 0, NULL);

}

PcPDProducer::~PcPDProducer()
{
}

void PcPDProducer::UpdateCpuTime(void)
{
	ULARGE_INTEGER Idle;
	ULARGE_INTEGER Kernel;
	ULARGE_INTEGER User;
	//---------------------------------------------------------
	// 시스템 사용 시간을 구한다.
	//
	// 아이들 타임 / 커널 사용 타임 (아이들포함) / 유저 사용 타임
	//---------------------------------------------------------
	if (GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User) == false)
	{
		return;
	}
	// 커널 타임에는 아이들 타임이 포함됨.
	ULONGLONG KernelDiff = Kernel.QuadPart - _ftProcessor_LastKernel.QuadPart;
	ULONGLONG UserDiff = User.QuadPart - _ftProcessor_LastUser.QuadPart;
	ULONGLONG IdleDiff = Idle.QuadPart - _ftProcessor_LastIdle.QuadPart;
	ULONGLONG Total = KernelDiff + UserDiff;
	ULONGLONG TimeDiff;

	if (Total == 0)
	{
		_fProcessorUser = 0.0f;
		_fProcessorKernel = 0.0f;
		_fProcessorTotal = 0.0f;
	}
	else
	{
		// 커널 타임에 아이들 타임이 있으므로 빼서 계산.
		_fProcessorTotal = (float)((double)(Total - IdleDiff) / Total * 100.0f);
		_fProcessorUser = (float)((double)UserDiff / Total * 100.0f);
		_fProcessorKernel = (float)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
	}
	_ftProcessor_LastKernel = Kernel;
	_ftProcessor_LastUser = User;
	_ftProcessor_LastIdle = Idle;

	//---------------------------------------------------------
	// 지정된 프로세스 사용률을 갱신한다.
	//---------------------------------------------------------
	ULARGE_INTEGER None;
	ULARGE_INTEGER NowTime;
	//---------------------------------------------------------
	// 현재의 100 나노세컨드 단위 시간을 구한다. UTC 시간.
	//
	// 프로세스 사용률 판단의 공식
	//
	// a = 샘플간격의 시스템 시간을 구함. (그냥 실제로 지나간 시간)
	// b = 프로세스의 CPU 사용 시간을 구함.
	//
	// a : 100 = b : 사용률 공식으로 사용률을 구함.
	//---------------------------------------------------------
	//---------------------------------------------------------
	// 얼마의 시간이 지났는지 100 나노세컨드 시간을 구함,
	//---------------------------------------------------------
	GetSystemTimeAsFileTime((LPFILETIME)&NowTime);
	//---------------------------------------------------------
	// 해당 프로세스가 사용한 시간을 구함.
	//
	// 두번째, 세번째는 실행,종료 시간으로 미사용.
	//---------------------------------------------------------
	GetProcessTimes(_hProcess, (LPFILETIME)&None, (LPFILETIME)&None, (LPFILETIME)&Kernel, (LPFILETIME)&User);
	//---------------------------------------------------------
	// 이전에 저장된 프로세스 시간과의 차를 구해서 실제로 얼마의 시간이 지났는지 확인.
	//
	// 그리고 실제 지나온 시간으로 나누면 사용률이 나옴.
	//---------------------------------------------------------
	TimeDiff = NowTime.QuadPart - _ftProcess_LastTime.QuadPart;
	UserDiff = User.QuadPart - _ftProcess_LastUser.QuadPart;
	KernelDiff = Kernel.QuadPart - _ftProcess_LastKernel.QuadPart;
	Total = KernelDiff + UserDiff;

	_fProcessTotal = (float)(Total / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessKernel = (float)(KernelDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessUser = (float)(UserDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
	_ftProcess_LastTime = NowTime;
	_ftProcess_LastKernel = Kernel;
	_ftProcess_LastUser = User;
}

std::wstring PcPDProducer::GetCurrentProcessName()
{
	wchar_t processName[MAX_PATH] = { 0 };
	// 현재 프로세스 핸들 얻기
	HANDLE hProcess = GetCurrentProcess();

	// 모듈(= 실행파일) 이름 가져오기 (예: MyApp.exe)
	if (GetModuleBaseNameW(hProcess, NULL, processName, MAX_PATH) > 0) {
		std::wstring name(processName);
		// 확장자(.exe) 제거
		size_t pos = name.rfind(L".exe");
		if (pos != std::wstring::npos) {
			name.erase(pos);
		}
		return name; // PDH 인스턴스 이름은 확장자 없는 프로세스명
	}
	return L"";
}

unsigned __stdcall PcPDProducer::TimerThread(LPVOID arg)
{
	PcPDProducer* producer = (PcPDProducer*)arg;

	while (1)
	{
		Sleep(1000);

		// 1초마다 갱신
		PdhCollectQueryData(producer->_query);

		// 갱신 데이터 얻음
		PdhGetFormattedCounterValue(producer->_pcTotal, PDH_FMT_DOUBLE, NULL, &producer->_pcV);

		PdhGetFormattedCounterValue(producer->_userM, PDH_FMT_DOUBLE, NULL, &producer->_userV);

		PdhGetFormattedCounterValue(producer->_nonpagedM, PDH_FMT_DOUBLE, NULL, &producer->_nonpagedV);

		PdhGetFormattedCounterValue(producer->_usingM, PDH_FMT_DOUBLE, NULL, &producer->_usingV);

		producer->UpdateCpuTime();

	}
}