#include <iostream>
#include <windows.h>
#include "Profiler.h"
#include <vector>
#include <mutex>

using namespace std;

struct stProfile {
	long Flag = 0;				// 프로파일의 사용 여부. (배열시에만)
	char Name[64] = {};		// 프로파일 샘플 이름.
	LARGE_INTEGER StartTime = {};			// 프로파일 샘플 실행 시간.
	__int64 TotalTime = 0;		// 전체 사용시간 카운터 Time.	(출력시 호출회수로 나누어 평균 구함)
	__int64 Min[2] = { -1,-1 };			// 최소 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최소 [1] 다음 최소 [2])
	__int64 Max[2] = { 0, 0 };		// 최대 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최대 [1] 다음 최대 [2])
	__int64 Call = 0;			// 누적 호출 횟수.
	long flag = 0; //1이면 profilebegin, 0이면 안 한 것

};

struct tlspointer
{
	DWORD id;
	stProfile* profilepointer;
};

__declspec(thread) bool tlsflag = 0;
__declspec(thread) stProfile PArr[ARRMAX];


vector<tlspointer> ProfilePointer;
std::mutex g_profilePointerMutex;

int txtnum;
int txtnumQ;

//Profiling 시작하기
void ProfileBegin(const char* szName)
{
	if (tlsflag == 0)
	{
		tlspointer box;
		box.id = GetCurrentThreadId();
		box.profilepointer = PArr;
		{
			std::lock_guard<std::mutex> lock(g_profilePointerMutex);
			ProfilePointer.push_back(box);
		}
		tlsflag = 1;
	}

	int idx = 0;
	for (; idx < ARRMAX; idx++)
	{
		if (PArr[idx].Flag)
		{
			//if (PArr[idx].Call == 0)
			//{
			//	throw 1;
			//	//DebugBreak();
			//}
			if (strcmp(PArr[idx].Name, szName) == 0)
			{
				//중복 profilebegin확인
				if (PArr[idx].flag != 0)
				{
					DebugBreak();
				}
				PArr[idx].flag = 1;

				QueryPerformanceCounter(&PArr[idx].StartTime);
				break;
			}
		}
	}

	if (idx == ARRMAX)
	{
		for (idx = 0; idx < ARRMAX; idx++)
		{
			if (!PArr[idx].Flag)
			{
				PArr[idx].Flag = true;
				strcpy_s(PArr[idx].Name, sizeof(PArr[idx].Name), szName);
				QueryPerformanceCounter(&PArr[idx].StartTime);

				//중복 profilebegin확인
				if (PArr[idx].flag != 0)
				{
					DebugBreak();
				}
				PArr[idx].flag = 1;
				break;
			}
		}
	}
}


//Profiling 끝내기
void ProfileEnd(const char* szName)
{
	int idx = 0;
	for (; idx < ARRMAX; idx++)
	{
		if (PArr[idx].Flag)
		{
			if (strcmp(PArr[idx].Name, szName) == 0)
			{
				break;
			}
		}
	}
	if (idx == ARRMAX)
	{
		//throw 1;
		DebugBreak();
	}
	//총시간에 걸린 시간 더하기
	LARGE_INTEGER End;
	QueryPerformanceCounter(&End);
	LARGE_INTEGER Time;
	Time.QuadPart = End.QuadPart - PArr[idx].StartTime.QuadPart;

	//중복 profileend 확인
	if (PArr[idx].flag != 1)
	{
		DebugBreak();
	}
	PArr[idx].flag = 0;

	PArr[idx].TotalTime += Time.QuadPart;

	//최소값이면 데이터 넣기
	if (PArr[idx].Min[0] == -1)
	{
		PArr[idx].Min[0] = Time.QuadPart;
	}
	else if (PArr[idx].Min[1] == -1)
	{
		PArr[idx].Min[1] = Time.QuadPart;
	}
	else
	{
		if (PArr[idx].Min[0] > Time.QuadPart)
		{
			PArr[idx].Min[0] = Time.QuadPart;
		}
		else if (PArr[idx].Min[1] > Time.QuadPart)
		{
			PArr[idx].Min[1] = Time.QuadPart;
		}
	}
	//최대값이면 데이터 넣기
	if (PArr[idx].Max[0] < Time.QuadPart)
	{
		PArr[idx].Max[0] = Time.QuadPart;
	}
	else if (PArr[idx].Max[1] < Time.QuadPart)
	{
		PArr[idx].Max[1] = Time.QuadPart;
	}

	PArr[idx].Call++;


}






//Profiling 데이터 txt로 출력
// Min 2개, Max 2개를 제외한 평균을 계산하므로
// Call이 4 초과로 충분히 누적된 뒤에만 출력해야 한다.
void ProfileDataOutText(void)
{

	char txtname[40];
	char threadidtxt[11];
	char txtnumstring[4];
	char txttitle[16] = "_Profiling_";

	{
		std::lock_guard<std::mutex> lock(g_profilePointerMutex);
		int size = ProfilePointer.size();
		for (int i = 0; i < size; i++)
		{
			tlspointer box = ProfilePointer[i];
			_itoa_s(box.id, threadidtxt, 10);
			_itoa_s(txtnum, txtnumstring, 10);

			strcpy_s(txtname, threadidtxt);
			strcat_s(txtname, txttitle);
			strcat_s(txtname, txtnumstring);
			strcat_s(txtname, ".txt");

			LARGE_INTEGER Freq;
			QueryPerformanceFrequency(&Freq);
			FILE* txtfile;
			fopen_s(&txtfile, txtname, "wt");
			if (txtfile != 0)
			{
				fprintf(txtfile, "----------------------------------------------------------------------------------\n");
				fprintf(txtfile, "       Name      |     Average    |      Min     |      Max     |      Call      \n");
				fprintf(txtfile, "----------------------------------------------------------------------------------\n");
				int idx = 0;
				for (; idx < ARRMAX; idx++)
				{
					if (box.profilepointer[idx].Flag)
					{
						float avg = static_cast<float>(box.profilepointer[idx].TotalTime - box.profilepointer[idx].Max[0] - box.profilepointer[idx].Max[1] - box.profilepointer[idx].Min[0] - box.profilepointer[idx].Min[1]) / Freq.QuadPart * 1000000 / (box.profilepointer[idx].Call - 4);
						fprintf(txtfile, "%17s|%14.4f㎲|%12.4f㎲|%12.4f㎲|%14d\n",
							box.profilepointer[idx].Name, avg, static_cast<float>(box.profilepointer[idx].Min[0] + box.profilepointer[idx].Min[1]) * 1000000 / 2 / Freq.QuadPart,
							static_cast<float>(box.profilepointer[idx].Max[0] + box.profilepointer[idx].Max[1]) * 1000000 / 2 / Freq.QuadPart,
							static_cast<int>(box.profilepointer[idx].Call));
					}
					if (!box.profilepointer[idx].Flag)
					{
						fprintf(txtfile, "-------------------------------------------------------------------------------\n");
						break;
					}
				}

				fclose(txtfile);
			}
		}
	}
	txtnum++;
}



// Min 2개, Max 2개를 제외한 평균을 계산하므로
// Call이 4 초과로 충분히 누적된 뒤에만 출력해야 한다.
void ProfileDataOutTextQ(void)
{
	char txtname[40];
	char threadidtxt[11];
	char txtnumstring[4];
	char txttitle[16] = "_ProfilingQ_";

	{
		std::lock_guard<std::mutex> lock(g_profilePointerMutex);
		int size = ProfilePointer.size();
		for (int i = 0; i < size; i++)
		{
			tlspointer box = ProfilePointer[i];
			_itoa_s(box.id, threadidtxt, 10);
			_itoa_s(txtnumQ, txtnumstring, 10);

			strcpy_s(txtname, threadidtxt);
			strcat_s(txtname, txttitle);
			strcat_s(txtname, txtnumstring);
			strcat_s(txtname, ".txt");

			FILE* txtfile;
			fopen_s(&txtfile, txtname, "wt");
			if (txtfile != 0)
			{
				fprintf(txtfile, "----------------------------------------------------------------------------------\n");
				fprintf(txtfile, "       Name      |     Average    |      Min     |      Max     |      Call      \n");
				fprintf(txtfile, "----------------------------------------------------------------------------------\n");
				int idx = 0;
				for (; idx < ARRMAX; idx++)
				{
					if (box.profilepointer[idx].Flag)
					{
						float avg = static_cast<float>(box.profilepointer[idx].TotalTime - box.profilepointer[idx].Max[0] - box.profilepointer[idx].Max[1] - box.profilepointer[idx].Min[0] - box.profilepointer[idx].Min[1]) / (box.profilepointer[idx].Call - 4);
						fprintf(txtfile, "%17s|%14.4f㎲|%12.4f㎲|%12.4f㎲|%14d\n",
							box.profilepointer[idx].Name, avg, static_cast<float>(box.profilepointer[idx].Min[0] + box.profilepointer[idx].Min[1]) * 1000000 / 2,
							static_cast<float>(box.profilepointer[idx].Max[0] + box.profilepointer[idx].Max[1]) * 1000000 / 2,
							static_cast<int>(box.profilepointer[idx].Call));
					}
					if (!box.profilepointer[idx].Flag)
					{
						fprintf(txtfile, "-------------------------------------------------------------------------------\n");
						break;
					}
				}

				fclose(txtfile);
			}
		}
		txtnumQ++;
	}
}

//프로파일링된 데이터 초기화(태그 제외)
void ProfileReset(void)
{

	{
		std::lock_guard<std::mutex> lock(g_profilePointerMutex);
		int size = ProfilePointer.size();

		for (int i = 0; i < size; i++)
		{
			tlspointer box = ProfilePointer[i];

			for (int j = 0; j < ARRMAX; j++)
			{
				if (box.profilepointer[j].Flag)
				{
					//box.profilepointer[j].Flag = false;
					//box.profilepointer[j].StartTime.QuadPart = 0;
					box.profilepointer[j].TotalTime = 0;
					box.profilepointer[j].Min[0] = -1;
					box.profilepointer[j].Min[1] = -1;
					box.profilepointer[j].Max[0] = 0;
					box.profilepointer[j].Max[1] = 0;
					box.profilepointer[j].Call = 0;
				}
			}
		}
	}
}