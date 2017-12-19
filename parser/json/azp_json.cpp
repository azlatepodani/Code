#include <stdlib.h>
#include <locale.h>
#include "azp_json.h"



parser_t::parser_t() {
	parsed = nullptr;
	context = nullptr;
	callback = [](void*, ParserTypes, const value_t&) { return true; };
	recursion = 0;
	max_recursion = 16;
	error = No_error;
	err_position = 0;
}


static bool parse_error(parser_base_t& p, ParserErrors err, const char * curPtr)
{
	p.error = err;
	p.err_position = curPtr - p._first;
	return false;
}


static char * skip_wspace(char * first, char * last);
static bool parseJsonObject(parser_base_t& p, char * first, char * last);
static bool parseJsonArray(parser_base_t& p, char * first, char * last);
static bool parseJsonScalarV(parser_base_t& p, char * first, char * last);


struct dec_on_exit {
	dec_on_exit(int& val) : val(val) {}
	~dec_on_exit() { --val; }
	int& val;
};


static bool parseJson(parser_base_t& p, char * first, char * last) {
	if (++p.recursion >= p.max_recursion) return parse_error(p, Max_recursion, first);
	
	dec_on_exit de(p.recursion);
	
	first = skip_wspace(first, last);
	if (first == last) {
		return parse_error(p, No_value, first);
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
	if (last-first < 4) return parse_error(p, Invalid_escape, first);
	
	unsigned u32 = 0;
	
	//
#define load_hex() 								\
	if (*first >= '0' && *first <='9') {		\
		u32 |= *first - '0';					\
	}											\
	else if ((*first|0x20) >= 'a' && (*first|0x20) <='z') {	\
		u32 |= (*first|0x20) - 'a' + 10;		\
	}											\
	else return parse_error(p, Invalid_escape, first)
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
		if ((u32 >> 10) != 0x36) {	// incorrect bit pattern
			return parse_error(p, Invalid_escape, first-4);
		}
		
		// surrogate pair
		if (last-first < 6) return parse_error(p, Invalid_escape, first);
		
		if (*first != '\\' || first[1] != 'u') return parse_error(p, Invalid_escape, first-6);
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

		if (u32 >= 0xD800 && u32 < 0xE000 && (u32 >> 10) == 0x37) {  // second word
			u32 = (u32 & 0x3FF) + ((saved & 0x3FF) << 10) + 0x10000; // convert to code point
		}
		else return parse_error(p, Invalid_escape, first-4);
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


#define wrap_user_callback(RT, VAL, ERR_HINT) 					\
					(p.callback(p.context, (RT), (VAL)) ? true	\
					: parse_error(p, User_requested, (ERR_HINT)))


static bool call_string_callback(parser_base_t& p, char* start, char * end,
								 ParserTypes report_type)
{
	value_t val;
	val.string = start;
	val.length = end-start;
	return wrap_user_callback(report_type, val, start);
}


// assumes that '"' was already parsed
static bool parseString(parser_base_t& p, char * first, char * last, ParserTypes report_type)
{
	auto n = last-first;
	auto start = first;
	auto cur = first;
	
	for (;n;--n) {
		if (*first == '"') {
			p.parsed = first+1;
			return call_string_callback(p, start, cur, report_type);
		}
		
		if (*first != '\\') {
			*cur++ = *first++;
			continue;
		}

		first++;
		n--;
		if (!n) return parse_error(p, No_string_end, start-1);
		
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
				return parse_error(p, Invalid_escape, first);
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
	
	return parse_error(p, No_string_end, start-1);
}


// assumes first != last
static bool parseNumber(parser_base_t& p, char * first, char * last) {
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
		return parse_error(p, Invalid_number, savedFirst);
	}
	
	bool result;
	if (haveDot | haveExp) {
		auto old = setlocale(LC_NUMERIC, "C");
		if (!old) {
			return parse_error(p, Runtime_error, savedFirst);
		}
		
		value_t val;
		val.number = atof(buf);
		
		if (!setlocale(LC_NUMERIC, old)) {
			return parse_error(p, Runtime_error, savedFirst);
		}

		result = wrap_user_callback(Number_float, val, first);
	}
	else {
		value_t val;
		val.integer = atoll(buf);
		result = wrap_user_callback(Number_float, val, first);
	}
	
	p.parsed = first;
	return result;
}


// assumes 't' was already parsed
static bool parseTrue(parser_base_t& p, char * first, char * last) {
	if (last-first < 3) return parse_error(p, Invalid_token, first-1);
	
	unsigned v = *first;
	v |= unsigned(first[1]) << 8;
	v |= unsigned(first[2]) << 16;
	
	if (v != '\0eur') return parse_error(p, Invalid_token, first-1);
	
	p.parsed = first + 3;
	value_t val;
	return wrap_user_callback(Bool_true, val, p.parsed);
}


// assumes 'f' was already parsed
static bool parseFalse(parser_base_t& p, char * first, char * last) {
	if (last-first < 4) return parse_error(p, Invalid_token, first-1);
	
	unsigned v = *first;
	v |= unsigned(first[1]) << 8;
	v |= unsigned(first[2]) << 16;
	v |= unsigned(first[3]) << 24;
	
	if (v != 'esla') return parse_error(p, Invalid_token, first-1);
	
	p.parsed = first + 4;
	value_t val;
	return wrap_user_callback(Bool_false, val, p.parsed);
}


// assumes 'n' was already parsed
static bool parseNull(parser_base_t& p, char * first, char * last) {
	if (last-first < 3) return parse_error(p, Invalid_token, first-1);
	
	unsigned v = *first;
	v |= unsigned(first[1]) << 8;
	v |= unsigned(first[2]) << 16;
	
	if (v != '\0llu') return parse_error(p, Invalid_token, first-1);
	
	p.parsed = first + 3;
	value_t val;
	return wrap_user_callback(Null_val, val, p.parsed);
}


static bool parseJsonScalarV(parser_base_t& p, char * first, char * last) {
	if (*first == '"') {
		return parseString(p, first+1, last, String_val);
	}

	if (*first >= '0' && *first <= '9' || *first == '-' || *first == '+') {
		return parseNumber(p, first, last);
	}
	
	if (*first == 't') return parseTrue(p, first+1, last);
	if (*first == 'f') return parseFalse(p, first+1, last);
	if (*first == 'n') return parseNull(p, first+1, last);
	
	return parse_error(p, No_value, first);
}


// assumes that '{' was already parsed
static bool parseJsonObject(parser_base_t& p, char * first, char * last) {
	value_t val;
	if (!wrap_user_callback(Object_begin, val, first)) return false;

	first = skip_wspace(first, last);
	if (first == last) return parse_error(p, Unbalanced_collection, first);
	
	if (*first == '}') {
		p.parsed = first + 1;
		return wrap_user_callback(Object_end, val, p.parsed);
	}
	
	for (;;) {
		// name
		if (*first != '"') return parse_error(p, Expected_key, first);
		
		if (!parseString(p, first+1, last, Object_key)) return parse_error(p, No_value, first);
		
		first = skip_wspace(p.parsed, last);
		if (first == last || *first != ':') return parse_error(p, Expected_colon, first);
		
		// value
		if (!parseJson(p, first+1, last)) return false;
		
		first = skip_wspace(p.parsed, last);
		if (first == last) return parse_error(p, Unbalanced_collection, first);

		if (*first == ',') {
			first = skip_wspace(first+1, last);
			if (first == last) return parse_error(p, Expected_key, first);
		}
		else break;
	}
	
	if (*first == '}') {
		p.parsed = first + 1;
		return wrap_user_callback(Object_end, val, p.parsed);
	}
	
	return parse_error(p, Unbalanced_collection, first);
}


// assumes that '[' was already parsed
static bool parseJsonArray(parser_base_t& p, char * first, char * last) {
	value_t val;
	if (!wrap_user_callback(Array_begin, val, first)) return false;

	first = skip_wspace(first, last);
	if (first == last) return parse_error(p, Unbalanced_collection, first);
	
	if (*first == ']') {
		p.parsed = first + 1;
		return wrap_user_callback(Array_end, val, p.parsed);
	}
	
	for (;;) {
		// value
		if (!parseJson(p, first, last)) return false;
		 
		first = skip_wspace(p.parsed, last);
		if (first == last) return parse_error(p, Unbalanced_collection, first);

		if (*first == ',') {
			first = skip_wspace(first+1, last);
			if (first == last) return parse_error(p, No_value, first);
		}
		else break;
	}
	
	if (*first == ']') {
		p.parsed = first + 1;
		return wrap_user_callback(Array_end, val, p.parsed);
	}
	
	return parse_error(p, Unbalanced_collection, first);
}


