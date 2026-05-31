/*
#include <vector>
#include<iostream>
#include<string>
#include<cctype>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

struct Parameter
{
	string _dataType;		//자료형 문자열
	string _variable;		//변수
};
struct OneFunction
{
	string line;
	char _type = -1;			//0:SC, 1:CS
	string _functionName;
	vector<Parameter> _paramA;
	int _num;					//packetnum
};

struct OneClass
{
	string _className;
	string block;			//{~}범위 string
	vector<OneFunction> _funcA;
};

int main(int argc, char* argv[])
{
	for (int i = 1; i < argc; i++)
	{
		//프로토콜 파일 open
		FILE* file;
		fopen_s(&file, argv[i], "rb");
		if (file == NULL)
		{
			printf("fopen error\n");
		}

		//버퍼에 통으로 읽어오기
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		char* buffer = new char[size + 1];
		rewind(file);
		int error = fread(buffer, size, 1, file);
		if (error == 0)
		{
			printf("fread error\n");
			return 0;
		}
		fclose(file);
		buffer[size] = '\0';

		string txtB = buffer;
		string Remain = buffer;
		vector<OneClass> ClassArray;

		//파일 이름들 만들기
		string filename;
		int fs = Remain.find('#');
		Remain = Remain.substr(fs + 1);
		int fe = Remain.find('#');
		filename = Remain.substr(0, fe);
		Remain = Remain.substr(fe + 1);

		//폴더 생성
		string outDir = "RPC_" + filename;
		fs::create_directories(outDir);

		string def = filename + "Define.h";
		string proxyh = filename + "Proxy.h";
		string proxyc = filename + "Proxy.cpp";
		string stubh = filename + "Stub.h";
		string stubc = filename + "Stub.cpp";

		string deff = outDir + "\\" + filename + "Define.h";
		string proxyhf = outDir + "\\" + filename + "Proxy.h";
		string proxycf = outDir + "\\" + filename + "Proxy.cpp";
		string stubhf = outDir + "\\" + filename + "Stub.h";
		string stubcf = outDir + "\\" + filename + "Stub.cpp";


		//기본 세팅 해두기
		FILE* fdefineh;
		FILE* fproxyh;
		FILE* fproxyc;
		FILE* fstubh;
		FILE* fstubc;
		fopen_s(&fdefineh, deff.c_str(), "wb");
		fprintf(fdefineh, "#pragma once\n\n\n");

		fopen_s(&fproxyh, proxyhf.c_str(), "wb");
		fprintf(fproxyh, "#pragma once\n");
		fprintf(fproxyh, "#include \"IRPC.h\"\n\n");

		fopen_s(&fproxyc, proxycf.c_str(), "wb");
		fprintf(fproxyc, "#include \"%s\"\n", def);
		fprintf(fproxyc, "#include \"%s\"\n", proxyh);
		fprintf(fproxyc, "#include \"ContentsServer.h\"\n");
		fprintf(fproxyc, "#include \"CPacket.h\"\n\n");

		fopen_s(&fstubh, stubhf.c_str(), "wb");
		fprintf(fstubh, "#pragma once\n");
		fprintf(fstubh, "#include \"IRPC.h\"\n\n");

		fopen_s(&fstubc, stubcf.c_str(), "wb");
		fprintf(fstubc, "#include \"%s\"\n", def);
		fprintf(fstubc, "#include \"%s\"\n", stubh);
		fprintf(fstubc, "#include \"CPacket.h\"\n\n");



		//컨텐츠 하나씩 뗴어내기
		while (Remain.find('<') != -1)
		{
			OneClass One;
			int nameS = Remain.find('<');
			int nameE = Remain.find('>');
			One._className = Remain.substr(nameS + 1, nameE - nameS - 1);
			int blockS = Remain.find('{');
			int blockE = Remain.find('}');
			One.block = Remain.substr(blockS, blockE);
			ClassArray.push_back(One);

			Remain = Remain.substr(blockE + 1);
		}

		//함수 한줄씩 떼어내기
		for (int j = 0; j < ClassArray.size(); j++)
		{
			string tempblock = ClassArray[j].block;
			int linestart = tempblock.find('\n') + 1;
			tempblock = tempblock.substr(linestart);
			while (1)
			{
				if (*tempblock.c_str() == '}')
				{
					break;
				}
				int lineend = tempblock.find('\n');
				OneFunction func;
				func.line = tempblock.substr(0, lineend);
				ClassArray[j]._funcA.push_back(func);
				tempblock = tempblock.substr(lineend + 1);
				if (*tempblock.c_str() == '}')
				{
					break;
				}
			}
		}

		//인자 하나씩 떼어내기
		for (int j = 0; j < ClassArray.size(); j++)
		{
			for (int f = 0; f < ClassArray[j]._funcA.size(); f++)
			{
				string templine = ClassArray[j]._funcA[f].line;
				//proxy
				if (templine.find("SC") != -1)
				{
					ClassArray[j]._funcA[f]._type = 0;
					int index = templine.find("SC");
					templine = templine.substr(index + 2);
					int eindex = templine.find('(');
					string tempfname = templine.substr(0, eindex);
					tempfname.erase(find(tempfname.begin(), tempfname.end(), ' '));
					ClassArray[j]._funcA[f]._functionName = tempfname;
					templine = templine.substr(eindex + 1);
					while (1)
					{
						string dataparam;
						int end;
						end = templine.find(',');
						if (end != -1)
						{
							dataparam = templine.substr(0, end);
							templine = templine.substr(end + 1);
							int divide = dataparam.find_last_of(' ');
							Parameter param;
							param._dataType = dataparam.substr(0, divide);
							param._variable = dataparam.substr(divide + 1);
							ClassArray[j]._funcA[f]._paramA.push_back(param);
						}
						else
						{
							end = templine.find(')');
							dataparam = templine.substr(0, end);
							templine = templine.substr(end + 1);
							int divide = dataparam.find_last_of(' ');
							Parameter param;
							param._dataType = dataparam.substr(0, divide);
							param._variable = dataparam.substr(divide + 1);
							ClassArray[j]._funcA[f]._paramA.push_back(param);
							break;
						}
					}
					int nums = templine.find('#');
					templine = templine.substr(nums + 1);
					int last;
					if (templine.find(0x0d) != -1)
					{
						last = templine.find(0x0d);
					}
					else
					{
						last = templine.find('\n');
					}
					string num = templine.substr(0, last);
					ClassArray[j]._funcA[f]._num = stoi(num);
				}
				//stub
				if (templine.find("CS") != -1)
				{
					ClassArray[j]._funcA[f]._type = 1;
					int index = templine.find("CS");
					templine = templine.substr(index + 2);
					int eindex = templine.find('(');
					string tempfname = templine.substr(0, eindex);
					tempfname.erase(find(tempfname.begin(), tempfname.end(), ' '));
					ClassArray[j]._funcA[f]._functionName = tempfname;
					templine = templine.substr(eindex + 1);
					while (1)
					{
						string dataparam;
						int end;
						end = templine.find(',');
						if (end != -1)
						{
							dataparam = templine.substr(0, end);
							templine = templine.substr(end + 1);
							int divide = dataparam.find_last_of(' ');
							Parameter param;
							param._dataType = dataparam.substr(0, divide);
							param._variable = dataparam.substr(divide + 1);
							ClassArray[j]._funcA[f]._paramA.push_back(param);
						}
						else
						{
							end = templine.find(')');
							dataparam = templine.substr(0, end);
							templine = templine.substr(end + 1);
							int divide = dataparam.find_last_of(' ');
							Parameter param;
							param._dataType = dataparam.substr(0, divide);
							param._variable = dataparam.substr(divide + 1);
							ClassArray[j]._funcA[f]._paramA.push_back(param);
							break;
						}
					}
					int nums = templine.find('#');
					templine = templine.substr(nums + 1);
					int last;
					if (templine.find(0x0d) != -1)
					{
						last = templine.find(0x0d);
					}
					else
					{
						last = templine.find('\n');
					}
					string num = templine.substr(0, last);
					ClassArray[j]._funcA[f]._num = stoi(num);
				}
			}


			//}


		}

		//소스코드 작성
		for (int j = 0; j < ClassArray.size(); j++)
		{
			fprintf(fproxyh, "class %sProxy : public IProxy\n{\npublic:\n", ClassArray[j]._className);
			fprintf(fstubh, "class %sStub : public IStub\n{\npublic:\n", ClassArray[j]._className);
			fprintf(fstubh, "virtual void ProcMessage(__int64 sessionID,CPacket packet) override;\n");
			fprintf(fstubh, "protected:\n");
			fprintf(fstubc, "void %sStub::ProcMessage(__int64 sessionID, CPacket packet)\n{\n", ClassArray[j]._className);
			fprintf(fstubc, "unsigned char type;\npacket >> type;\nswitch (type)\n{\n");
			for (int f = 0; f < ClassArray[j]._funcA.size(); f++)
			{
				string defmes = ClassArray[j]._funcA[f]._functionName;
				for (int d = 0; d < defmes.length(); d++)
				{
					char* temp = (char*)defmes.c_str() + d;
					*temp = toupper(*(defmes.c_str() + d));
				}
				fprintf(fdefineh, "#define %s %d\n", defmes, ClassArray[j]._funcA[f]._num);

				//SC proxy
				if (ClassArray[j]._funcA[f]._type == 0)
				{
					fprintf(fproxyh, "void Proxy%s(__int64* sessionA, int count", ClassArray[j]._funcA[f]._functionName);
					fprintf(fproxyc, "void %sProxy::Proxy%s(__int64* sessionA, int count", ClassArray[j]._className, ClassArray[j]._funcA[f]._functionName);
					for (int c = 0; c < ClassArray[j]._funcA[f]._paramA.size(); c++)
					{
						fprintf(fproxyh, ", %s %s", ClassArray[j]._funcA[f]._paramA[c]._dataType, ClassArray[j]._funcA[f]._paramA[c]._variable);
						fprintf(fproxyc, ", %s %s", ClassArray[j]._funcA[f]._paramA[c]._dataType, ClassArray[j]._funcA[f]._paramA[c]._variable);
					}
					fprintf(fproxyh, ");\n");
					fprintf(fproxyc, ")\n{\n");
					fprintf(fproxyc, "CPacket packet;\npacket.Clear();\nunsigned char type=%s;\npacket<<type", defmes);
					for (int c = 0; c < ClassArray[j]._funcA[f]._paramA.size(); c++)
					{
						fprintf(fproxyc, "<<%s", ClassArray[j]._funcA[f]._paramA[c]._variable);
					}
					fprintf(fproxyc, ";\n");
					fprintf(fproxyc, "for (int i = 0; i < count; i++)\n");
					fprintf(fproxyc, "{\n_server->SendPacket(sessionA[i], packet);\n}\n");
					fprintf(fproxyc, "}\n");

				}
				//CS stub
				if (ClassArray[j]._funcA[f]._type == 1)
				{
					fprintf(fstubh, "virtual void Proc%s(__int64 sessionID", ClassArray[j]._funcA[f]._functionName);
					fprintf(fstubc, "case %s:\n{\n", defmes);
					for (int c = 0; c < ClassArray[j]._funcA[f]._paramA.size(); c++)
					{
						fprintf(fstubh, ",%s %s", ClassArray[j]._funcA[f]._paramA[c]._dataType, ClassArray[j]._funcA[f]._paramA[c]._variable);
						fprintf(fstubc, "%s %s_=0;\n", ClassArray[j]._funcA[f]._paramA[c]._dataType, ClassArray[j]._funcA[f]._paramA[c]._variable);
					}
					fprintf(fstubh, ")=0;\n");
					fprintf(fstubc, "packet");
					for (int c = 0; c < ClassArray[j]._funcA[f]._paramA.size(); c++)
					{
						fprintf(fstubc, ">>%s_", ClassArray[j]._funcA[f]._paramA[c]._variable);
					}
					fprintf(fstubc, ";\n");
					fprintf(fstubc, "Proc%s(sessionID", ClassArray[j]._funcA[f]._functionName);
					for (int c = 0; c < ClassArray[j]._funcA[f]._paramA.size(); c++)
					{
						fprintf(fstubc, ",%s_", ClassArray[j]._funcA[f]._paramA[c]._variable);
					}
					fprintf(fstubc, ");\nbreak;\n}\n");


				}

			}
			fprintf(fproxyh, "};\n");
			fprintf(fstubh, "virtual void Proc%sDefault(__int64 sessionID, CPacket packet)=0;\n", ClassArray[j]._className);
			fprintf(fstubh, "};\n\n");
			fprintf(fstubc, "default:\n{\n");
			fprintf(fstubc, "Proc%sDefault(sessionID,packet);\nbreak;\n}\n", ClassArray[j]._className);
			fprintf(fstubc, "}\n}\n\n\n");
		}



		fclose(fdefineh);
		fclose(fproxyh);
		fclose(fproxyc);
		fclose(fstubh);
		fclose(fstubc);

	}
}
*/