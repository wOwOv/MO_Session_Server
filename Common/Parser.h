#pragma once

class Parser
{
public:
	Parser();
	virtual ~Parser();

	bool LoadFile(const char* filename);//Load는 한번만 가능
	bool GetValue(const char* block,const char* name, int* val);
	bool GetString(const char* block,const char* name, char* str, int valuelength);

private:
	bool FindAlpha(const char* start, const char* end, char* alpha, char** param);
	bool FindWord(const char* tgt, const char* firstptr, char** pointer);		//찾은 word의 마지막 알파벳을 가리키는 포인터를 반환

	bool SetBlock(const char* block,char** start, char** end);		//start, end는 outparameter, block은 {}로 묶여 있음 그래서 { start, }을 end로 줌
	
	char* end() const;

private:
	char* _buffer = nullptr;
	char* _end = nullptr;
	bool _used = false;

};

