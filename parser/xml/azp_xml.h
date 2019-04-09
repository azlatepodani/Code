#pragma once

#include <cstdint>


namespace azp {
	
	
class parser_t;


//
// Parses a string of chars according to the ECMA-404 'The JSON Data Interchange Standard'
//
// Preconditions: the string is UTF-8 encoded. The parser's context 'p' is freshly initialized (do not reuse)
//
// Returns true if the string is conform, the maximum recursion depth wasn't reached and
// the parser's callback didn't return 'false' in any of the invocations.
//
// [first, last) is a semi-open interval
// The buffer will be modified by the function and the result is no longer valid JSON data.
//
// The function will throw only if the user defined callback will throw
//
bool parseXml(parser_t& p, char * first, char * last);

//
// Helper function: it will make a copy of the buffer and call the function above.
//
bool parseXml(parser_t& p, const char * first, const char * last);


enum ParserTypes {
	Tag_open,
	Tag_close,
	Attribute_name,
	Attribute_value,
	Text,
	Cdata_text,
	Pinstr_name,
	Pinstr_text,
	
	Max_types,
};


enum ParserErrors {
	No_error,
	Unexpected_char,		// the current character cannot be parsed
	Max_recursion,			// the configured maximum nesting level was exceeded
	No_value,				// the parser couldn't find the root node
	Invalid_escape,			// a character reference was outside the permissible range
	User_requested,			// the callback function returned 'false'
	Runtime_error,			// an external API returned an error
	Expected_closing_brace,
	Expected_name,
	Expected_quote,
	Expected_attr_value,
	Unbalanced_collection,	// the closing tag's name didn't match the current node
	Expected_semicolon,		// a reference didn't end with a ';' char
	Expected_cdata,			// the parser expects a CDATA section
	Expected_cdata_end,		// "]]>" missing
	Expected_comment,
	Expected_comment_end,	// "[^-] -->" missing
	Expected_pi_end,		// "?>" missing
	Invalid_pi_name,		// the PI uses the reserved "xml" name
	Expected_version_decl,	// '<?xml' is not followed by 'version="1.x"'
	Expected_encoding,		// 'encoding' is not followed by '="enc"'
	Expected_sddecl,		// 'standalone' is not followed by '="yes|no"'
	
	Max_errors,
};



struct string_view_t {
	const char* str;	// not necessarily 0-terminated
	size_t len;
};

//
// Progress callback.
// Return 'true' to continue parsing and 'false' to abort.
// @see parser_t::set_callback()
//
typedef bool (* parser_callback_t)(void * context, ParserTypes type, const string_view_t& val);


//
// Internal structure: holds the parser data
// Some fields are accessible via parser_t
//
struct parser_base_t {
	char * parsed;			// position after the last character parsed
	void * context;			// callback context
	parser_callback_t callback;	// user callback
	uint32_t recursion;			// current nesting level
	uint32_t max_recursion;		// maximum nesting level
	ParserErrors error;		// error hint
	size_t err_position;	// error position
	const char * _first;	// saved pointer to buffer start
	size_t parsed_offset;	// offset of the first character following the parsed string. (=0 during parsing)
	string_view_t tag;		// name of the current tag
	string_view_t ver;		// xml version
	string_view_t enc;		// encoding
	string_view_t sddecl;	// standalone decl
};



class parser_t : parser_base_t {

public:
	parser_t();

	void set_callback(parser_callback_t cb, void* ctx) {
		callback = cb;
		context = ctx;
	}

	void set_max_recursion(uint32_t maxr) {
		max_recursion = maxr;
	}
	
	uint32_t get_max_recursion() const {
		return max_recursion;
	}
	
	ParserErrors get_error() const { return error; }
	
	size_t get_err_position() const { return err_position; }
	
	// Gets the position of the first character following the parsed value.
	// Returns nullptr if the 'parseJson(parser_t&, const char*, const char*)' function was used.
	// The value is nullptr if the parser failed to process the string.
	char * get_parsed() const { return parsed; }
	
	// Gets the offset of the first character following the parsed value.
	// The value is 0 if the parser failed to process the string.
	size_t get_parsed_offset() const { return parsed_offset; }

	friend bool parseXml(parser_t& p, char * first, char * last);
	friend bool parseXml(parser_t& p, const char * first, const char * last);
};


} // namespace azp

