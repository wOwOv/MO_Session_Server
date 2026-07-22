#include <iostream>
#include <windows.h>
#include "Profiler.h"
#include <vector>
#include <mutex>
#include <array>
#include <map>
#include <memory>
#include <string>
#include <atomic>
#include <cstring>

struct stProfile {
	long Flag = 0;				// 프로파일의 사용 여부. (배열시에만)
	char Name[64] = {};		// 프로파일 샘플 이름.
	LARGE_INTEGER StartTime = {};			// 프로파일 샘플 실행 시간.
	__int64 TotalTime = 0;		// 전체 사용시간 카운터 Time.	(출력시 호출회수로 나누어 평균 구함)
	__int64 Min = -1;			// 최소 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최소 [1] 다음 최소 [2])
	__int64 Max = 0;		// 최대 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최대 [1] 다음 최대 [2])
	__int64 Call = 0;			// 누적 호출 횟수.
	long IsRunning = 0;

};

struct ThreadProfileState
{
	DWORD threadId = 0;
	std::mutex mutex;
	std::array<stProfile, ARRMAX> profiles{};
};

__declspec(thread) ThreadProfileState* g_tlsProfile = nullptr;

std::mutex g_profileRegistryMutex;
std::vector<std::unique_ptr<ThreadProfileState>> g_threadProfiles;
std::atomic<long> g_profileOutputNumber{ 0 };

namespace
{
	ThreadProfileState& GetThreadProfileState()
	{
		if (g_tlsProfile != nullptr)
		{
			return *g_tlsProfile;
		}

		auto state = std::make_unique<ThreadProfileState>();
		state->threadId = GetCurrentThreadId();

		ThreadProfileState* rawState = state.get();

		{
			std::lock_guard<std::mutex> lock(g_profileRegistryMutex);
			g_threadProfiles.push_back(std::move(state));
		}

		g_tlsProfile = rawState;
		return *g_tlsProfile;
	}
}


//Profiling 시작하기
void ProfileBegin(const char* name)
{
	ThreadProfileState& state = GetThreadProfileState();
	std::lock_guard<std::mutex> lock(state.mutex);

	for (stProfile& profile : state.profiles)
	{
		if (profile.Flag != 0 && strcmp(profile.Name, name) == 0)
		{
			if (profile.IsRunning != 0)
			{
				DebugBreak();
			}

			profile.IsRunning = 1;
			QueryPerformanceCounter(&profile.StartTime);
			return;
		}
	}

	for (stProfile& profile : state.profiles)
	{
		if (profile.Flag == 0)
		{
			profile.Flag = 1;
			strcpy_s(profile.Name, name);
			profile.IsRunning = 1;
			QueryPerformanceCounter(&profile.StartTime);
			return;
		}
	}

	DebugBreak(); // ARRMAX 슬롯 부족
}


//Profiling 끝내기
void ProfileEnd(const char* name)
{
	LARGE_INTEGER endTime;
	QueryPerformanceCounter(&endTime);

	ThreadProfileState& state = GetThreadProfileState();
	std::lock_guard<std::mutex> lock(state.mutex);

	for (stProfile& profile : state.profiles)
	{
		if (profile.Flag == 0 || strcmp(profile.Name, name) != 0)
		{
			continue;
		}

		if (profile.IsRunning == 0)
		{
			DebugBreak();
		}

		const __int64 elapsed = endTime.QuadPart - profile.StartTime.QuadPart;

		profile.IsRunning = 0;
		profile.TotalTime += elapsed;

		if (profile.Min == -1 || elapsed < profile.Min)
		{
			profile.Min = elapsed;
		}

		if (elapsed > profile.Max)
		{
			profile.Max = elapsed;
		}

		++profile.Call;
		return;
	}

	DebugBreak(); // Begin 없이 End 호출
}





struct ProfileSummary
{
	__int64 totalTime = 0;
	__int64 minTime = -1;
	__int64 maxTime = 0;
	__int64 callCount = 0;
};

void ProfileDataOutText()
{
	std::vector<ThreadProfileState*> states;

	{
		std::lock_guard<std::mutex> lock(g_profileRegistryMutex);

		for (const auto& state : g_threadProfiles)
		{
			states.push_back(state.get());
		}
	}

	std::map<std::string, ProfileSummary> summaries;

	for (ThreadProfileState* state : states)
	{
		std::lock_guard<std::mutex> lock(state->mutex);

		for (const stProfile& profile : state->profiles)
		{
			if (profile.Flag == 0 || profile.Call == 0)
			{
				continue;
			}

			ProfileSummary& summary = summaries[profile.Name];

			summary.totalTime += profile.TotalTime;
			summary.callCount += profile.Call;

			if (summary.minTime == -1 || profile.Min < summary.minTime)
			{
				summary.minTime = profile.Min;
			}

			if (profile.Max > summary.maxTime)
			{
				summary.maxTime = profile.Max;
			}
		}
	}

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	const long outputNumber = g_profileOutputNumber.fetch_add(1);

	char fileName[64] = {};
	sprintf_s(fileName, "Profiling_%ld.txt", outputNumber);

	FILE* file = nullptr;
	fopen_s(&file, fileName, "wt");
	if (file == nullptr)
	{
		return;
	}

	fprintf(file, "Name|Average(us)|Min(us)|Max(us)|Call\n");

	for (const auto& entry : summaries)
	{
		const std::string& name = entry.first;
		const ProfileSummary& summary = entry.second;

		if (summary.callCount <= 4)
		{
			continue;
		}

		const double averageUs =
			static_cast<double>(summary.totalTime) * 1000000.0 /
			static_cast<double>(frequency.QuadPart) /
			static_cast<double>(summary.callCount);

		const double minUs =
			static_cast<double>(summary.minTime) * 1000000.0 /
			static_cast<double>(frequency.QuadPart);

		const double maxUs =
			static_cast<double>(summary.maxTime) * 1000000.0 /
			static_cast<double>(frequency.QuadPart);

		fprintf(
			file,
			"%-28s | %14.3f | %14.3f | %14.3f | %12lld\n",
			name.c_str(),
			averageUs,
			minUs,
			maxUs,
			summary.callCount);
	}

	fclose(file);
}

