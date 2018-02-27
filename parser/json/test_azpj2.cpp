
#if defined(_MSC_VER)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif // _MSC_VER

#include "../../include/test_utils.h"
#include "azp_json_api.h"


using namespace azp;


std::pair<JsonValue, std::string> parseJson(const std::string& doc) {
	std::pair<JsonValue, std::string> root;
	try { root = json_reader(doc); }
	catch (std::exception& e) {
		std::cout << "parse failure  " << e.what() << '\n';
	}
	return root;
}


std::string writeJson(const JsonValue& root) {
	std::string stm;
	json_writer(stm, root);
	return stm;
}


void check() {
	JsonValue v(1.E10 / 3);
	std::string j;
	json_writer(j, v);
	auto p = json_reader(j);
	
	if (p.first.type == JsonValue::FLOAT_NUM && p.first.u.float_num == v.u.float_num) {
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
	 
	auto str = loadFile(argv[1]);
	
	benchmark("Json API load",  [&str](){parseJson(str);});
	auto root = parseJson(str);
	benchmark("Json API write", [&root,&str](){/*if (str != */writeJson(root.first)/*) __debugbreak()*/;});
	
	printf("\n");
	return 0;
}


