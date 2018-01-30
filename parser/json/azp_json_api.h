#pragma once

#include <string>
#include <assert.h>
#include "azp_vector.h"

#ifndef _NOEXCEPT
#define _NOEXCEPT noexcept
#endif

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
		char        buf[sizeof(JsonString)];
		long long  	number;
		double      float_num;
		string_view_t view;
	} u;
	
	explicit JsonValue() : type(EMPTY) { }
	
	JsonValue(const JsonValue& other) : type(EMPTY) {
		operator=(other);
	}
	
	JsonValue(JsonValue&& other) _NOEXCEPT
		: type(EMPTY)
	{
		operator=(std::move(other));
	}
	
	explicit JsonValue(std::nullptr_t) : type(EMPTY) { }
	
	explicit JsonValue(JsonObject obj)
		: type(OBJECT)
	{
		new (u.buf) JsonObject(std::move(obj));
	}
	
	explicit JsonValue(JsonArray arr)
		: type(ARRAY)
	{
		new (u.buf) JsonArray(std::move(arr));
	}
	
	explicit JsonValue(long long num)
		: type(NUMBER)
	{
		new (&u.number) long long(num);
	}
	
	explicit JsonValue(double num)
		: type(FLOAT_NUM)
	{
		new (&u.float_num) double(num);
	}
	
	void _initString(JsonString&& str) {
		new (u.buf) JsonString(std::move(str));
	}
	
	explicit JsonValue(JsonString str)
		: type(STRING)
	{
		_initString(std::move(str));
	}
	
	explicit JsonValue(const char* str)
		: type(STRING)
	{
		_initString(JsonString(str));
	}
	
	explicit JsonValue(string_view_t str)
		: type(STRING_VIEW)
	{
		u.view = str;
	}
	
	explicit JsonValue(bool val) : type(val ? BOOL_TRUE : BOOL_FALSE) { }
	
	JsonValue& operator=(const JsonValue& other);
	
	JsonValue& operator=(JsonValue&& other) _NOEXCEPT;
	
	const JsonObject& object() const _NOEXCEPT {
		assert(type == OBJECT);
		return *reinterpret_cast<const JsonObject *>(u.buf);
	}
	
	JsonObject& object() _NOEXCEPT {
		assert(type == OBJECT);
		return *reinterpret_cast<JsonObject *>(u.buf);
	}
	
	const JsonArray& array() const _NOEXCEPT {
		assert(type == ARRAY);
		return *reinterpret_cast<const JsonArray *>(u.buf);
	}
	
	JsonArray& array() _NOEXCEPT {
		assert(type == ARRAY);
		return *reinterpret_cast<JsonArray *>(u.buf);
	}
	
	const JsonString& string() const _NOEXCEPT {
		assert(type == STRING);
		return *reinterpret_cast<const JsonString *>(u.buf);
	}
	
	JsonString& string() _NOEXCEPT {
		assert(type == STRING);
		return *reinterpret_cast<JsonString *>(u.buf);
	}

    ~JsonValue();
};

//
// Guarantees that any double will be written so that parsing the resulting JSON will return an identical value
// Precondition: the double value is not any of the NANs and INFs
//
void json_writer(std::string& stm, const JsonValue& val);

//
// The returned string is the memory backing for all the string values in the json value.
// Copying the json value will decouple the new object from the original string.
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
	
	JsonObjectField(JsonObjectField&& other) _NOEXCEPT = default;
	JsonObjectField& operator=(const JsonObjectField& other) = default;
	JsonObjectField& operator=(JsonObjectField&& other) _NOEXCEPT = default;
};


} // namespace asu


