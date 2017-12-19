#pragma once


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
	Undefined,
	Max_types,
};


enum ParserErrors {
	No_error,
	Max_recursion,
	No_value,
	Invalid_escape,
	User_requested,
	No_string_end,
	Invalid_number,
	Runtime_error,
	Invalid_token,
	Unbalanced_collection,
	Expected_key,
	Expected_colon,
};


union value_t {
	long long integer;
	double number;
	struct {
		const char* string;
		size_t length;
	};
};


typedef bool (* parser_callback_t)(void * context, ParserTypes type, const value_t& val);



struct parser_base_t {
	char * parsed;
	void * context;
	parser_callback_t callback;
	int recursion;
	int max_recursion;
	ParserErrors error;
	size_t err_position;
	const char * _first;
};



class parser_t : parser_base_t {

public:
	parser_t();

	void set_callback(parser_callback_t cb, void* ctx) {
		callback = cb;
		context = ctx;
	}

	void set_max_recursion(int maxr) {
		max_recursion = maxr;
	}
	
	ParserErrors get_error() { return error; }
	
	size_t get_err_position() { return err_position; }

	friend bool parseJson(parser_t& p, char * first, char * last);
};


//
// Parses a string of chars according to the ECMA-404 The JSON Data Interchange Standard
//
// Preconditions: the string is UTF-8 encoded.
//
// Returns true if the string is conform, the maximum recursion depth wasn't reached
// the parser's callback didn't return 'false' in any of the invocations.
//
// [first, last) is a semi-open interval
//
bool parseJson(parser_t& p, char * first, char * last);


