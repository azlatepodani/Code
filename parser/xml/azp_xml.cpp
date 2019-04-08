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
	callback = [](void*, ParserTypes, const string_view_t&) { return true; };
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
static bool parseTagBodyAndClosingTag(parser_base_t& p, char * first, char * last);


#define wrap_user_callback(RT, VAL, ERR_HINT) 					\
					(p.callback(p.context, (RT), (VAL)) ? true	\
					: parse_error(p, User_requested, (ERR_HINT)))
					

// assumes that '<' was already parsed
static bool parseXmlTag(parser_base_t& p, char * first, char * last) {
	if (++p.recursion >= p.max_recursion) return parse_error(p, Max_recursion, first);
	
	dec_on_exit de(p.recursion);

	if (!parseName(p, Tag_open, first, last)) return false;
	first = p.parsed;
	
	bool tagClosed = false;
	
	while (true) {
		first = skip_wspace(first, last);
		if (first == last) return parse_error(p, Expected_closing_brace, first);
		
		if (*first == '>') { ++first; break; }

		if (*first == '/') { 
			tagClosed = true;
			
			++first;
			if (first != last && *first == '>') { ++first; break; }
			return parse_error(p, Expected_closing_brace, first);
		}
		
		if (!parseTagAttribute(p, first, last)) return false;
		first = p.parsed;
	}
	
	if (!tagClosed) {
		if (!parseTagBodyAndClosingTag(p, first, last)) return false;
	}
	else {
		p.parsed = first;
	}
	
	string_view_t val{0,0};
	return wrap_user_callback(Tag_close, val, p.parsed);
}


static char * findNameEnd(char * first, char * last) {
	while (first != last) {
		switch (*first++) {
			case '/':
			case '>':
			case ' ':
			case '\t':
			case '\n':
			case '\r':
			case '=':
			case ';':
			case '"':
			case '\'':
			case '!':
				return first-1;
				
			default:;
		}
	}

	return first;
}


static bool parseName(parser_base_t& p, ParserTypes type, char * first, char * last) {
	auto savedFirst = first;
	first = findNameEnd(first, last);
	if (savedFirst == first) return parse_error(p, Expected_name, first);
	
	p.parsed = first;
	
	string_view_t val{savedFirst, size_t(first-savedFirst)};
	return wrap_user_callback(type, val, p.parsed);
}


static bool parseReference(parser_base_t& p, char * first, char * last) {
	return true;
}


static bool parseAttrValue(parser_base_t& p, char * first, char * last) {
	auto ch = *first++;
	if ((ch != '"') & (ch != '\'')) return parse_error(p, Unexpected_char, first-1);
	
	auto savedFirst = first;

	while (true) {
		auto c = *first++;
		if (c == ch) break;
		
		if (c == '&') {
			if (!parseReference(p, first, last)) return false;
			first = p.parsed;
		}
		else if (c == '<') return parse_error(p, Unexpected_char, first-1);
		
		if (first == last) return parse_error(p, Expected_quote, first);
	}
	
	p.parsed = first;
	
	string_view_t val{savedFirst, size_t(first - savedFirst - 1)};	// first was incremented after locating the closing quote
	return wrap_user_callback(Attribute_value, val, p.parsed);
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


static bool expandCharReference(parser_base_t& p, char * first, char * last, char * cur, char *& textEnd) {
	auto n = last-first;
	if (!n) return false;
	
	char * savedFirst;
	uint32_t num = 0;
	
	// the last valid XML char is 0xEFFFF == 983039
	if (*first == 'x') {
		savedFirst = ++first;
		if (n > 5) n = 5;
		
		while (n--) {
			auto ch = *first++;
			if (isDigit(ch)) {
				num <<= 4;
				num |= ch - '0';
			}
			else if (isHexAlpha(ch)) {
				num <<= 4;
				num |= (ch|0x20) - 'a' + 10;
			}
			else {
				--first;	// ch wasn't parsed yet
				break;
			}
		}
			}
	else {
		savedFirst = first;
		if (n > 6) n = 6;
		
		while (n--) {
			auto ch = *first++;
			if (isDigit(ch)) {
				num = (num << 3) + (num << 1);
				num += ch - '0';
			}
			else {
				--first;	// ch wasn't parsed yet
				break;
			}
		}
	}
	
	if (first == last || *first != ';') return parse_error(p, Expected_semicolon, first);
	++first;
	// minimum validity checks
	if (((num >= 0xD800) & (num < 0xE000)) | (num > 0xEFFFF) | (num == 0))
		return parse_error(p, Invalid_escape, savedFirst);

	if (num < 128) {
		*cur++ = (char)num;
	}
	else if (num < 0x800) {
		*cur++ = char((num >> 6) | 0xC0);
		*cur++ = char((num & 0x3F) | 0x80);
	}
	else if (num < 0x10000) {
		*cur++ = char((num >> 12) | 0xE0);
		*cur++ = char(((num >> 6) & 0x3F) | 0x80);
		*cur++ = char((num & 0x3F) | 0x80);
	}
	else {
		*cur++ = char((num >> 18) | 0xF0);
		*cur++ = char(((num >> 12) & 0x3F) | 0x80);
		*cur++ = char(((num >> 6) & 0x3F) | 0x80);
		*cur++ = char((num & 0x3F) | 0x80);
	}
	
	textEnd = cur;
	p.parsed = first;
	return true;
}


static bool expandReference(parser_base_t& p, char * first, char * last, char * cur, char *& textEnd) {
	auto ch = *first;
	auto n = last - first++;
	
	if (ch != '#') {
		switch (ch) {
			case 'a': 
				if (n >= 3 && memcmp(first, "mp;", 3) == 0) {
					*cur++ = '&';
					first += 3;
					goto Out;
				}
				
				if (n >= 4 && memcmp(first, "pos;", 4) == 0) {
					*cur++ = '\'';
					first += 4;
					goto Out;
				}
				break;
				
			case 'g':
				if (n >= 2 && memcmp(first, "t;", 2) == 0) {
					*cur++ = '>';
					first += 2;
					goto Out;
				}
				
				break;
				
			case 'l':
				if (n >= 2 && memcmp(first, "t;", 2) == 0) {
					*cur++ = '<';
					first += 2;
					goto Out;
				}
				
				break;
				
			case 'q':
				if (n >= 4 && memcmp(first, "uot;", 4) == 0) {
					*cur++ = '"';
					first += 4;
					goto Out;
				}
				
				break;
				
			default:;
		}
		
		// just skip the text for now 
		first = findNameEnd(first-1, last);
		if (first == last || *first != ';') return false;
		++first;
	}
	else {
		return expandCharReference(p, first, last, cur, textEnd);
	}
	
Out:
	textEnd = cur;
	p.parsed =  first;
	return true;
}


static bool parseCharData(parser_base_t& p, char * first, char * last) {
	auto n = last - first;
	
	auto savedFirst = first;
	auto ch = *first;
	
	while (n && ((ch != '<') & (ch != '&'))) {
		ch = *(++first);
		--n;
	}
	
	auto textEnd = first;
	
	while (ch == '&') {
		auto savedEnd = textEnd;
		if (!expandReference(p, first+1, last, savedEnd, textEnd)) {
			return parse_error(p, Invalid_reference, first);
		}
		
		first = p.parsed;
		
		n = last - first;
		if (!n) break;
		
		ch = *first;
		while (n && ((ch != '<') & (ch != '&'))) {
			*textEnd++ = ch;
			ch = *(++first);
			--n;
		}
	}
	
	p.parsed = first;
	string_view_t val{savedFirst, size_t(textEnd-savedFirst)};
	if (val.len && !wrap_user_callback(Text, val, first)) return false;
	
	return true;
}


static bool parseClosingTag(parser_base_t& p, char * first, char * last) {
	auto nameEnd = findNameEnd(first, last);
	if (first == nameEnd) return parse_error(p, Unexpected_char, first);
	
	// ? need to check the name
	
	first = skip_wspace(nameEnd, last);
	if (first == last || *first != '>') return parse_error(p, Expected_closing_brace, first);
	
	p.parsed = first+1;
	
	return true;
}


static bool parseCDataSect(parser_base_t& p, char * first, char * last) {
	return false;
}


static bool parseComment(parser_base_t& p, char * first, char * last) {
	return false;
}


static bool parseProcessingInstruction(parser_base_t& p, char * first, char * last) {
	return false;
}


static bool parseTagBodyAndClosingTag(parser_base_t& p, char * first, char * last)
{
	while (true) {
		if (!parseCharData(p, first, last)) return false;
		
		first = p.parsed;
		
		if (last - first < 4) return parse_error(p, Unbalanced_collection, first);	// a valid closing tag is at least '</a>'
		
		++first;	// skip '<'
		
		auto ch = *first;
		
		if (ch == '/') return parseClosingTag(p, first+1, last);
	
		if ((ch != '!') & (ch != '?')) {
			if (!parseXmlTag(p, first, last)) return false;
		}
		else if (ch == '!') {
			++first;
			
			if (*first == '[') {
				if (!parseCDataSect(p, first+1, last)) return false;
			}
			else if (*first == '-') {
				if (!parseComment(p, first+1, last)) return false;
			}
			else {
				return parse_error(p, Unexpected_char, first);
			}
		}
		else if (ch == '?') {
			if (!parseProcessingInstruction(p, first+1, last)) return false;
		}
		else {
			return parse_error(p, Unexpected_char, first);
		}
		
		first = p.parsed;
	}
}


} // namespace azp

	bool cb (void *, azp::ParserTypes type, const azp::string_view_t& val) {
		static const char * tab[] = { "Tag_open",
										"Tag_close",
										"Attribute_name",
										"Attribute_value",
										"Text",
										"N/a","N/a","N/a",
									};
		if (val.len) printf("%s   %.*s\n", tab[type], val.len, val.str);
		else printf("%s\n", tab[type]);
		return true;
	}

int main() {
	char vec[][100] = {//"<tag></tag>", "<tag> a </tag>",
					"<tag><tag></tag>a<tag>&apos;</tag></tag>",
					// "<tag a='1'/>",
					// "<tag a='1' b=\"2\"/>",
					};
						
	for (auto s : vec) {
		azp::parser_t p;
		p.set_callback(cb, 0);
		parseXml(p, s, s+strlen(s));
	}
}
