#include <stdlib.h>
#include <locale.h>
#include <cmath>
#include <climits>
#include <string.h>
#include "azp_xml.h"
#include <memory>
// #include <string>
// #include <iostream>
// #include <fstream>
// #if defined(_MSC_VER)
// #include <windows.h>
// #endif


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
    
    p.parsed = nullptr;    // 'parsed' would be invalid after return
    
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
    tag = string_view_t{0,0};
    ver = string_view_t{0,0};
    enc = string_view_t{0,0};
    sddecl = string_view_t{0,0};
}


static char * skip_wspace(char * first, char * last);
static bool parseXmlTag(parser_base_t& p, char * first, char * last);
static bool parseXmlProlog(parser_base_t& p, char * first, char * last);
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
    if (!parseXmlProlog(p, first, last)) return false;
    first = p.parsed;
    
    if (first != last) {
        return parseXmlTag(p, first, last);
    }
    
    return parse_error(p, No_value, first);
}


static bool parse_error(parser_base_t& p, ParserErrors err, const char * curPtr)
{
    p.error = err;
    p.err_position = curPtr - p._first;
    return false;
}


static char * skip_wspace(char * first, char * last) {
    auto n = last - first;
    if (n && *first > ' ') return first;    // likely
    for (; n; --n) {
        if ((*first == ' ') | (*first == '\n') | (*first == '\r') | (*first == '\t')) ++first;
        else return first;
    }
    return first;
}


inline bool isDigit(char c) {
    return (uint32_t(c) - '0') < 10;    // if 'c' isn't in ('0' .. '9'), the result is >= 10
}


inline bool isHexAlpha(char c) {
    return ((uint32_t(c)|0x20) - 'a') <= ('f' - 'a');    // convert capital letters to small letters and test
}


static bool parseXmlVersion(parser_base_t& p, char * first, char * last)
{
    do {
        if (last - first < 15 || memcmp(first, "version", 7) != 0) break;    // "version='1.x'?>"
        
        first = skip_wspace(first+7, last);
        if (first == last || *first != '=') break;
        
        first = skip_wspace(first+1, last);
        if (first == last) break;
        
        auto ch = *first;
        
        if (ch != '"' && ch != '\'') break;
        
        if (last - first < 6) break;  // 1.x"?>
        
        auto savedFirst = first;
        
        if (*(++first) != '1' || *(++first) != '.' ||!isDigit(*(++first)) || *(++first) != ch) break;
        
        p.ver.str = savedFirst+1;
        p.ver.len = size_t(first - savedFirst - 1);
        
        p.parsed = first+1;
        return true;
    }
    while (0);
    
    return parse_error(p, Expected_version_decl, first);
}


static bool isAtoZ(char c) {
    return ((uint32_t(c)|0x20) - 'a') <= ('z' - 'a');
}


static bool parseXmlEncoding(parser_base_t& p, char * first, char * last)
{
    p.parsed = first;
    
    do {
        if (last - first < 14 || memcmp(first, "encoding", 8) != 0) return true;    // "encoding='e'?>"
        
        first = skip_wspace(first+8, last);
        if (first == last || *first != '=') break;
        
        first = skip_wspace(first+1, last);
        if (first == last) break;
        
        auto ch = *first;
        
        if (ch != '"' && ch != '\'') break;
        
        auto n = last - ++first;
        if (n < 4) break;  // e'?>
        
        auto savedFirst = first;
        
        if (!isAtoZ(*first)) goto Out;
        
        ++first; --n;
        
        bool foundEnd = false;
        
        while (n--) {
            auto c = *first;
            
            if (c == ch) { foundEnd = true; break; }
            
            if (!isAtoZ(c) && !isDigit(c)) {
                if ((c != '.') & (c != '_') & (c != '-')) goto Out;
            }
            
            ++first;
        }
        
        if (!foundEnd) break;
        
        p.enc.str = savedFirst;
        p.enc.len = size_t(first - savedFirst);
        
        p.parsed = first+1;
        return true;
    }
    while (0);
    
Out:
    return parse_error(p, Expected_encoding, first);
}


static bool parseSdDecl(parser_base_t& p, char * first, char * last)
{
    p.parsed = first;
    
    do {
        if (last - first < 17 || memcmp(first, "standalone", 10) != 0) break;    // "standalone='no'?>"
        
        first = skip_wspace(first+10, last);
        if (first == last || *first != '=') break;
        
        first = skip_wspace(first+1, last);
        if (first == last) break;
        
        auto ch = *first;
        
        if (ch != '"' && ch != '\'') break;
        
        if (last - ++first < 5) break;  // no'?>
        
        auto savedFirst = first;
        
        if (*first == 'y') {
            if (*(++first) != 'e' || *(++first) != 's') break;
        }
        else if (*first == 'n') {
            if (*(++first) != 'o') break;
        }
        else break;
        
        if (*(++first) != ch) break;    // closing quote
        
        p.sddecl.str = savedFirst;
        p.sddecl.len = size_t(first - savedFirst);
        
        p.parsed = first+1;
        return true;
    }
    while (0);
    
    return parse_error(p, Expected_sddecl, first);
}


static bool parseXmlDecl(parser_base_t& p, char * first, char * last) {
    p.parsed = first;    // this element is optional
    if (last - first < 21) return true; // "<?xml version='1.x'?>"
    
    if ((*first != '<') | (first[1] != '?')) return true;

    first += 2;

    if ((first[0]|0x20) != 'x' || (first[1]|0x20) != 'm' || (first[2]|0x20) != 'l') return true;

    auto verptr = skip_wspace(first+3, last);
    if (verptr == first+3) return true;
    
    if (!parseXmlVersion(p, verptr, last)) return false;
    
    first = skip_wspace(p.parsed, last);
    if (first == last) return parse_error(p, Expected_pi_end, first);
    
    if (!parseXmlEncoding(p, first, last)) return false;
    
    first = skip_wspace(p.parsed, last);
    if (first == last) return parse_error(p, Expected_pi_end, first);
    
    if (!parseSdDecl(p, first, last)) return false;
    
    first = skip_wspace(p.parsed, last);
    if (last-first < 2) return parse_error(p, Expected_pi_end, first);
    
    if (*first != '?' || first[1] != '>') return parse_error(p, Expected_pi_end, first);
    
    p.parsed = first+2;
    
    return true;
}


static bool parseComment(parser_base_t& p, char * first, char * last);
static bool parseProcessingInstruction(parser_base_t& p, char * first, char * last);


static bool parseXmlProlog(parser_base_t& p, char * first, char * last) {
    if (!parseXmlDecl(p, first, last)) return false;
    
    first = p.parsed;
    
    while (true) {
        first = skip_wspace(first, last);
        
        if (last-first < 4) return parse_error(p, No_value, first);        // <a/>
        if (*first != '<') return parse_error(p, Unexpected_char, first);
        
        auto ch = *(++first);

        if (ch == '!') {
            ++first;
            
            if (*first == '-') {
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
            p.parsed = first;    // we expect the root node's name here.
            break;
        }
        
        first = p.parsed;
    }

    return true;
}


static bool parseName(parser_base_t& p, char * first, char * last);
static bool parseAttrName(parser_base_t& p, char * first, char * last);
static bool parseTagAttribute(parser_base_t& p, char * first, char * last);
static bool parseTagBodyAndClosingTag(parser_base_t& p, char * first, char * last);


#define wrap_user_callback(RT, VAL, ERR_HINT)                     \
                    (p.callback(p.context, (RT), (VAL)) ? true    \
                    : parse_error(p, User_requested, (ERR_HINT)))
                    

// assumes that '<' was already parsed
static bool parseXmlTag(parser_base_t& p, char * first, char * last) {
    if (++p.recursion >= p.max_recursion) return parse_error(p, Max_recursion, first);
    
    dec_on_exit de(p.recursion);

    if (!parseName(p, first, last)) return false;
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
        auto ch = *first;
        if ((uint32_t(uint8_t(ch))|0x20) < 'a') {
            switch (ch) {
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
                    return first;
                    
                default:;
            }
        }
        
        ++first;
    }

    return first;
}


static bool parseName(parser_base_t& p, char * first, char * last) {
    auto savedFirst = first;
    first = findNameEnd(first, last);
    if (savedFirst == first) return parse_error(p, Expected_name, first);
    
    p.parsed = first;
    
    p.tag = string_view_t{savedFirst, size_t(first-savedFirst)};
    return wrap_user_callback(Tag_open, p.tag, p.parsed);
}


static bool parseAttrName(parser_base_t& p, char * first, char * last) {
    auto savedFirst = first;
    first = findNameEnd(first, last);
    if (savedFirst == first) return parse_error(p, Expected_name, first);
    
    p.parsed = first;
    
    string_view_t val{savedFirst, size_t(first-savedFirst)};
    return wrap_user_callback(Attribute_name, val, p.parsed);
}


static bool expandReference(parser_base_t& p, char * first, char * last, char * cur, char *& textEnd);


static bool parseAttrValue(parser_base_t& p, char * first, char * last) {
    auto ch = *first;
    if ((ch != '"') & (ch != '\'')) return parse_error(p, Unexpected_char, first);
    
    auto n = last - ++first;
    
    auto savedFirst = first;
    auto c = *first;
    bool foundEnd = false;
    
    while (n--) {
        if (c == ch) { foundEnd = true; break; }
        if ((c == '&') | (c == '<')) break;

        c = *(++first);
    }
    
    if (c == '<') return parse_error(p, Unexpected_char, first);

    auto textEnd = first;
    
    if (!foundEnd) {
        while ((c == '&') & (n != 0)) {
            auto savedEnd = textEnd;
            if (!expandReference(p, first+1, last, savedEnd, textEnd)) return false;
            
            first = p.parsed;
            
            n = last - first;
            if (!n) break;
            
            c = *first;
            while (n--) { // maybe copy after...
                if (c == ch) { foundEnd = true; break; }
                if ((c == '&') | (c == '<')) break;
                
                *textEnd++ = c;
                c = *(++first);
            }
        }
        
        if (c == '<') return parse_error(p, Unexpected_char, first);
        
        if (!foundEnd) return parse_error(p, Expected_quote, first);
    }
    
    p.parsed = first + 1;
    
    string_view_t val{savedFirst, size_t(first - savedFirst)};
    return wrap_user_callback(Attribute_value, val, p.parsed);
}


static bool parseTagAttribute(parser_base_t& p, char * first, char * last) {
    if (!parseAttrName(p, first, last)) return false;
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
            auto ch = *first;
            if (isDigit(ch)) {
                num <<= 4;
                num |= ch - '0';
            }
            else if (isHexAlpha(ch)) {
                num <<= 4;
                num |= (ch|0x20) - 'a' + 10;
            }
            else {
                break;
            }
            
            ++first;
        }
    }
    else {
        savedFirst = first;
        if (n > 6) n = 6;
        
        while (n--) {
            auto ch = *first;
            if (isDigit(ch)) {
                num = (num << 3) + (num << 1);
                num += ch - '0';
            }
            else {
                break;
            }
            
            ++first;
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
        if (first == last || *first != ';') return parse_error(p, Expected_semicolon, first);
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
    
    while ((ch == '&') & (n != 0)) {
        auto savedEnd = textEnd;
        if (!expandReference(p, first+1, last, savedEnd, textEnd)) return false;
        
        first = p.parsed;
        
        n = last - first;
        if (!n) break;
        
        ch = *first;
        while (n && ((ch != '<') & (ch != '&'))) { // maybe copy after...
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
    
    if (nameEnd - first != p.tag.len || memcmp(first, p.tag.str, p.tag.len) != 0)
        return parse_error(p, Unbalanced_collection, first);
    
    first = skip_wspace(nameEnd, last);
    if (first == last || *first != '>') return parse_error(p, Expected_closing_brace, first);
    
    p.parsed = first+1;
    
    return true;
}


static bool parseCDataSect(parser_base_t& p, char * first, char * last) {
    auto n = last-first;
    if (n < 9) return parse_error(p, Expected_cdata, first); // strlen("CDATA[]]>") = 9
    
    if (memcmp(first, "CDATA[", 6) != 0) return parse_error(p, Expected_cdata, first);
    
    first += 6; n -= 6;
    auto savedFirst = first;
    bool foundEnd = false;
    
    while (n--) {
        auto ch = *first;
        if (ch == '>') {
            if ((*(first - 1) == ']') & (*(first-2) == ']')) {
                foundEnd = true;
                break;
            }
        }
        ++first;
    }
    
    if (!foundEnd) return parse_error(p, Expected_cdata_end, first);
    
    p.parsed = first;
    string_view_t val{savedFirst, size_t(first - savedFirst - 2)};
    return wrap_user_callback(Cdata_text, val, first);
}


static bool parseComment(parser_base_t& p, char * first, char * last) {
    auto n = last-first;
    if (n < 5) return parse_error(p, Expected_cdata, first); // strlen("- -->") = 5
    
    if (*first != '-' != 0) return parse_error(p, Expected_comment, first);
    
    ++first; --n;
    bool foundEnd = false;
    
    while (n--) {
        auto ch = *first;
        if (ch == '>') {
            if ((*(first - 1) == '-') & (*(first-2) == '-')) {
                foundEnd = (*(first-3) != '-');    // ---> is not allowed
                break;
            }
        }
        ++first;
    }
    
    if (!foundEnd) return parse_error(p, Expected_comment_end, first);
    
    p.parsed = first+1;
    return true;
}


static bool parseProcessingInstruction(parser_base_t& p, char * first, char * last) {
    auto n = last-first;
    if (n < 3)    return parse_error(p, Expected_cdata, first); // strlen("a?>") = 3
    
    auto nameEnd = findNameEnd(first, last);
    if (first == nameEnd) return parse_error(p, Unexpected_char, first);
    
    string_view_t val{first, size_t(nameEnd-first)};
    
    if (val.len == 3 && (first[0]|0x20) == 'x' && (first[1]|0x20) == 'm' && (first[2]|0x20) == 'l')
        return parse_error(p, Invalid_pi_name, first);
    
    if  (!wrap_user_callback(Pinstr_name, val, first)) return false;
    
    n = last - nameEnd;
    first = nameEnd;
    
    if ((n < 2) | (*first == '?') & (first[1] != '>')) return parse_error(p, Expected_pi_end, first);
    
    first = skip_wspace(first, last);
    if (first == nameEnd) return parse_error(p, Expected_pi_end, first);
    
    auto savedFirst = first;
    
    bool foundEnd = false;
    
    n = last - first;
    while (n--) {
        auto ch = *first;
        if (ch == '>') {
            if (*(first - 1) == '?') {
                foundEnd = true;
                break;
            }
        }
        ++first;
    }
    
    if (!foundEnd) return parse_error(p, Expected_pi_end, first);
    
    p.parsed = first+1;
    val = string_view_t{savedFirst, size_t(first - savedFirst - 1)};
    return wrap_user_callback(Pinstr_text, val, first);
}


static bool parseTagBodyAndClosingTag(parser_base_t& p, char * first, char * last)
{
    while (true) {
        if (!parseCharData(p, first, last)) return false;
        
        first = p.parsed;
        
        if (last - first < 4) return parse_error(p, Unbalanced_collection, first);    // a valid closing tag is at least '</a>'
        
        ++first;    // skip '<'
        
        auto ch = *first;
        
        if (ch == '/') return parseClosingTag(p, first+1, last);
    
        if ((ch != '!') & (ch != '?')) {
            auto bak = p.tag;
            if (!parseXmlTag(p, first, last)) return false;
            p.tag = bak;
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

    // bool cb (void *, azp::ParserTypes type, const azp::string_view_t& val) {
        // static const char * tab[] = { "Tag_open",
                                        // "Tag_close",
                                        // "Attribute_name",
                                        // "Attribute_value",
                                        // "Text",
                                        // "N/a","N/a","N/a",
                                    // };
        // if (val.len) printf("%s   %.*s\n", tab[type], (int)val.len, val.str);
        // else printf("%s\n", tab[type]);
        // return true;
    // }
    
    
 // #if defined(_MSC_VER)
	// inline std::string loadFile(const wchar_t * path) {
		// std::string str;
		// auto h = CreateFileW(path, GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);
		// if (h == INVALID_HANDLE_VALUE) {
			// printf("cannot open file  %d\n", GetLastError());
			// return std::string();
		// }
		// auto size = GetFileSize(h, 0);
		// str.resize(size);
		// if (!ReadFile(h, &str[0], (ULONG)str.size(), 0,0)) {
            // printf("cannot read file  %d\n", GetLastError());
		// }
		// CloseHandle(h);
		// return str;
	// }

// #else
	// inline std::string loadFile(const char * path) {
		// std::string str;
		// std::ifstream stm(path, std::ios::binary);
		
		// if (!stm.good()) {
			// printf("cannot open file\n");
			// return std::string();
		// }
		
		// stm.seekg(0, std::ios_base::end);
		// auto size = stm.tellg();
		// stm.seekg(0, std::ios_base::beg);
		
		// str.resize(size);
		// stm.read(&str[0], size);
		// return str;
	// }
// #endif // _MSC_VER   


// #if defined(_MSC_VER)
// int wmain(int argc, PWSTR argv[])
// {
	// SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	// if (GetThreadPriority(GetCurrentThread()) != THREAD_PRIORITY_HIGHEST) printf("Priority set failed\n");
	 
	// if (!SetThreadAffinityMask(GetCurrentThread(), 2)) printf("Affinity set failed\n");

// #else
// int main(int argc, char* argv[]) {
// #endif

    // //char vec[][100] = {//"<tag></tag>", "<tag> a </tag>",
                    // //"<?XmllL version='1.4'?><tag><tag></tag>a <![CDATA[<tag>&apos;</tag>]]></tag>",
                    // // "<tag a='1'/>",
                    // // "<tag a='1' b=\"2\"/>",
                    // //};
                    // //"C:\\Users\\Andrei-notebook\\Downloads\\rec00001output"
    // if (argc != 2) return -1;
                        
    // auto buf = loadFile(argv[1]);
    // //for (auto s : vec) {
        // azp::parser_t p;
        // p.set_callback(cb, 0);
        // if (!parseXml(p, &*buf.begin(), &*buf.end())) printf("problem\n");
    // //}
// }
