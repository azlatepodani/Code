#include <stdlib.h>
#include <locale.h>
#include "azp_json.h"



parser_t::parser_t() {
	parsed = nullptr;
	current_type = Undefined;
	context = nullptr;
	callback = [](void*, enum ParserTypes, value_t&) { return true; };
	recursion = 0;
	max_recursion = 16;
}



static char * skip_wspace(char * first, char * last);
static bool parseJsonObject(parser_base_t& p, char * first, char * last);
static bool parseJsonArray(parser_base_t& p, char * first, char * last);
static bool parseJsonScalarV(parser_base_t& p, char * first, char * last);


static bool parseJson(parser_base_t& p, char * first, char * last) {
	first = skip_wspace(first, last);
	if (first == last) {
		return false;
	}
	
	if (*first == '{') return parseJsonObject(p, first+1, last);
	if (*first == '[') return parseJsonArray(p, first+1, last);
	return parseJsonScalarV(p, first, last);
}

bool parseJson(parser_t& p, char * first, char * last) {
	return parseJson(static_cast<parser_base_t&>(p), first, last);
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
static bool unescapeUnicodeChar(parser_base_t& p, char * first, char * last, char ** pCur) {
	if (last-first < 4) return false;
	
	unsigned u32 = 0;
	
	//
#define load_hex() 								\
	if (*first >= '0' && *first <='9') {		\
		u32 |= *first - '0';					\
	}											\
	else if ((*first|0x20) >= 'a' && (*first|0x20) <='z') {	\
		u32 |= (*first|0x20) - 'a' + 10;		\
	}											\
	else return false
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
		if ((u32 >> 10) != 0x36) return false; // incorrect bit pattern
		
		// surrogate pair
		if (last-first < 6) return false;
		
		if (*first != '\\' || first[1] != 'u') return false;
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
		else return false;
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
	return true;
#undef load_hex
}


static bool validUtf8(char * first, char * last) {
	return true;
}


bool call_string_callback(parser_base_t& p, char* start, char * end) {
	value_t val;
	val.string = start;
	val.length = end-start;
	return p.callback(p.context, p.current_type, val);
}


// assumes that '"' was already parsed
static bool parseString(parser_base_t& p, char * first, char * last) {
	auto n = last-first;
	auto start = first;
	auto cur = first;
	
	for (;n;--n) {
		if (*first == '"') {
			if (!validUtf8(start, cur)) return false;
			p.parsed = first+1;
			return call_string_callback(p, start, cur);
		}
		
		if (*first != '\\') {
			*cur++ = *first++;
			continue;
		}

		first++;
		n--;
		if (!n) return false;
		
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
				return false;
			}
			
			first++;
			cur++;
		}
		else {
			if (!unescapeUnicodeChar(p, first+1, last, &cur)) {
				return false;
			}
			
			first = p.parsed;
			n = last - first + 1;	// + 1 is needed for the decrement of the for loop
		}
	}
	
	return false;
}

// assumes first != last
static bool parseNumber(parser_base_t& p, char * first, char * last) {
	char buf[64];
	char * cur = buf;
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
	*(last-1) = savedCh;
	
	if (haveDot && !haveExp && !haveDotDigit) return false;
	if (haveExp && !haveExpDigit) return false;
	if (!haveDigit) return false;
	
	bool result;
	if (haveDot || haveExp) {
		// TODO: error checking
		auto old = setlocale(LC_NUMERIC, "C");
		value_t val;
		val.number = atof(buf);
		setlocale(LC_NUMERIC, old);

		result = p.callback(p.context, Number_float, val);
	}
	else {
		value_t val;
		val.integer = atoll(buf);
		result = p.callback(p.context, Number_int, val);
	}
	
	p.parsed = first;
	return result;
}


// assumes 't' was already parsed
static bool parseTrue(parser_base_t& p, char * first, char * last) {
	if (last-first < 3) return false;
	
	unsigned v = *first;
	v |= unsigned(first[1]) << 8;
	v |= unsigned(first[2]) << 16;
	
	if (v != '\0eur') return false;
	p.parsed = first + 3;
	value_t val;
	return p.callback(p.context, Bool_true, val);
}


// assumes 'f' was already parsed
static bool parseFalse(parser_base_t& p, char * first, char * last) {
	if (last-first < 3) return false;
	
	unsigned v = *first;
	v |= unsigned(first[1]) << 8;
	v |= unsigned(first[2]) << 16;
	v |= unsigned(first[3]) << 24;
	
	if (v != 'esla') return false;
	p.parsed = first + 4;
	value_t val;
	return p.callback(p.context, Bool_false, val);
}

// assumes 'n' was already parsed
static bool parseNull(parser_base_t& p, char * first, char * last) {
	if (last-first < 3) return false;
	
	unsigned v = *first;
	v |= unsigned(first[1]) << 8;
	v |= unsigned(first[2]) << 16;
	
	if (v != '\0llu') return false;
	p.parsed = first + 3;
	value_t val;
	return p.callback(p.context, Null_val, val);
}


static bool parseJsonScalarV(parser_base_t& p, char * first, char * last) {
	if (*first == '"') {
		p.current_type = String_val;
		return parseString(p, first+1, last);
	}

	if (*first >= '0' && *first <= '9' || *first == '-' || *first == '+') return parseNumber(p, first, last);
	if (*first == 't') return parseTrue(p, first+1, last);
	if (*first == 'f') return parseFalse(p, first+1, last);
	if (*first == 'n') return parseNull(p, first+1, last);
	return false;
}

// assumes that '{' was already parsed
static bool parseJsonObject(parser_base_t& p, char * first, char * last) {
	value_t val;
	if (!p.callback(p.context, Object_begin, val)) return false;

	first = skip_wspace(first, last);
	if (first == last) return false;
	
	if (*first == '}') {
		p.parsed = first + 1;
		return p.callback(p.context, Object_end, val);
	}
	
	for (;;) {
		// name
		if (*first != '"') return false;
		
		p.current_type = Object_key;
		if (!parseString(p, first+1, last)) return false;
		
		first = skip_wspace(p.parsed, last);
		if (first == last) return false;
		
		if (*first != ':') return false;
		
		// value
		if (!parseJson(p, first+1, last)) return false;
		
		first = skip_wspace(p.parsed, last);
		if (first == last) return false;

		if (*first == ',') {
			first = skip_wspace(first+1, last);
			if (first == last) return false;
		}
		else break;
	}
	
	if (*first == '}') {
		p.parsed = first + 1;
		return p.callback(p.context, Object_end, val);
	}
	
	return false;
}

// assumes that '[' was already parsed
static bool parseJsonArray(parser_base_t& p, char * first, char * last) {
	value_t val;
	if (!p.callback(p.context, Array_begin, val)) return false;

	first = skip_wspace(first, last);
	if (first == last) return false;
	
	if (*first == ']') {
		p.parsed = first + 1;
		return p.callback(p.context, Array_end, val);
	}
	
	for (;;) {
		// value
		if (!parseJson(p, first, last)) return false;
		
		first = skip_wspace(p.parsed, last);
		if (first == last) return false;

		if (*first == ',') {
			first = skip_wspace(first+1, last);
			if (first == last) return false;
		}
		else break;
	}
	
	if (*first == ']') {
		p.parsed = first + 1;
		return p.callback(p.context, Array_end, val);
	}
	
	return false;
}





/*

void main() {
	char a[] = "\"string\\u0020\\udBC0\\udC00\\\\\"";
	char b[] = "213.e1";
	char c[] = "123";
	char d[] = "{\"name\":false}";
	char e[] = "{\"name2\":true, \"name\n3\":  \t{}}";
	char f[] = "[null]";
	char g[] = "[true, \"lala\",\r  \t[{}]]";
	
	{
	parser_base_t p = {0};
	parseJson(p, a, a+sizeof(a)-1);
	}
	{
	parser_base_t p = {0};
	parseJson(p, b, b+sizeof(b)-1);
	}
	{
	parser_base_t p = {0};
	parseJson(p, c, c+sizeof(c)-1);
	}
	{
	parser_base_t p = {0};
	parseJson(p, d, d+sizeof(d)-1);
	}
	{
	parser_base_t p = {0};
	parseJson(p, e, e+sizeof(e)-1);
	}
	{
	parser_base_t p = {0};
	parseJson(p, f, f+sizeof(f)-1);
	}
	{
	parser_base_t p = {0};
	parseJson(p, g, g+sizeof(g)-1);
	}
}

*/