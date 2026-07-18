#ifndef PROFILER_H
#define PROFILER_H

#ifdef ENABLE_PROFILER
#define PRO_BEGIN(TagName) ProfileBegin(TagName)
#define PRO_END(TagName)   ProfileEnd(TagName)
#else
#define PRO_BEGIN(TagName) ((void)0)
#define PRO_END(TagName)   ((void)0)
#endif

void ProfileBegin(const char* szName);
void ProfileEnd(const char* szName);

//마이크로초
void ProfileDataOutText(void);

//클럭수
void ProfileDataOutTextQ(void);

void ProfileReset(void);

#define ARRMAX 20

class Profile
{
public:
	Profile(const char* tag)
	{
		PRO_BEGIN(tag);
		_tag = tag;
	}
	~Profile()
	{
		PRO_END(_tag);
	}

	const char* _tag;
};

#endif