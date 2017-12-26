#include <stdlib.h>
#include <locale.h>
#include "azp_json.h"

namespace azp {

parser_t::parser_t() {
	parsed = nullptr;
	context = nullptr;
	callback = [](void*, ParserTypes, const value_t&) { return true; };
	recursion = 0;
	max_recursion = 16;
	error = No_error;
	err_position = 0;
	_first = nullptr;
}

// Internal type.
struct parse_exception {
	parse_exception(parser_base_t& p, ParserErrors err, const char * curPtr)
	{
		p.error = err;
		p.err_position = curPtr - p._first;
	}
};


static char * skip_wspace(char * first, char * last);
static void parseJsonObject(parser_base_t& p, char * first, char * last);
static void parseJsonArray(parser_base_t& p, char * first, char * last);
static void parseJsonScalarV(parser_base_t& p, char * first, char * last);


struct dec_on_exit {
	dec_on_exit(int& val) : val(val) {}
	~dec_on_exit() { --val; }
	int& val;
};

static void parseJson(parser_base_t& p, char * first, char * last) {
	if (++p.recursion >= p.max_recursion) throw parse_exception(p, Max_recursion, first);
	
	dec_on_exit de(p.recursion);
	
	first = skip_wspace(first, last);
	if (first == last) {
		throw parse_exception(p, No_value, first);
	}
	
	if (*first == '{') { parseJsonObject(p, first+1, last); return; }
	if (*first == '[') { parseJsonArray(p, first+1, last); return; }
	parseJsonScalarV(p, first, last);
}

bool parseJson(parser_t& p, char * first, char * last) {
	char* old = nullptr;
	
	try {
		p._first = first;
		
		old = setlocale(LC_NUMERIC, "C");
		if (!old) {
			throw parse_exception(p, Runtime_error, first);
		}
		
		parseJson(static_cast<parser_base_t&>(p), first, last);
		
		if (!setlocale(LC_NUMERIC, old)) {
			old = nullptr;
			throw parse_exception(p, Runtime_error, first);
		}
		
		return true;
	}
	catch (parse_exception&) {
		if (old) setlocale(LC_NUMERIC, old);
		return false;
	}
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


// assumes that '\u' was already parsed
static void unescapeUnicodeChar(parser_base_t& p, char * first, char * last, char ** pCur) {
	if (last-first < 4) throw parse_exception(p, Invalid_escape, first);
	
	unsigned u32 = 0;
	
	//
#define load_hex() 								\
	if (*first >= '0' && *first <='9') {		\
		u32 |= *first - '0';					\
	}											\
	else if ((*first|0x20) >= 'a' && (*first|0x20) <='z') {	\
		u32 |= (*first|0x20) - 'a' + 10;		\
	}											\
	else throw parse_exception(p, Invalid_escape, first);
	//
	
	load_hex();
	u32 <<= 4;
	first++;
	
	load_hex();
	u32 <<= 4;
	first++;
	
	load_hex();
	u32 <<= 4;
	first++;

	load_hex();
	first++;

	if (u32 >= 0xD800 && u32 < 0xE000) {
		if ((u32 >> 10) != 0x36) { 	// incorrect bit pattern
			throw parse_exception(p, Invalid_escape, first-4);
		}
		
		// surrogate pair
		if (last-first < 6) {
			throw parse_exception(p, Invalid_escape, first);
		}
		
		if (*first != '\\' || first[1] != 'u') {
			throw parse_exception(p, Invalid_escape, first-6);
		}
		
		first += 2;
		
		auto saved = u32;
		u32 = 0;
		
		load_hex();
		u32 <<= 4;
		first++;
		
		load_hex();
		u32 <<= 4;
		first++;
		
		load_hex();
		u32 <<= 4;
		first++;

		load_hex();
		first++;

		if (u32 >= 0xD800 && u32 < 0xE000 && (u32 >> 10) == 0x37) { // second word
			u32 = (u32 & 0x3FF) + ((saved & 0x3FF) << 10) + 0x10000;	// convert to unicode code point
		}
		else {
			throw parse_exception(p, Invalid_escape, first-4);
		}
	}
	
	// write as UTF-8
	auto cur = *pCur;
	if (u32 < 128) {
		*cur++ = (char)u32;
	}
	else if (u32 < 0x800) {
		*cur++ = char((u32 >> 6) | 0xC0);
		*cur++ = char((u32 & 0x3F) | 0x80);
	}
	else if (u32 < 0x10000) {
		*cur++ = char((u32 >> 12) | 0xE0);
		*cur++ = char(((u32 >> 6) & 0x3F) | 0x80);
		*cur++ = char((u32 & 0x3F) | 0x80);
	}
	else {
		*cur++ = char((u32 >> 18) | 0xF0);
		*cur++ = char(((u32 >> 12) & 0x3F) | 0x80);
		*cur++ = char(((u32 >> 6) & 0x3F) | 0x80);
		*cur++ = char((u32 & 0x3F) | 0x80);
	}
	
	*pCur = cur;
	p.parsed = first;
#undef load_hex
}


void call_string_callback(parser_base_t& p, char* start, char * end, ParserTypes report_type) {
	value_t val;
	val.string = start;
	val.length = end-start;
	if (!p.callback(p.context, report_type, val)) {
		throw parse_exception(p, User_requested, start);
	}
}


// assumes that '"' was already parsed
static void parseString(parser_base_t& p, char * first, char * last, ParserTypes report_type) {
	auto n = last-first;
	auto start = first;
	auto cur = first;
	
	for (;n;--n) {
		if (*first == '"') {
			p.parsed = first+1;
			call_string_callback(p, start, cur, report_type);
			return;
		}
		
		if (*first != '\\') {
			*cur++ = *first++;
			continue;
		}

		first++;
		n--;
		if (!n) {
			throw parse_exception(p, No_string_end, start-1);
		}
		
		if (*first != 'u') {
			if ((*first == '"') | (*first == '\\') | (*first == '/')) {
				*cur = *first;
			}
			else if (*first == 'n') {
				*cur = '\n';
			}
			else if (*first == 'r') {
				*cur = '\r';
			}
			else if (*first == 't') {
				*cur = '\t';
			}
			else if (*first == 'b') {
				*cur = '\b';
			}
			else if (*first == 'f') {
				*cur = '\f';
			}
			else {
				throw parse_exception(p, Invalid_escape, first);
			}
			
			first++;
			cur++;
		}
		else {
			unescapeUnicodeChar(p, first+1, last, &cur);
			
			first = p.parsed;
			n = last - first + 1;	// + 1 is needed for the decrement of the for loop
		}
	}

	throw parse_exception(p, No_string_end, start-1);
}

// assumes first != last
static void parseNumber(parser_base_t& p, char * first, char * last) {
	char buf[64];
	char * cur = buf;
	char * savedFirst = first;
	auto savedCh = *(last-1);
	*(last-1) = 0; // sentinel
	
	// optional +/- signs
	if (*first == '-') *cur++ = *first++;
	if (*first == '+') first++;

	bool haveDigit = false;
	bool haveDot = false;
	bool haveExp = false;
	bool haveDotDigit = false;
	bool haveExpDigit = false;

	// digits
	while (*first >= '0' && *first <= '9') {
		*cur++ = *first++;
		haveDigit = true;
	}
	
	// optional '.<digits>'
	if (*first == '.') {
		haveDot = true;
		*cur++ = *first++;
		
		// optional digits after the dot
		while (*first >= '0' && *first <= '9') {
			*cur++ = *first++;
			haveDotDigit = true;
		}
	}
	
	// optional 'e[sign]<digits>'
	if ((*first|0x20) == 'e') {
		haveExp = true;
		
		//if (!haveDot) *cur++ = '.';				// sscanf needs a '.' before 'e'
		//if (!haveDotDigit) *cur++ = '0';		// sscanf needs a digit between '.' and 'e'
		*cur++ = *first++;

		// optional +/- signs
		if (*first == '-' || *first == '+') *cur++ = *first++;
	
		// digits after the exponent
		while (*first >= '0' && *first <= '9') {
			*cur++ = *first++;
			haveExpDigit = true;
		}
	}
	
	if (first == (last-1) && savedCh >= '0' && savedCh <= '9') {
		*cur++ = savedCh;
		first++;
		if (haveExp) haveExpDigit = true;
		else if (haveDot) haveDotDigit = true;
		else haveDigit = true;
	}
	
	*cur = 0;
	*(last-1) = savedCh;	// replace the sentinel with the saved character
	
	if (haveDot && !haveExp && !haveDotDigit ||
		haveExp && !haveExpDigit ||
		!haveDigit)
	{
		throw parse_exception(p, Invalid_number, savedFirst);
	}
	
	bool result;
	if (haveDot || haveExp) {
		value_t val;
		val.number = atof(buf);
		result = p.callback(p.context, Number_float, val);
	}
	else {
		value_t val;
		val.integer = atoll(buf);
		result = p.callback(p.context, Number_int, val);
	}
	
	p.parsed = first;
	if (!result) {
		throw parse_exception(p, User_requested, first);
	}
}


// assumes 't' was already parsed
static void parseTrue(parser_base_t& p, char * first, char * last) {
	unsigned v;
	
	if (last-first < 4) {
		if (last-first < 3) {
			throw parse_exception(p, Invalid_token, first-1);
		}
		
		v = *first;
		v |= unsigned(first[1]) << 8;
		v |= unsigned(first[2]) << 16;
	}
	else {
		v = *(unsigned *)first & 0xFFFFFF;
	}
	
	if (v != '\0eur') {
		throw parse_exception(p, Invalid_token, first-1);
	}
	
	p.parsed = first + 3;
	
	value_t val;
	if (!p.callback(p.context, Bool_true, val)) {
		throw parse_exception(p, User_requested, p.parsed);
	}
}


// assumes 'f' was already parsed
static void parseFalse(parser_base_t& p, char * first, char * last) {
	if (last-first < 4) {
		throw parse_exception(p, Invalid_token, first-1);
	}
	
	unsigned v = *(unsigned *)first;
	
	if (v != 'esla') {
		throw parse_exception(p, Invalid_token, first-1);
	}
	
	p.parsed = first + 4;
	
	value_t val;
	if (!p.callback(p.context, Bool_true, val)) {
		throw parse_exception(p, User_requested, p.parsed);
	}
}

// assumes 'n' was already parsed
static void parseNull(parser_base_t& p, char * first, char * last) {
	unsigned v;
	
	if (last-first < 4) {
		if (last-first < 3) {
			throw parse_exception(p, Invalid_token, first-1);
		}
		
		v = *first;
		v |= unsigned(first[1]) << 8;
		v |= unsigned(first[2]) << 16;
	}
	else {
		v = *(unsigned *)first & 0xFFFFFF;
	}
	
	if (v != '\0llu') {
		throw parse_exception(p, Invalid_token, first-1);
	}
	
	p.parsed = first + 3;
	
	value_t val;
	if (!p.callback(p.context, Bool_true, val)) {
		throw parse_exception(p, User_requested, p.parsed);
	}
}


static void parseJsonScalarV(parser_base_t& p, char * first, char * last) {
	if (*first == '"') {
		parseString(p, first+1, last, String_val);
		return;
	}

	if (*first >= '0' && *first <= '9' || *first == '-' || *first == '+') {
		parseNumber(p, first, last);
		return;
	}
	
	if (*first == 't') { parseTrue(p, first+1, last); return; }
	if (*first == 'f') { parseFalse(p, first+1, last); return; }
	if (*first == 'n') { parseNull(p, first+1, last); return; }
	
	throw parse_exception(p, No_value, first);
}

// assumes that '{' was already parsed
static void parseJsonObject(parser_base_t& p, char * first, char * last) {
	value_t val;
	if (!p.callback(p.context, Object_begin, val)) {
		throw parse_exception(p, User_requested, first);
	}

	first = skip_wspace(first, last);
	if (first == last) {
		throw parse_exception(p, Unbalanced_collection, first);
	}
	
	if (*first == '}') {
		p.parsed = first + 1;
		if (!p.callback(p.context, Object_end, val)) {
			throw parse_exception(p, User_requested, p.parsed);
		}
		
		return;
	}
	
	for (;;) {
		// name
		if (*first != '"') {
			throw parse_exception(p, Expected_key, first);
		}
		
		parseString(p, first+1, last, Object_key);
		
		first = skip_wspace(p.parsed, last);
		if (first == last || *first != ':') {
			throw parse_exception(p, Expected_colon, first);
		}
		
		// value
		parseJson(p, first+1, last);
		
		first = skip_wspace(p.parsed, last);
		if (first == last) {
			throw parse_exception(p, Unbalanced_collection, first);
		}

		if (*first == ',') {
			first = skip_wspace(first+1, last);
			if (first == last) {
				throw parse_exception(p, Expected_key, first);
			}
		}
		else break;
	}
	
	if (*first == '}') {
		p.parsed = first + 1;
		if (!p.callback(p.context, Object_end, val)) {
			throw parse_exception(p, User_requested, p.parsed);
		}
		
		return;
	}
	
	throw parse_exception(p, Unbalanced_collection, first);
}

// assumes that '[' was already parsed
static void parseJsonArray(parser_base_t& p, char * first, char * last) {
	value_t val;
	if (!p.callback(p.context, Array_begin, val)) {
		throw parse_exception(p, User_requested, first);
	}
	
	first = skip_wspace(first, last);
	if (first == last) {
		throw parse_exception(p, Unbalanced_collection, first);
	}
	
	if (*first == ']') {
		p.parsed = first + 1;
		if (!p.callback(p.context, Array_end, val)) {
			throw parse_exception(p, User_requested, p.parsed);
		}
		
		return;
	}
	
	for (;;) {
		// value
		parseJson(p, first, last);
		
		first = skip_wspace(p.parsed, last);
		if (first == last) {
			throw parse_exception(p, Unbalanced_collection, first);
		}

		if (*first == ',') {
			first = skip_wspace(first+1, last);
			if (first == last) {
				throw parse_exception(p, No_value, first);
			}
		}
		else break;
	}
	
	if (*first == ']') {
		p.parsed = first + 1;
		if (!p.callback(p.context, Array_end, val)) {
			throw parse_exception(p, User_requested, p.parsed);
		}
		
		return;
	}
	
	throw parse_exception(p, Unbalanced_collection, first);
}


} // namespace azp

