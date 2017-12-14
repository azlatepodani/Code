#pragma once

#include <string>
#include <vector>
#include <assert.h>


namespace asu {

struct JsonObjectField;
struct JsonValue;

typedef std::vector<JsonObjectField>  JsonObject;
typedef std::vector<JsonValue>  JsonArray;
typedef std::string  JsonString;

static_assert(sizeof(JsonString) >= sizeof(JsonObject), "check JsonValue::Impl::buf size");
static_assert(sizeof(JsonString) >= sizeof(JsonArray), "check JsonValue::Impl::buf size");


struct JsonValue {
	enum TYPES {
		OBJECT,
		ARRAY,
		STRING,
		NUMBER,
		FLOAT_NUM,
		BOOL_TRUE,
		BOOL_FALSE,
		EMPTY
	} 				type;
	
	union Impl {
		//
		// VS2012 doesn't allow unions with fields that have copy constructors
		//
		//JsonObject  object;
		//JsonArray   array;
		//std::string string;
		char        buf[sizeof(JsonString)];
		long long  	number;
		double      float_num;
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
	
	explicit JsonValue(const char * str)
		: type(STRING)
	{
		_initString(JsonString(str));
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


void json_writer(std::ostream& stm, const JsonValue& val);
void json_reader(std::istream& stm, JsonValue& val);
void json_reader(const std::vector<char>& stm, JsonValue & val);

JsonValue& json_get_child(JsonValue& val, const std::string& path);
JsonValue& json_get_child(JsonValue& val, const std::string& path, JsonValue& defVal);

inline const JsonValue& json_get_child(const JsonValue& val, const std::string& path) {
    return json_get_child(const_cast<JsonValue&>(val), path);
}

inline const JsonValue& json_get_child(const JsonValue& val, const std::string& path, const JsonValue& defVal) {
    return json_get_child(const_cast<JsonValue&>(val), path, const_cast<JsonValue&>(defVal));
}

JsonValue& json_put_child(JsonValue& val, const std::string& path, JsonValue child);

JsonValue json_remove_child(JsonValue& val, const std::string& path);


struct JsonObjectField {
	JsonString  name;
	JsonValue  value;
	
	JsonObjectField(JsonString name_, JsonValue value_)
		: name(name_), value(value_) { }
	
	JsonObjectField(const JsonObjectField& other)
		: name(other.name)
		, value(other.value)
	{ }
	
	JsonObjectField(JsonObjectField&& other) _NOEXCEPT
		: name(std::move(other.name))
		, value(std::move(other.value))
	{ }
	
	JsonObjectField& operator=(const JsonObjectField& other) {
		name = other.name;
		value = other.value;
		return  *this;
	}
	
	JsonObjectField& operator=(JsonObjectField&& other) _NOEXCEPT {
		name = std::move(other.name);
		value = std::move(other.value);
		return  *this;
	}
};


bool operator==(const JsonValue& left, const JsonValue& right);
bool operator==(const JsonObjectField& left, const JsonObjectField& right);


inline bool operator!=(const JsonValue& left, const JsonValue& right) {
	return !(left == right);
}


inline bool operator!=(const JsonObjectField& left, const JsonObjectField& right) {
	return !(left == right);
}


long long json_to_number(const JsonValue& val);
bool json_to_bool(const JsonValue& val);
std::string json_to_string(const JsonValue& val);
double json_to_float(const JsonValue& val);



} // namespace asu


