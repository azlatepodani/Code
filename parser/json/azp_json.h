#pragma once

namespace azp {
	
	
enum ParserTypes {
	Object_begin,
	Object_end,
	Array_begin,
	Array_end,
	Object_key,
	Number_int,
	Number_float,
	String_val,
	Bool_true,
	Bool_false,
	Null_val,
	Max_types,
};


enum ParserErrors {
	No_error,
	Max_recursion,			// the configured maximum nesting level was exceeded
	No_value,				// the parser couldn't parse a JSON value at the error position
	Invalid_escape,			// a '\' character wasn't followed by a valid sequence
	User_requested,			// the callback function returned 'false'
	No_string_end,			// the closing '"' were not present in the buffer
	Invalid_number,			// a numeric value couldn't be parsed
	Runtime_error,			// an external API returned an error
	Invalid_token,			// the parser encountered 't', 'f', 'n', but couldn't parse 'true', 'false' or 'null' 
	Unbalanced_collection,	// the '[', ']', '{', '}' weren't paired correctly
	Expected_key,			// inside an object, at the error position, a key was expected
	Expected_colon,			// inside an object, at the error position, a ':' character was expected
	Max_errors,
};


union value_t {
	long long integer;
	double number;
	struct {
		const char* string;	// this pointer is not valid after the callback returns
		size_t length;
	};
};

//
// Progress callback.
// Return 'true' to continue parsing and 'false' to abort.
// @see parser_t::set_callback()
//
typedef bool (* parser_callback_t)(void * context, ParserTypes type, const value_t& val);


//
// Internal structure: holds the parser data
// Some fields are accessible via parser_t
//
struct parser_base_t {
	char * parsed;			// position after the last character parsed
	void * context;			// callback context
	parser_callback_t callback;	// user callback
	int recursion;			// current nesting level
	int max_recursion;		// maximum nesting level
	ParserErrors error;		// error hint
	size_t err_position;	// error position
	const char * _first;	// saved pointer to buffer start
};



class parser_t : parser_base_t {

public:
	parser_t();

	void set_callback(parser_callback_t cb, void* ctx) {
		callback = cb;
		context = ctx;
	}

	// maxr should be > 0.
	void set_max_recursion(int maxr) {
		max_recursion = maxr;
	}
	
	ParserErrors get_error() { return error; }
	
	size_t get_err_position() { return err_position; }
	
	// Gets the position of the first character following the parsed value.
	// This function can be used to parse non-conforming or hybrid data.
	char * get_parsed() { return parsed; }

	friend bool parseJson(parser_t& p, char * first, char * last);
	friend bool parseJson(parser_t& p, const char * first, const char * last);
};


//
// Parses a string of chars according to the ECMA-404 The JSON Data Interchange Standard
//
// Preconditions: the string is UTF-8 encoded.
//
// Returns true if the string is conform, the maximum recursion depth wasn't reached and
// the parser's callback didn't return 'false' in any of the invocations.
//
// [first, last) is a semi-open interval
// The buffer will be modified by the function and the result is no longer valid JSON data.
//
// The function will throw only if the user defined callback will throw
//
bool parseJson(parser_t& p, char * first, char * last);

//
// Helper function: it will make a copy of the buffer and call the function above.
//
bool parseJson(parser_t& p, const char * first, const char * last);

} // namespace azp

