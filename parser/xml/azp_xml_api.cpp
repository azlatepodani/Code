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


static size_t xml_writer_size(const XmlDocument& doc);
static void xml_writer_imp(std::string& stm, const XmlTag& tag);


void xml_writer(std::string& stm, const XmlDocument& doc) {
	auto size = xml_writer_size(doc);
	size += size/3;
	stm.reserve(size);
	
	if (doc.version.len || doc.encoding.len || doc.standalone.len) {
		stm.append("<?xml version=\"");
		stm.append(doc.version.len ? doc.version.str : "1.1");
		stm.push_back('"');
		
		if (doc.encoding.len) {
			stm.append("encoding=\"");
			stm.append(doc.encoding.str);
			stm.push_back('"');
		}
		
		if (doc.standalone.len) {
			stm.append("standalone=\"");
			stm.append(doc.standalone.str);
			stm.push_back('"');
		}
		
		stm.append("?>");
	}
	
	for (auto& node : doc.misc) {
		stm.append("<?");
		stm.append(node.pi().first.str, node.pi().first.len);
		stm.push_back(' ');
		stm.append(node.pi().second.str, node.pi().second.len);
		stm.append("?>");
	}
	
	xml_writer_imp(stm, doc.root);
}


static void xmlEscape(const char* first, size_t len, std::string& dst)
{
    auto last = first + len;
    auto start = first;
	const char* esc;
	int escLen;
	
	while (first != last) {
        auto ch = *first;
        if ((uint32_t(uint8_t(ch))|0x20) < 'a') {
            switch (ch) {
                case '"': goto Escapequ;
                case '&': goto Escapeam;
                case '\'': goto Escapeap;
                case '<': goto Escapelt;
                case '>': goto Escapegt;
                    
                default:;
            }
        }
        
        ++first;
		continue;
		
Escapequ: esc = "&quot;"; escLen = 6; goto Append;
Escapeam: esc = "&amp;"; escLen = 5; goto Append;
Escapeap: esc = "&apos;"; escLen = 6; goto Append;
Escapelt: esc = "&lt;"; escLen = 4; goto Append;
Escapegt: esc = "&gt;"; escLen = 4; 

Append:
		dst.append(start, first);
		dst.append(esc, esc+escLen);
		start = ++first;
    }
}


static size_t xml_writer_size(const XmlTag& tag) {
	size_t size = 2*tag.name.len + 5;
	
	for (auto& att : tag.attributes) {
		size += 3 + att.first.len + att.second.len;
	}
	
	for (auto& node : tag.children) {
		if (node.type == XmlGenNode::Tag) {
			size += xml_writer_size(node.tag());
			continue;
		}
		
		switch (node.type)
		{
		case XmlGenNode::Text:
			size += node.str().len;
			break;
			
		case XmlGenNode::Cdata_text:
			size += 12 + node.str().len;	//<![CDATA[]]>
			break;
			
		case XmlGenNode::Pinstr:
			size += 5 + node.pi().first.len + node.pi().second.len;
			break;
			
		default:;
		}
	}
	
	return size;
}

static size_t xml_writer_size(const XmlDocument& doc) {
	size_t size = 50;	// prelude
	
	for (auto& node : doc.misc) {
		size += 5 + node.pi().first.len + node.pi().second.len;
	}
	
	size += xml_writer_size(doc.root);
	
	return size;
}


static void xml_writer_imp(std::string& stm, const XmlTag& tag) {
	stm.push_back('<');
	stm.append(tag.name.str, tag.name.len);
	
	for (auto& att : tag.attributes) {
		stm.push_back(' ');
		stm.append(att.first.str, att.first.len);
		stm.push_back('=');
		stm.push_back('"');
		xmlEscape(att.second.str, att.second.len, stm);
		stm.push_back('"');
	}
	
	if (tag.children.empty()) {
		stm.push_back('/');
		stm.push_back('>');
	}
	else {
		stm.push_back('>');
		
		for (auto& node : tag.children) {
			if (node.type == XmlGenNode::Tag) {
				xml_writer_imp(stm, node.tag());
				continue;
			}
			
			switch (node.type)
			{
			case XmlGenNode::Text:
				xmlEscape(node.str().str, node.str().len, stm);
				break;
				
			case XmlGenNode::Cdata_text:
				stm.append("<![CDATA[", 9);
				stm.append(node.str().str, node.str().len);
				stm.append("]]>");
				break;
				
			case XmlGenNode::Pinstr:
				stm.append("<?", 2);
				stm.append(node.pi().first.str, node.pi().first.len);
				stm.push_back(' ');
				stm.append(node.pi().second.str, node.pi().second.len);
				stm.append("?>", 2);
				break;
				
			default:;
			}
		}

		stm.push_back('<');stm.push_back('/');
		stm.append(tag.name.str, tag.name.len);
		stm.push_back('>');
	}
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
