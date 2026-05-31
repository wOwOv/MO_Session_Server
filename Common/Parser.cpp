#include "Parser.h"
#include "stdio.h"
#include "string.h"
#include <string>

Parser::Parser()
{
}

Parser::~Parser()
{
	delete _buffer;
}

bool Parser::LoadFile(const char* filename)
{
	if (!_used)
	{
		FILE* file = nullptr;
		errno_t err = fopen_s(&file, filename, "rb");
		if (err != 0)
		{
			return false;
		}

		//버퍼에 복사
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		_buffer = new char[size];
		rewind(file);
		size_t frerr = fread(_buffer, size, 1, file);
		if (frerr == 0)
		{
			return false;
		}
		fclose(file);

		_end = &_buffer[size - 1];

		_used = true;
		return true;
	}

	printf("Already File Loaded\n");
	return false;
}

bool Parser::GetValue(const char* block, const char* name, int* val)
{

	{
		if (_buffer == nullptr)
		{
			printf("File Not Loaded\n");
			return false;
		}

		char* s = nullptr;			//start pointer
		char* e = nullptr;			//end pointer

		int blockcheck=SetBlock(block, &s, &e);
		if (!blockcheck)
		{
			return false;
		}
		//해당 범위 안에서 word를 찾아주면 : abcd 값의 포인터를 넘겨주고싶음
		char* cur = s;
		char* tgtname = new char[strlen(name) + 1];
		strcpy_s(tgtname, strlen(name) + 1, name);

		for (; cur != e; cur++)
		{
			//첫 단어 시작 같은지
			bool alphaCheck = FindAlpha(cur, e, (char*)name, &cur);
			if (alphaCheck)
			{

				bool wordCheck = FindWord(name, cur, &cur);
				if (wordCheck)
				{
					//' '나 ':'가 아닌 시작 포인터 찾아야함
					cur++;
					while (*cur != 0x0d && *cur != 0x0a)
					{
						if (*cur == ':')
						{
							break;
						}
						cur++;
					}

					if (*cur == ':')
					{
						cur++;
						if (*cur == 0x0d || *cur == 0x0a)
						{
							printf("No Value\n");
							return false;
						}

						while (*cur == ' ')
						{
							cur++;
						}

						char* valend = cur;

						while (*valend != ' ' && *valend != 0x0d && *valend != 0x0a)
						{
							valend++;
						}
						valend--;
						__int64 vallength = valend - cur + 1;
						if (vallength == 1)
						{
						
							*val = *cur - '0';
							return true;
						}
						char* temp = new char[vallength + 1];
						memcpy_s(temp, vallength + 1, cur, vallength);
						temp[vallength] = '\0';
						int check = atoi(temp);
						delete[] temp;
						if (check == 0)
						{
							printf("Value Is Not Number\n");
							return false;
						}
						*val = check;
						return true;
					}
					else
					{
						printf("No ':'\n");
						return false;
					}
				}

			}

		}




		return false;
	}
}

bool Parser::GetString(const char* block, const char* name, char* str, int strsize)
{
	if (_buffer == nullptr)
	{
		printf("File Not Loaded\n");
		return false;
	}
	char* s = nullptr;
	char* e = nullptr;

	int blockcheck = SetBlock(block, &s, &e);
	if (!blockcheck)
	{
		return false;
	}
	char* cur = s;
	char* tgtname = new char[strlen(name)+1];
	strcpy_s(tgtname, strlen(name)+1, name);

	for (; cur != e;cur++) 
	{
		//첫 단어 시작 같은지
		bool alphaCheck=FindAlpha(cur, e, (char*)name, &cur);
		if (alphaCheck)
		{

			bool wordCheck = FindWord(name, cur,  &cur);
			if (wordCheck)
			{
			//' '나 ':'가 아닌 시작 포인터 찾아야함
				cur++;
				while (*cur!=0x0d&&*cur!=0x0a)
				{
					if (*cur == ':')
					{
						break;
					}
					cur++;
				}

				if (*cur == ':')
				{
					cur++;
					if (*cur == 0x0d || *cur == 0x0a)
					{
						printf("No Value\n");
						return false;
					}

					while (*cur == ' ')
					{
						cur++;
					}

					char* valend = cur;

					while (*valend != ' ' && *valend != 0x0d && *valend != 0x0a)
					{
						valend++;
					}
					valend--;
					__int64 vallength = valend - cur + 1;
					if (strsize < vallength + 1)//data값 제외 null도 있어야해서 +1
					{
						printf("Not Enough Str Capactiy\n");
						return false;
					}
					memcpy_s(str, strsize, cur, vallength);
					str[vallength] = '\0';
					return true;
				}
				else
				{
					printf("No ':'\n");
					return false;
				}
			}

		}
		
	}




	return false;
}

bool Parser::FindAlpha(const char* start, const char* end, char* alpha,char** param)
{
	char* cur = (char*)start;
	for (; cur != end; cur++)
	{
		if (*cur == *alpha)
		{
			*param = cur;
			return true;
		}
	}
	return false;
}

bool Parser::FindWord(const char* tgt, const char* firstptr, char** pointer)
{
	char* cur = (char*)firstptr;
	while (*cur != ' ' && *cur != ':' && *cur != 0x0d && *cur != 0x0a && *cur != '{')
	{
		cur++;
	}
	cur--;
	__int64 length = cur - firstptr + 1;

	std::string temp(tgt);
	if (length != temp.length())
	{
		return false;
	}
	if (memcmp(firstptr, tgt, length) == 0)
	{
		*pointer = cur;//단어의 끝포인터
		return true;
	}

	*pointer = cur;//잘못된 단어의 끝포인터
	return false;
}

bool Parser::SetBlock(const char* block, char** start, char** end)
{
	char* s = nullptr;
	char* e = nullptr;
	char* c = _buffer;
	char* tgt = nullptr;
	char* bufend = this->end();
	for (; c != bufend; c++)
	{
		bool alphaCheck = FindAlpha(c, bufend, (char*)block, &c);
		if (alphaCheck)
		{
			bool wordCheck=FindWord(block, c, &tgt);
			if (wordCheck)
			{
			//{}찾아야함
				tgt++;
				while (*tgt != '{')
				{
					if (*tgt != ' '&& *tgt != 0x0d&& *tgt != 0x0a)
					{
						printf("Wrong Block\n");
						return false;
					}
					tgt++;
				}
				s = tgt;		//{포인터 넘겨줌
				tgt++;
				while (*tgt != '}')
				{
					if (*tgt == '{')
					{
						printf("Wrong Block\n");
						return false;
					}

					tgt++;
				}
				e = tgt;

				*start = s;
				*end = e;

				return true;
			}
			else
			{
				c++;
			}
		}
		else
		{
			break;
		}
	}

	printf("Wrong Block Name\n");
	return false;
}

char* Parser::end() const
{
	return _end;
}
