#include <stdlib.h>
#include <locale.h>
#include <cmath>
#include <climits>
#include <string.h>
#include "azp_xml.h"
#include <memory>


namespace azp {

	
static bool parseXml(parser_base_t& p, char * first, char * last);
static bool parse_error(parser_base_t& p, ParserErrors err, const char * curPtr);


bool parseXml(parser_t& p, char * first, char * last) {
	p._first = first;
	
	bool result = parseXml(static_cast<parser_base_t&>(p), first, last);
	
	if (result) {
		p.parsed_offset = p.parsed - first;
	}
	else {
		p.parsed = nullptr;
	}
	
	return result;
}


bool parseXml(parser_t& p, const char * first, const char * last)
{
	auto size = last - first;
	std::unique_ptr<char[]> buf(new (std::nothrow) char[size]);
	
	if (!buf) return parse_error(p, Runtime_error, nullptr);
	
	memcpy(buf.get(), first, size);
	
	auto result = parseXml(p, buf.get(), buf.get()+size);
	
	p.parsed = nullptr;	// 'parsed' would be invalid after return
	
	return result;
}

	
parser_t::parser_t() {
	parsed = nullptr;
	context = nullptr;
	callback = [](void*, ParserTypes, const value_t&) { return true; };
	recursion = 0;
	max_recursion = 16;
	error = No_error;
	err_position = 0;
	_first = nullptr;
	parsed_offset = 0;
}


static char * skip_wspace(char * first, char * last);
static bool parseXmlTag(parser_base_t& p, char * first, char * last);
static bool parseJsonArray(parser_base_t& p, char * first, char * last);
static bool parseJsonScalarV(parser_base_t& p, char * first, char * last);


struct dec_on_exit {
	// cppcheck-suppress noExplicitConstructor
	dec_on_exit(uint32_t& val) : val(val) {}
	~dec_on_exit() { --val; }
	uint32_t& val;
};


// parses a JSON value
static bool parseXml(parser_base_t& p, char * first, char * last) {
	first = skip_wspace(first, last);
	if (first == last) {
		return parse_error(p, No_value, first);
	}
	
	if (*first == '<') return parseXmlTag(p, first+1, last);
	//if (*first == '[') return parseJsonArray(p, first+1, last);
	return parse_error(p, Unexpected_char, first);
}


static bool parse_error(parser_base_t& p, ParserErrors err, const char * curPtr)
{
	p.error = err;
	p.err_position = curPtr - p._first;
	return false;
}


static char * skip_wspace(char * first, char * last) {
	auto n = last - first;
    if (n && *first > ' ') return first;	// likely
	for (; n; --n) {
		if ((*first == ' ') | (*first == '\n') | (*first == '\r') | (*first == '\t')) ++first;
		else return first;
	}
	return first;
}


inline bool isDigit(char c) {
	return (uint32_t(c) - '0') < 10;	// if 'c' isn't in ('0' .. '9'), the result is >= 10
}


inline bool isHexAlpha(char c) {
	return ((uint32_t(c)|0x20) - 'a') <= ('f' - 'a');	// convert capital letters to small letters and test
}


static bool parseName(parser_base_t& p, ParserTypes type, char * first, char * last);
static bool parseTagAttribute(parser_base_t& p, char * first, char * last);


// assumes that '<' was already parsed
static bool parseXmlTag(parser_base_t& p, char * first, char * last) {
	if (++p.recursion >= p.max_recursion) return parse_error(p, Max_recursion, first);
	
	dec_on_exit de(p.recursion);

	if (!parseName(p, Tag_open, first, last)) return false;
	first = p.parsed;
	
	while (true) {
		first = skip_wspace(first, last);
		if (first == last) return parse_error(p, Expected_closing_brace, first);
		
		if (*first == '>') { p.parsed = first; break; }

		if (*first == '/') { 
			tagClosed = true;
			
			++first;
			if (first != last && *first == '>') { p.parsed = first; break; }
			return parse_error(p, Expected_closing_brace, first);
		}
		
		if (!parseTagAttribute(p, first, last)) return false;
		first = p.parsed;
	}
	
	if (!tagClosed) {
		if (!parseTagBodyAndClosingTag(p, first, last)) return false;
	}
	
	value_t val;
	return wrap_user_callback(Tag_close, val, p.parsed);
}


static bool parseName(parser_base_t& p, ParserTypes type, char * first, char * last) {
	auto savedFirst = first;
	while (first != last) {
		switch (*first) {
			case '/':
			case '>':
			case ' ':
			case '\t':
			case '\n':
			case '\r':
			case '=':
				goto Name_done;
				
			default: ++first;
		}
	}
	
Name_done:
	if (savedFirst == first) return parse_error(p, Expected_name, first);
	
	p.parsed = first;
	
	string_view_t val;
	val.str = savedFirst;
	val.len = first - savedFirst;
	return wrap_user_callback(type, val, p.parsed);
}


static bool parseAttrValue(parser_base_t& p, char * first, char * last) {
	auto ch = *first++;
	if ((ch != '"') & (ch != '\'')) parse_error(p, Unexpected_char, first-1);

	while (true) {
		auto 
	}
}


static bool parseTagAttribute(parser_base_t& p, char * first, char * last) {
	if (!parseName(p, Attribute_name, first, last)) return false;
	first = p.parsed;

	first = skip_wspace(first, last);
	if (first == last) return parse_error(p, Expected_attr_value, first);
	
	if (*first != '=') return parse_error(p, Expected_attr_value, first);
	
	first = skip_wspace(first+1, last);
	if (first == last) return parse_error(p, Expected_attr_value, first);
	
	return parseAttrValue(p, first, last);
}


} // namespace azp
