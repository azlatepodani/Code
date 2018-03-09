#include <stdlib.h>
#include <locale.h>
#include <cmath>
#include <climits>
#include <string.h>
#include "azp_json.h"
#include <memory>


namespace azp {

	
static bool parseJson(parser_base_t& p, char * first, char * last);
static bool parse_error(parser_base_t& p, ParserErrors err, const char * curPtr);

	
bool parseJson(parser_t& p, char * first, char * last) {
	p._first = first;
	
	auto old = setlocale(LC_NUMERIC, "C");
	if (!old) {
		return parse_error(p, Runtime_error, first);
	}
	
	bool result = parseJson(static_cast<parser_base_t&>(p), first, last);
	
	if (!setlocale(LC_NUMERIC, old) && result) {	// set the error type only if result == true
		return parse_error(p, Runtime_error, first);
	}
	
	if (result) {
		p.parsed_offset = p.parsed - first;
	}
	else {
		p.parsed = nullptr;
	}
	
	return result;
}


bool parseJson(parser_t& p, const char * first, const char * last)
{
	auto size = last - first;
	std::unique_ptr<char> buf(new (std::nothrow) char[size]);
	
	if (!buf) return parse_error(p, Runtime_error, nullptr);
	
	memcpy(buf.get(), first, size);
	
	auto result = parseJson(p, buf.get(), buf.get()+size);
	
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
static bool parseJsonObject(parser_base_t& p, char * first, char * last);
static bool parseJsonArray(parser_base_t& p, char * first, char * last);
static bool parseJsonScalarV(parser_base_t& p, char * first, char * last);


struct dec_on_exit {
	// cppcheck-suppress noExplicitConstructor
	dec_on_exit(int32_t& val) : val(val) {}
	~dec_on_exit() { --val; }
	int32_t& val;
};


// parses a JSON value
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


// assumes that '\u' was already parsed
static bool unescapeUnicodeChar(parser_base_t& p, char * first, char * last, char ** pCur) {
	if (last-first < 4) return parse_error(p, Invalid_escape, first);
	
	uint32_t u32 = 0;
	
	//
#define load_hex() 								\
	if (isDigit(*first)) {						\
		u32 |= *first - '0';					\
	}											\
	else if (isHexAlpha(*first)) {	            \
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
		if (u32 >= 0xDC00) {	// incorrect bit pattern
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

		if (u32 >= 0xDC00 && u32 < 0xE000) {  // second word
			u32 = (u32 & 0x3FF) + ((saved & 0x3FF) << 10) + 0x10000; // convert to code point
		}
		else return parse_error(p, Invalid_escape, first-4);
	}
	
	// write as UTF-8
	// note: we don't need to test if 'cur' reaches the end of the buffer, because
	// the unescape operation always writes less characters than it parses. E.g.
	// \uFFFF -> ef bf bf
	// \uDBFF\uDFFF -> f4 8f bf bf
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
	val.string.p = start;
	val.string.len = end-start;
	return wrap_user_callback(report_type, val, start);
}


// assumes that '"' was already parsed
static bool parseString(parser_base_t& p, char * first, char * last, ParserTypes report_type)
{
	auto start = first;
	auto cur = first;
	char c;
	
	// Don't copy unless necessary -> find first escape
	for (;first != last; ++first) {
		c = *first;
		// end of string
		if (c == '"') {
			p.parsed = first+1;
			return call_string_callback(p, start, first, report_type);
		}
		
		if (c == '\\') {
			cur = first;
			goto _escape_seq;
		}
		
		// these values should have been escaped
		if ((uint8_t)c < 0x20) return parse_error(p, Invalid_char, first);
	}
	
	for (;first != last; ++first) {
		c = *first;
		// end of string
		if (c == '"') {
			p.parsed = first+1;
			return call_string_callback(p, start, cur, report_type);
		}
		
		// copy normal character
		if (c != '\\') {
			// these values should have been escaped
			if ((uint8_t)c < 0x20) return parse_error(p, Invalid_char, first);
			
			*cur++ = c;
			continue;
		}

_escape_seq:
		// process escape sequence
		first++;
		if (first == last) return parse_error(p, No_string_end, start-1);
		
		c = *first;
		if (c != 'u') {
			if ((c == '"') | (c == '\\') | (c == '/')) {
				*cur = c;
			}
			else if (c == 'n') {
				*cur = '\n';
			}
			else if (c == 'r') {
				*cur = '\r';
			}
			else if (c == 't') {
				*cur = '\t';
			}
			else if (c == 'b') {
				*cur = '\b';
			}
			else if (c == 'f') {
				*cur = '\f';
			}
			else {
				return parse_error(p, Invalid_escape, first);
			}
			
			cur++;
		}
		else {
			if (!unescapeUnicodeChar(p, first+1, last, &cur)) {
				return false;
			}
			
			first = p.parsed - 1; // - 1 is needed for the increment of the for loop
		}
	}
	
	return parse_error(p, No_string_end, start-1);
}


// assumes last == first + max_len + 1 (see parseNumber)
// 'max_len' is the longest string for floating numbers accepted
static bool parseNumberNoCopy(parser_base_t& p, char * first, char * last) {
	char * cur = first;
	char * savedFirst = first;
		
	auto savedCh = *(last-1);
	*(last-1) = 0; // sentinel
	
	// optional '-' sign
	if (*first == '-') {
		cur++;
		first++;
	}

	bool haveDigit = false;
	bool haveDot = false;
	bool haveExp = false;
	bool haveDotDigit = false;
	bool haveExpDigit = false;

	if (*first == '0') {	// no leading zeros allowed
		cur++;
		first++;	// valid input: 0, 0.x, 0ex
		haveDigit = true;
	}
	else {
		// digits
		while (isDigit(*first)) {
			cur++;
			first++;
			haveDigit = true;
		}
	}
	
	// optional '.<digits>'
	if (*first == '.') {
		haveDot = true;
		cur++;
		first++;
		
		// optional digits after the dot
		while (isDigit(*first)) {
			cur++;
			first++;
			haveDotDigit = true;
		}
	}
	
	// optional 'e[sign]<digits>'
	if ((*first|0x20) == 'e') {
		haveExp = true;
		
		cur++;
		first++;

		// optional +/- signs
		if (*first == '-' || *first == '+') {
			cur++;
			first++;
		}
	
		// digits after the exponent
		while (isDigit(*first)) {
			cur++;
			first++;
			haveExpDigit = true;
		}
	}
	
	*(last-1) = savedCh;	// replace the sentinel with the saved character

	if (haveDot && !haveExp && !haveDotDigit ||
		haveExp && !haveExpDigit ||
		!haveDigit)
	{
		return parse_error(p, Invalid_number, savedFirst);
	}

	savedCh = *cur;
	*cur = 0;
	
	bool result;
	if (haveDot | haveExp) {		
		value_t val;
		val.number = strtod(savedFirst, nullptr);
		if (val.number == HUGE_VAL) {
			return parse_error(p, Invalid_number, savedFirst);
		}
		
		result = wrap_user_callback(Number_float, val, first);
	}
	else {
		value_t val;
		val.integer = strtoll(savedFirst, nullptr, 10);
		if ((val.integer == LLONG_MAX || val.integer ==  LLONG_MIN) && (errno == ERANGE)) {
			return parse_error(p, Invalid_number, savedFirst);
		}
		
		result = wrap_user_callback(Number_int, val, first);
	}
	
	*cur = savedCh;
	
	p.parsed = first;
	return result;
}


// assumes first != last
static bool parseNumber(parser_base_t& p, char * first, char * last) {
	constexpr size_t max_len = 24; // longest valid long long string "-9223372036854775807" (19+1)
	                               // longest valid double string "-1.1111111111111112e+300" (24)
	
	if (last - first > max_len) {
		// adjust last so that we don't attempt to parse more than the buffer size
		return parseNumberNoCopy(p, first, first+max_len+1);
	}

	char buf[32];
	size_t cur = 0;
	char * savedFirst = first;
	
	auto savedCh = *(last-1);
	*(last-1) = 0; // sentinel
	
	// optional '-' sign
	if (*first == '-') buf[cur++] = *first++;

	bool haveDigit = false;
	bool leadingZero = false;
	bool haveDot = false;
	bool haveExp = false;
	bool haveDotDigit = false;
	bool haveExpDigit = false;

	if (*first == '0') {	// no leading zeros allowed
		buf[cur++] = *first++;	// valid input: 0, 0.x, 0ex
		haveDigit = true;
		leadingZero = true;
	}
	else {
		// digits
		while (isDigit(*first)) {
			buf[cur++] = *first++;
			haveDigit = true;
		}
	}
	
	// optional '.<digits>'
	if (*first == '.') {
		haveDot = true;
		leadingZero = false;
		buf[cur++] = *first++;
		
		// optional digits after the dot
		while (isDigit(*first)) {
			buf[cur++] = *first++;
			haveDotDigit = true;
		}
	}
	
	// optional 'e[sign]<digits>'
	if ((*first|0x20) == 'e') {
		haveExp = true;
		leadingZero = false;
		
		buf[cur++] = *first++;

		// optional +/- signs
		if (*first == '-' || *first == '+') buf[cur++] = *first++;
	
		// digits after the exponent
		while (isDigit(*first)) {
			buf[cur++] = *first++;
			haveExpDigit = true;
		}
	}
	
	if (first == (last-1) && !leadingZero && isDigit(savedCh)) {
		buf[cur++] = savedCh;
		first++;
		if (haveExp) haveExpDigit = true;
		else if (haveDot) haveDotDigit = true;
		else haveDigit = true;
	}
	
	buf[cur] = 0;
	*(last-1) = savedCh;	// replace the sentinel with the saved character
	
	if (haveDot && !haveExp && !haveDotDigit ||
		haveExp && !haveExpDigit ||
		!haveDigit)
	{
		return parse_error(p, Invalid_number, savedFirst);
	}
	
	bool result;
	if (haveDot | haveExp) {		
		value_t val;
		val.number = strtod(buf, nullptr);
		if (val.number == HUGE_VAL) {
			return parse_error(p, Invalid_number, savedFirst);
		}
		
		result = wrap_user_callback(Number_float, val, first);
	}
	else {
		value_t val;
		val.integer = strtoll(buf, nullptr, 10);
		if ((val.integer == LLONG_MAX || val.integer ==  LLONG_MIN) && (errno == ERANGE)) {
			return parse_error(p, Invalid_number, savedFirst);
		}
		
		result = wrap_user_callback(Number_int, val, first);
	}
	
	p.parsed = first;
	return result;
}


// assumes 't' was already parsed
static bool parseTrue(parser_base_t& p, char * first, char * last) {
	uint32_t v;
	
	if (last-first < 4) {
		if (last-first < 3) {
			return parse_error(p, Invalid_token, first-1);
		}
		
		v = *first;
		v |= uint32_t(first[1]) << 8;
		v |= uint32_t(first[2]) << 16;
	}
	else {
		v = *(uint32_t *)first & 0xFFFFFF;
	}
	
	if (v != '\0eur') return parse_error(p, Invalid_token, first-1);
	
	p.parsed = first + 3;
	value_t val;
	return wrap_user_callback(Bool_true, val, p.parsed);
}


// assumes 'f' was already parsed
static bool parseFalse(parser_base_t& p, char * first, char * last) {
	if (last-first < 4) return parse_error(p, Invalid_token, first-1);
	
	uint32_t v = *(uint32_t *)first;
	
	if (v != 'esla') return parse_error(p, Invalid_token, first-1);
	
	p.parsed = first + 4;
	value_t val;
	return wrap_user_callback(Bool_false, val, p.parsed);
}


// assumes 'n' was already parsed
static bool parseNull(parser_base_t& p, char * first, char * last) {
	uint32_t v;
	
	if (last-first < 4) {
		if (last-first < 3) {
			return parse_error(p, Invalid_token, first-1);
		}
		
		v = *first;
		v |= uint32_t(first[1]) << 8;
		v |= uint32_t(first[2]) << 16;
	}
	else {
		v = *(uint32_t *)first & 0xFFFFFF;
	}
	
	if (v != '\0llu') return parse_error(p, Invalid_token, first-1);
	
	p.parsed = first + 3;
	value_t val;
	return wrap_user_callback(Null_val, val, p.parsed);
}


static bool parseJsonScalarV(parser_base_t& p, char * first, char * last) {
	if (*first == '"') {
		return parseString(p, first+1, last, String_val);
	}

	if (isDigit(*first) | (*first == '-')) {
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


} // namespace azp

