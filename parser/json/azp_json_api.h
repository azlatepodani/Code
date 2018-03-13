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
	explicit JsonValue(JsonObject obj);
	explicit JsonValue(JsonArray arr);
	explicit JsonValue(int64_t num) noexcept;
	explicit JsonValue(double num) noexcept;
	explicit JsonValue(JsonString str) noexcept;
	explicit JsonValue(const char* str);
	explicit JsonValue(string_view_t str) noexcept;
	explicit JsonValue(bool val) noexcept;
	
	JsonValue& operator=(const JsonValue& other);
	JsonValue& operator=(JsonValue&& other) noexcept;
	
    ~JsonValue();
	
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
std::pair<JsonValue, std::string> json_reader(const std::string& stm);


struct JsonObjectField {
	JsonString  name;
	JsonValue  value;
	
	JsonObjectField() = default;
	
	// cppcheck-suppress passedByValue
	JsonObjectField(JsonString name_, JsonValue value_)
		: name(std::move(name_)), value(std::move(value_)) { }
	
	JsonObjectField(const JsonObjectField& other)
		: name(other.name)
		, value(other.value)
	{ }
	
	JsonObjectField(JsonObjectField&& other) noexcept = default;
	JsonObjectField& operator=(const JsonObjectField& other) = default;
	
	JsonObjectField& operator=(JsonObjectField&& other) noexcept { // = default;	// clang bug
		name = std::move(other.name);
		value = std::move(other.value);
		return *this;
	}
};

//
// Implementation
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

inline JsonValue::JsonValue(JsonObject obj)
	: type(Object)
{
	new (&u.object) JsonObject(std::move(obj));
}

inline JsonValue::JsonValue(JsonArray arr)
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

inline JsonValue::JsonValue(string_view_t str) noexcept
	: type(String_view)
{
	u.view = str;
}

inline JsonValue::JsonValue(bool val) noexcept : type(val ? Bool_true : Bool_false) { }


} // namespace asu


