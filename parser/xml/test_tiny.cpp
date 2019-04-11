
#if defined(_MSC_VER)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif // _MSC_VER

#include "../../include/test_utils.h"
#include "tinyxml/tinyxml.h"


using namespace azp;


TiXmlDocument parseJson(const std::string& buf) {
	TiXmlDocument doc;
	try {
		bool loadOkay = doc.Parse(buf.c_str(), 0, TIXML_ENCODING_UTF8);
        if (doc.Error()) printf("parse failure %d\n", doc.ErrorId());
		//optimize_for_search(root.first);
	}
	catch (std::exception& e) {
		std::cout << "parse failure  " << e.what() << '\n';
	}
	return doc;
}


std::string writeJson(const TiXmlDocument& doc) {
    TiXmlPrinter printer;
    printer.SetIndent(0);
    doc.Accept( &printer );
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
	 
	auto str = loadFile(argv[1]);
	
	benchmark("TinyXML load",  [&str](){parseJson(str);});
	auto root = parseJson(str); 
	// if (str != writeJson(root.first)) printf("problem\n");
	// else printf("ok\n");
	benchmark("TinyXML write", [&root,&str](){/*if (str != */writeJson(root)/*) __debugbreak()*/;});
	
	printf("\n");
	return 0;
}


