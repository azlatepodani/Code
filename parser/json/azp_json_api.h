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

static_assert(sizeof(JsonString) >= sizeof(JsonObject), "check JsonValue::Impl::buf size");
static_assert(sizeof(JsonString) >= sizeof(JsonArray), "check JsonValue::Impl::buf size");


struct string_view_t {
	const char* str;	// not necessarily 0-terminated
	size_t len;
};

//
// Models the JSON data. Minimal implementation.
//
struct JsonValue {
	enum TYPES {
		OBJECT,
		ARRAY,
		STRING,
		STRING_VIEW,
		NUMBER,
		FLOAT_NUM,
		BOOL_TRUE,
		BOOL_FALSE,
		EMPTY
	} type;
	
	union Impl {
		char          buf[sizeof(JsonString)];
		long long  	  number;
		double        float_num;
		string_view_t view;
	} u;
	
	JsonValue() noexcept;
	
	JsonValue(const JsonValue& other);
	JsonValue(JsonValue&& other) noexcept;
	
	explicit JsonValue(std::nullptr_t) noexcept;
	explicit JsonValue(JsonObject obj);
	explicit JsonValue(JsonArray arr);
	explicit JsonValue(long long num) noexcept;
	explicit JsonValue(double num) noexcept;
	explicit JsonValue(JsonString str) noexcept;
	explicit JsonValue(const char* str);
	explicit JsonValue(string_view_t str) noexcept;
	explicit JsonValue(bool val) noexcept;
	
	JsonValue& operator=(const JsonValue& other);
	JsonValue& operator=(JsonValue&& other) noexcept;
	
	const JsonObject& object() const noexcept;
		  JsonObject& object() noexcept;
	
	const JsonArray& array() const noexcept;
		  JsonArray& array() noexcept;
	
	const JsonString& string() const noexcept;
		  JsonString& string() noexcept;

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

inline JsonValue::JsonValue() noexcept : type(EMPTY) { }

inline JsonValue::JsonValue(const JsonValue& other) : type(EMPTY) {
	operator=(other);
}

inline JsonValue::JsonValue(JsonValue&& other) noexcept
	: type(EMPTY)
{
	operator=(std::move(other));
}

inline JsonValue::JsonValue(std::nullptr_t) noexcept : type(EMPTY) { }

inline JsonValue::JsonValue(JsonObject obj)
	: type(OBJECT)
{
	new (u.buf) JsonObject(std::move(obj));
}

inline JsonValue::JsonValue(JsonArray arr)
	: type(ARRAY)
{
	new (u.buf) JsonArray(std::move(arr));
}

inline JsonValue::JsonValue(long long num) noexcept
	: type(NUMBER)
{
	new (&u.number) long long(num);
}

inline JsonValue::JsonValue(double num) noexcept
	: type(FLOAT_NUM)
{
	new (&u.float_num) double(num);
}

inline void JsonValue::_initString(JsonString&& str) noexcept {
	new (u.buf) JsonString(std::move(str));
}

inline JsonValue::JsonValue(JsonString str) noexcept
	: type(STRING)
{
	_initString(std::move(str));
}

inline JsonValue::JsonValue(const char* str)
	: type(STRING)
{
	_initString(JsonString(str));
}

inline JsonValue::JsonValue(string_view_t str) noexcept
	: type(STRING_VIEW)
{
	u.view = str;
}

inline JsonValue::JsonValue(bool val) noexcept : type(val ? BOOL_TRUE : BOOL_FALSE) { }

inline const JsonObject& JsonValue::object() const noexcept {
	assert(type == OBJECT);
	return *reinterpret_cast<const JsonObject *>(u.buf);
}

inline JsonObject& JsonValue::object() noexcept {
	assert(type == OBJECT);
	return *reinterpret_cast<JsonObject *>(u.buf);
}

inline const JsonArray& JsonValue::array() const noexcept {
	assert(type == ARRAY);
	return *reinterpret_cast<const JsonArray *>(u.buf);
}

inline JsonArray& JsonValue::array() noexcept {
	assert(type == ARRAY);
	return *reinterpret_cast<JsonArray *>(u.buf);
}

inline const JsonString& JsonValue::string() const noexcept {
	assert(type == STRING);
	return *reinterpret_cast<const JsonString *>(u.buf);
}

inline JsonString& JsonValue::string() noexcept {
	assert(type == STRING);
	return *reinterpret_cast<JsonString *>(u.buf);
}



} // namespace asu


