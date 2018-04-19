
#if defined(_MSC_VER)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif // _MSC_VER

#include "../../include/test_utils.h"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"


using namespace azp;

using BoostJV = boost::property_tree::ptree;


BoostJV parseJson(const std::string& doc) {
	std::istringstream inStream(doc);
	BoostJV root;
	try { boost::property_tree::json_parser::read_json(inStream, root); }
		catch (std::exception& e) {
		std::cout << "parse failure  " << e.what() << '\n';
	}
	return root;
}


std::string writeJson(const BoostJV& root) {
	std::ostringstream outStream;
	boost::property_tree::json_parser::write_json(outStream, root);
	return outStream.str();
}


void check() {
	BoostJV v, d;
	d.put_value(1.E10 / 3);
	v.put_child("test", d);
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
	
	benchmark("Boost load",  [&str](){parseJson(str);});
	auto root = parseJson(str);
	benchmark("Boost write", [&root](){writeJson(root);});
	
	printf("\n");
}


