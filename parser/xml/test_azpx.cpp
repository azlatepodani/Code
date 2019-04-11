
#if defined(_MSC_VER)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif // _MSC_VER

#include "../../include/test_utils.h"
#include "azp_xml_api.h"


using namespace azp;


XmlDocument parseJson(const std::string& buf) {
	XmlDocument doc;
	try { doc = azp::xml_reader(buf);
		//optimize_for_search(root.first);
	}
	catch (std::exception& e) {
		std::cout << "parse failure  " << e.what() << '\n';
	}
	return doc;
}


std::string writeJson(const XmlDocument& doc) {
	std::string stm;
	xml_writer(stm, doc);
	return stm;
}



#if defined(_MSC_VER)
int wmain(int, PWSTR argv[])
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	if (GetThreadPriority(GetCurrentThread()) != THREAD_PRIORITY_HIGHEST) printf("Priority set failed\n");
	 
	if (!SetThreadAffinityMask(GetCurrentThread(), 2)) printf("Affinity set failed\n");

#else
int main(int, char* argv[]) {
#endif
	 
	auto str = loadFile(argv[1]);
	
	benchmark("XML API load",  [&str](){parseJson(str);});
	auto root = parseJson(str); 
	// if (str != writeJson(root.first)) printf("problem\n");
	// else printf("ok\n");
	benchmark("XML API write", [&root,&str](){/*if (str != */writeJson(root)/*) __debugbreak()*/;});
	
	printf("\n");
	return 0;
}


