
#if defined(_MSC_VER)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif // _MSC_VER

#include "../../include/test_utils.h"
#include "json/value.h"
#include "json/writer.h"
#include "json/reader.h"


#pragma comment(lib, "libjson.lib")


// https://github.com/open-source-parsers/jsoncpp


Json::Value parseJson(const std::string& doc) {
	Json::Value root;
	if (!Json::Reader().parse(doc, root)) {
		std::cout << "parse failure\n";
	}
	return root;
}


std::string writeJson(const Json::Value& root) {
	std::string jsonString = Json::FastWriter().write(root);
	return jsonString;
}


void check() {
	Json::Value v(1.E10 / 3);
	std::string j = writeJson(v);
	auto p = parseJson(j);
	
	if (p == v) {
		printf("check ok\n");
	}
	else {
		printf("check failed\n");
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
	 
	std::string str = loadFile(argv[1]);
	
	benchmark("JsonCPP load",  [&str](){parseJson(str);});
	auto root = parseJson(str);
	benchmark("JsonCPP write", [&root](){writeJson(root);});
	
	printf("\n");
}


