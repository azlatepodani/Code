#pragma once

#include <string>
#include <assert.h>
#include "azp_vector.h"


namespace azp {

struct JsonObjectField;
struct JsonValue;

using alloc_t = default_alloc_t;

typedef vector<JsonObjectField, alloc_t>  JsonObject;
typedef vector<JsonValue, alloc_t>  JsonArray;
typedef std::string  JsonString;



struct string_view_t {
	const char* str;	// not necessarily 0-terminated
	size_t len;
};


//
// Models the JSON data. Minimal implementation.
//
struct JsonValue {
	enum ValueTypes {
		Empty,
		Object,
		Array,
		String,
		String_view,
		Number,
		Float_num,
		Bool_true,
		Bool_false,
		Max_types
	} type;
	
	union Impl {
		JsonObject    object;
		JsonArray     array;
		JsonString    string;
		int64_t  	  number;
		double        float_num;
		string_view_t view;
		
		Impl() { }
		~Impl() { }
	} u;
	
	JsonValue() noexcept;
	
	JsonValue(const JsonValue& other);
	JsonValue(JsonValue&& other) noexcept;
	
	explicit JsonValue(std::nullptr_t) noexcept;
	explicit JsonValue(JsonObject obj) noexcept;
	explicit JsonValue(JsonArray arr) noexcept;
	explicit JsonValue(int64_t num) noexcept;
	explicit JsonValue(double num) noexcept;
	explicit JsonValue(JsonString str) noexcept;
	explicit JsonValue(const char* str);
	explicit JsonValue(const string_view_t& str) noexcept;
	explicit JsonValue(bool val) noexcept;
	
	JsonValue& operator=(const JsonValue& other);
	JsonValue& operator=(JsonValue&& other) noexcept;
	
    ~JsonValue() noexcept;
	
protected:
	void _initString(JsonString&& str) noexcept;
};


//
// Serializes the JsonValue to a ECMA-404 'The JSON Data Interchange Standard' string.
//
// Preconditions:
// - the strings passed to JsonValues are encoded UTF-8
// - double values are not any of the NANs and INFs
//
void json_writer(std::string& stm, const JsonValue& val);

//
// Converts a conforming JSON string to the corresponding tree.
// Note: The returned string (pair::second) is the memory backing for all the string values in the JSON value.
//       Copying the JSON value will decouple the new object from the original string.
//
// Preconditions:
// - @see azp::parseJson
//
// Throws std::exception in case of error.
//
std::pair<JsonValue, std::string> json_reader(const std::string& stm);

//
// Sorts the JSON objects' members by key for improved search times.
//
void optimize_for_search(JsonValue& root) noexcept;



struct JsonObjectField {
	enum NameTypes {
		String,
		String_view,
		Max_types
	};
	
	union Impl {
		JsonString    s;
		string_view_t v;
		
		Impl() { }
		~Impl() { }
	};
	
	NameTypes  type;
	Impl  name;
	JsonValue  value;
	
	JsonObjectField() noexcept;
	~JsonObjectField() noexcept;
	
	// cppcheck-suppress passedByValue
	JsonObjectField(JsonString name_, JsonValue value_) noexcept;
	// cppcheck-suppress passedByValue
	JsonObjectField(const string_view_t& name_, JsonValue value_) noexcept;
	// cppcheck-suppress passedByValue
	JsonObjectField(const char* name_, JsonValue value_);
	
	JsonObjectField(const JsonObjectField& other);
	JsonObjectField(JsonObjectField&& other) noexcept;
	JsonObjectField& operator=(const JsonObjectField& other);
	JsonObjectField& operator=(JsonObjectField&& other) noexcept;
	
	void _initString(JsonString&& str) noexcept;
	
	size_t nameSize() const noexcept;
	const char* nameStr() const noexcept;
};



//
// Inline implementation
//

inline JsonValue::JsonValue() noexcept : type(Empty) { }

inline JsonValue::JsonValue(const JsonValue& other) : type(Empty) {
	operator=(other);
}

inline JsonValue::JsonValue(JsonValue&& other) noexcept
	: type(Empty)
{
	operator=(std::move(other));
}

inline JsonValue::JsonValue(std::nullptr_t) noexcept : type(Empty) { }

inline JsonValue::JsonValue(JsonObject obj) noexcept
	: type(Object)
{
	new (&u.object) JsonObject(std::move(obj));
}

inline JsonValue::JsonValue(JsonArray arr) noexcept
	: type(Array)
{
	new (&u.array) JsonArray(std::move(arr));
}

inline JsonValue::JsonValue(int64_t num) noexcept
	: type(Number)
{
	new (&u.number) int64_t(num);
}

inline JsonValue::JsonValue(double num) noexcept
	: type(Float_num)
{
	new (&u.float_num) double(num);
}

inline void JsonValue::_initString(JsonString&& str) noexcept {
	new (&u.string) JsonString(std::move(str));
}

inline JsonValue::JsonValue(JsonString str) noexcept
	: type(String)
{
	_initString(std::move(str));
}

inline JsonValue::JsonValue(const char* str)
	: type(String)
{
	_initString(JsonString(str));
}

inline JsonValue::JsonValue(const string_view_t& str) noexcept
	: type(String_view)
{
	u.view = str;
}

inline JsonValue::JsonValue(bool val) noexcept : type(val ? Bool_true : Bool_false) { }

//--------------------------

inline void JsonObjectField::_initString(JsonString&& str) noexcept {
	new (&name.s) JsonString(std::move(str));
}

inline JsonObjectField::JsonObjectField() noexcept
	: type(String)
{
	_initString(JsonString());
}

inline JsonObjectField::JsonObjectField(JsonString name_, JsonValue value_) noexcept
	: type(String)
	, value(std::move(value_))
{
	_initString(std::move(name_));
}

inline JsonObjectField::JsonObjectField(const string_view_t& name_, JsonValue value_) noexcept
	: type(String_view)
	, value(std::move(value_))
{
	name.v = name_;
}

inline JsonObjectField::JsonObjectField(const char* name_, JsonValue value_)
	: type(String)
	, value(std::move(value_))
{
	_initString(JsonString(name_));
}

inline JsonObjectField::~JsonObjectField() noexcept {
    if (type == String) {
		name.s.~JsonString();
	}
}

inline JsonObjectField& JsonObjectField::operator=(const JsonObjectField& other) {
	JsonObjectField tmp(other);
	std::swap(*this, tmp);
	return *this;
}

inline size_t JsonObjectField::nameSize() const noexcept {
	return (type == String) ? name.s.size() : name.v.len;
}

inline const char* JsonObjectField::nameStr() const noexcept {
	return (type == String) ? name.s.c_str() : name.v.str;
}



} // namespace asu


