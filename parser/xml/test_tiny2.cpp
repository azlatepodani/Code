
#if defined(_MSC_VER)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif // _MSC_VER

#include "../../include/test_utils.h"
#include "tinyxml2/tinyxml2.h"


using namespace tinyxml2;


void parseJson(const std::string& buf, XMLDocument& doc) {
	try {
		auto err = doc.Parse(buf.c_str(), buf.size());
        if (err) printf("parse failure %d\n", err);
		//optimize_for_search(root.first);
	}
	catch (std::exception& e) {
		std::cout << "parse failure  " << e.what() << '\n';
	}
}


std::string writeJson(const XMLDocument& doc) {
    XMLPrinter printer(0, true);
    doc.Print( &printer );
	return printer.CStr();
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
	 
	auto str = azp::loadFile(argv[1]);
	
	azp::benchmark("TinyXML2 load",  [&str](){XMLDocument doc; parseJson(str, doc);});
    XMLDocument root; 
	parseJson(str, root); 
	// if (str != writeJson(root.first)) printf("problem\n");
	// else printf("ok\n");
	azp::benchmark("TinyXML2 write", [&root,&str](){/*if (str != */writeJson(root)/*) __debugbreak()*/;});
	
	printf("\n");
	return 0;
}


