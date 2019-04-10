#include <algorithm>
#include <float.h>
#include <exception>
#include <stdio.h>
#include <string.h>
#include "azp_xml.h"
#include "azp_xml_api.h"

#include <iostream>
#include <fstream>
#if defined(_MSC_VER)
#include <windows.h>
#endif



namespace azp {
    
alloc_t __alloc;

XmlGenNode::XmlGenNode(XmlTag&& tag) : type(Tag) {
    new (u.tag) XmlTag(std::move(tag));
}

XmlGenNode::XmlGenNode(XmlPInstr&& pi) : type(Pinstr) {
    new (&u.pi) XmlPInstr(std::move(pi));
}

XmlGenNode::XmlGenNode(string_view_t&& str, bool cdata) : type(cdata?Cdata_text:Text) {
    new (&u.str) string_view_t(std::move(str));
}

XmlGenNode::XmlGenNode(XmlGenNode&& node) : type(node.type) {
    if (type == Tag) {
        new (u.tag) XmlTag(std::move(node.tag()));
    }
    else {
        memcpy(&u, &node.u, sizeof(u));
    }
}

XmlGenNode::XmlGenNode(const XmlGenNode& node) : type(node.type) {
    if (type == Tag) {
        new (u.tag) XmlTag(node.tag());
    }
    else {
        memcpy(&u, &node.u, sizeof(u));
    }
}

XmlGenNode& XmlGenNode::operator=(XmlGenNode&& node) {
    if (type == Tag) {
        ((XmlTag*)u.tag)->~XmlTag();
    }
    
    type = node.type;
    
    if (type == Tag) {
        new (u.tag) XmlTag(std::move(node.tag()));
    }
    else {
        memcpy(&u, &node.u, sizeof(u));
    }
    
    return *this;
}

XmlGenNode& XmlGenNode::operator=(const XmlGenNode& node) {
    XmlGenNode tmp(node);
    *this = std::move(tmp);
    return *this;
}



template <typename Allocator>
struct parser_callback_ctx_t {
	bool firstCall;
    vector<XmlTag, Allocator> stack;
    XmlPInstr attr_pi;
    XmlDocument * result;
	Allocator& a;
	
	// cppcheck-suppress noExplicitConstructor
	parser_callback_ctx_t(Allocator& a) 
		: firstCall(1), stack(a), result(0), a(a)
	{ }
};


template <typename Allocator>
static bool parser_callback(void* ctx, enum ParserTypes type, const string_view_t& value) noexcept;



XmlDocument xml_reader(std::string stm) {
    XmlDocument doc;
    if (!stm.empty()) {
		parser_t p;
		alloc_t a;

		auto ctx = parser_callback_ctx_t<alloc_t>(a);

		p.set_max_recursion(20);
		p.set_callback(&parser_callback<alloc_t>, &ctx);

		ctx.stack.reserve(p.get_max_recursion());

		ctx.result = &doc;
		doc._backing = std::move(stm);
		
		if (!parseXml(p, &doc._backing[0], &doc._backing[0]+doc._backing.size())) {
			throw std::exception(/*"cannot parse"*/);
		}
	}
	
	return doc;
}


template <typename Allocator>
static bool parser_callback(void* ctx, enum ParserTypes type, const string_view_t& value) noexcept {
	auto& cbCtx = *(parser_callback_ctx_t<Allocator> *)ctx;
	
	if (!cbCtx.firstCall) {		
		switch (type)
		{
		case Tag_open:
			cbCtx.stack.push_back(XmlTag(string_view_t(value)));
			break;
			
		case Tag_close: {
			XmlTag obj(std::move(cbCtx.stack.back()));
			cbCtx.stack.pop_back();
			
			if (cbCtx.stack.begin() != cbCtx.stack.end()) {
                cbCtx.stack.back().children.push_back(std::move(obj));
			}
			else {
				// we're done
				cbCtx.result->root = std::move(obj);
			}
			
			break;
		}
		case Text:
        case Cdata_text:
			cbCtx.stack.back().children.push_back({string_view_t(value), (type==Text)?false:true});
            break;

		case Attribute_name:
		case Pinstr_name:
            cbCtx.attr_pi.first = value;
			break;

        case Attribute_value:
            cbCtx.attr_pi.second = value;
            cbCtx.stack.back().attributes.push_back(std::move(cbCtx.attr_pi));
			break;
            
        case Pinstr_text:
            cbCtx.attr_pi.second = value;
            cbCtx.stack.back().children.push_back(std::move(cbCtx.attr_pi));
			break;

		default:
			return false;
		}
	}
	else {
		if (type == Tag_open) {
            cbCtx.firstCall = false;
			cbCtx.stack.push_back(XmlTag(string_view_t(value)));
			return true;
		}
				
		switch (type)
		{
		case Attribute_name:
        case Pinstr_name:
			cbCtx.attr_pi.first = value;
			break;

		case Attribute_value:
			cbCtx.stack.back().attributes.push_back(std::move(cbCtx.attr_pi));
			break;

		case Pinstr_text:
			cbCtx.result->misc.push_back(std::move(cbCtx.attr_pi));
			break;

		default: return false;
		}
	}

    return true;
}


} // namespace asu


    
 #if defined(_MSC_VER)
	inline std::string loadFile(const wchar_t * path) {
		std::string str;
		auto h = CreateFileW(path, GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);
		if (h == INVALID_HANDLE_VALUE) {
			printf("cannot open file  %d\n", GetLastError());
			return std::string();
		}
		auto size = GetFileSize(h, 0);
		str.resize(size);
		if (!ReadFile(h, &str[0], (ULONG)str.size(), 0,0)) {
            printf("cannot read file  %d\n", GetLastError());
		}
		CloseHandle(h);
		return str;
	}

#else
	inline std::string loadFile(const char * path) {
		std::string str;
		std::ifstream stm(path, std::ios::binary);
		
		if (!stm.good()) {
			printf("cannot open file\n");
			return std::string();
		}
		
		stm.seekg(0, std::ios_base::end);
		auto size = stm.tellg();
		stm.seekg(0, std::ios_base::beg);
		
		str.resize(size);
		stm.read(&str[0], size);
		return str;
	}
#endif // _MSC_VER   


#if defined(_MSC_VER)
int wmain(int argc, PWSTR argv[])
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	if (GetThreadPriority(GetCurrentThread()) != THREAD_PRIORITY_HIGHEST) printf("Priority set failed\n");
	 
	if (!SetThreadAffinityMask(GetCurrentThread(), 2)) printf("Affinity set failed\n");

#else
int main(int argc, char* argv[]) {
#endif

    //char vec[][100] = {//"<tag></tag>", "<tag> a </tag>",
                    //"<?XmllL version='1.4'?><tag><tag></tag>a <![CDATA[<tag>&apos;</tag>]]></tag>",
                    // "<tag a='1'/>",
                    // "<tag a='1' b=\"2\"/>",
                    //};
                    //"C:\\Users\\Andrei-notebook\\Downloads\\rec00001output"
    if (argc != 2) return -1;
                        
    auto buf = loadFile(argv[1]);
    //for (auto s : vec) {
        try {
            azp::xml_reader(std::move(buf));
        }
catch(...) {        printf("problem\n");}
    //}
}
