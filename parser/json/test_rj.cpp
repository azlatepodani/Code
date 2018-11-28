
#if defined(_MSC_VER)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif // _MSC_VER

#include "../../include/test_utils.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


using namespace rapidjson;
using namespace azp;


// https://github.com/Tencent/rapidjson


Document parseJson(const std::string& doc) {
	Document d;
    //d.Parse(doc.c_str());
	d.Parse<kParseFullPrecisionFlag>(doc.c_str());

	return d;
}


std::string writeJson(const Document& d) {
	StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);

	return buffer.GetString();
}


static std::string double_to_string(double dbl) {
    char buf[340] = { 0, };
    auto len = sprintf(buf, "%.*g", 17, dbl);
    return std::string(buf, buf+len);
}


void check() {
	StringBuffer s;
    Writer<StringBuffer> writer(s);

	writer.Double(1. / 3);
	
	auto str = double_to_string(1. / 3);
	
	 if (str == s.GetString()) {
		printf("check ok\n");
	}
	else {
		printf("check failed  %s  %s\n", str.c_str(), s.GetString());
	}
}



#if defined(_MSC_VER)
int wmain(int, PWSTR argv[])
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	if (GetThreadPriority(GetCurrentThread()) != THREAD_PRIORITY_HIGHEST) printf("Priority set failed\n");
	 
	if (!SetThreadAffinityMask(GetCurrentThread(), 1)) printf("Affinity set failed\n");

#else
int main(int, char* argv[]) {
#endif

	check();
	 
	auto str = loadFile(argv[1]);
	
	benchmark("RapidJson load",  [&str](){parseJson(str);});
	auto root = parseJson(str);
	benchmark("RapidJson write", [&root,&str](){writeJson(root);});
	
	printf("\n");
	return 0;
}


