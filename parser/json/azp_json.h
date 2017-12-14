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


union value_t {
	long long integer;
	double number;
	struct {
		const char* string;
		size_t length;
	};
};


typedef bool (* parser_callback_t)(void * context, enum ParserTypes type, value_t& val);



struct parser_base_t {
	char * parsed;
	enum ParserTypes current_type;
	void * context;
	parser_callback_t callback;
	int recursion;
	int max_recursion;
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

	friend bool parseJson(parser_t& p, char * first, char * last);
};


bool parseJson(parser_t& p, char * first, char * last);


