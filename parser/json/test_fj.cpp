
#if defined(_MSC_VER)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif // _MSC_VER

#undef min
#undef max

#include "../../include/test_utils.h"
#include "json.h"


using namespace azp;


// Chromium


folly::dynamic parseJson(const std::string& doc) {
	folly::json::serialization_opts opts;
	opts.validate_utf8 = true;
	return folly::parseJson(doc, opts);
}


std::string writeJson(const folly::dynamic& d) {
	return folly::toJson(d);
}


static std::string double_to_string(double dbl) {
    char buf[340] = { 0, };
    auto len = sprintf(buf, "%.*g", 17, dbl);
    return std::string(buf, buf+len);
}


void check() {
	// StringBuffer s;
    // Writer<StringBuffer> writer(s);

	// writer.Double(1. / 3);
	
	// auto str = double_to_string(1. / 3);
	
	 // if (str == s.GetString()) {
		// printf("check ok\n");
	// }
	// else {
		// printf("check failed  %s  %s\n", str.c_str(), s.GetString());
	// }
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
	
	benchmark("Folly-JSON load",  [&str](){parseJson(str);});
	auto root = parseJson(str);
	benchmark("Folly-JSON write", [&root,&str](){writeJson(root);});
	
	printf("\n");
	return 0;
}


