#pragma once

#include <string>
#include <assert.h>
#include "azp_vector.h"


namespace azp {


using alloc_t = default_alloc_t;


typedef std::pair<string_view_t, string_view_t> XmlAttribute, XmlPInstr;
typedef vector<XmlAttribute, alloc_t>  XmlAttributes;

#define XmlTagSize      64

struct XmlTag;


extern alloc_t __alloc;

struct XmlGenNode {
    enum Types {
        Tag,
        Text,
        Cdata_text,
        Pinstr,
        Max_types
    };
    
    Types type;
    union Impl {
        char tag[XmlTagSize];
        string_view_t str;
        XmlPInstr pi;
        
        Impl() {}
        ~Impl() {}
    } u;
    
    XmlGenNode(XmlTag&& tag);
    XmlGenNode(XmlPInstr&& pi);
    XmlGenNode(string_view_t&& str, bool cdata);
    
    XmlGenNode(XmlGenNode&& node);
    XmlGenNode(const XmlGenNode& node);
    XmlGenNode& operator=(XmlGenNode&& node);
    XmlGenNode& operator=(const XmlGenNode& node);
    
    XmlTag& tag() { return *(XmlTag*)u.tag; }
    const XmlTag& tag() const { return *(XmlTag*)u.tag; }
    XmlPInstr& pi() { return u.pi; }
    const XmlPInstr& pi() const { return u.pi; }
    string_view_t& str() { return u.str; }
    const string_view_t& str() const { return u.str; }
};


struct XmlTag {
    string_view_t name;
    XmlAttributes attributes;
    vector<XmlGenNode, alloc_t> children;
    
    XmlTag()
        : name{0,0}
        , children(__alloc, 4)
        , attributes(__alloc)
    { }
    
    XmlTag(string_view_t&& str)
        : name(str)
        , children(__alloc, 4)
        , attributes(__alloc)
    {}
};


static_assert(XmlTagSize >= sizeof(XmlTag));


struct XmlDocument {
    string_view_t version;
    string_view_t encoding;
    string_view_t standalone;
    vector<XmlGenNode, alloc_t> misc;   // only PIs
    XmlTag root;
    std::string _backing;   // this holds all the strings in the tree
    
    XmlDocument()
        : version{0,0}
        , encoding{0,0}
        , standalone{0,0}
        , misc(__alloc)
    { }
    
    XmlDocument(XmlDocument&&) = default;
    XmlDocument(const XmlDocument&other);
    XmlDocument& operator=(XmlDocument&&) = default;
    XmlDocument& operator=(const XmlDocument&other);
};


// //
// // Serializes the JsonValue to a ECMA-404 'The JSON Data Interchange Standard' string.
// //
// // Preconditions:
// // - the strings passed to JsonValues are encoded UTF-8
// // - double values are not any of the NANs and INFs
// //
// void json_writer(std::string& stm, const JsonValue& val);

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
XmlDocument xml_reader(std::string stm);

// //
// // Sorts the JSON objects' members by key for improved search times.
// //
// void optimize_for_search(JsonValue& root) noexcept;



// struct JsonObjectField {
	// enum NameTypes {
		// String,
		// String_view,
		// Max_types
	// };
	
	// union Impl {
		// JsonString    s;
		// string_view_t v;
		
		// Impl() { }
		// ~Impl() { }
	// };
	
	// NameTypes  type;
	// Impl  name;
	// JsonValue  value;
	
	// JsonObjectField() noexcept;
	// ~JsonObjectField() noexcept;
	
	// // cppcheck-suppress passedByValue
	// JsonObjectField(JsonString name_, JsonValue value_) noexcept;
	// // cppcheck-suppress passedByValue
	// JsonObjectField(const string_view_t& name_, JsonValue value_) noexcept;
	// // cppcheck-suppress passedByValue
	// JsonObjectField(const char* name_, JsonValue value_);
	
	// JsonObjectField(const JsonObjectField& other);
	// JsonObjectField(JsonObjectField&& other) noexcept;
	// JsonObjectField& operator=(const JsonObjectField& other);
	// JsonObjectField& operator=(JsonObjectField&& other) noexcept;
	
	// void _initString(JsonString&& str) noexcept;
	
	// size_t nameSize() const noexcept;
	// const char* nameStr() const noexcept;
// };



// //
// // Inline implementation
// //

// inline JsonValue::JsonValue() noexcept : type(Empty) { }

// inline JsonValue::JsonValue(const JsonValue& other) : type(Empty) {
	// operator=(other);
// }

// inline JsonValue::JsonValue(JsonValue&& other) noexcept
	// : type(other.type)
// {
	// switch (other.type) {
		// case Object:
			// new (&u.object) JsonObject(std::move(other.u.object));
			// break;

		// case Array:
			// new (&u.array) JsonArray(std::move(other.u.array));
			// break;
			
		// case String:
			// _initString(std::move(other.u.string));
			// break;
			
		// case String_view:
			// u.view = other.u.view;
			// break;
			
		// case Number:
			// u.number = other.u.number;
			// break;
			
		// case Float_num:
			// u.float_num = other.u.float_num;
			// break;
			
		// case Bool_true:
		// case Bool_false:
		// default: // case Empty:
		// ;	// nothing to do
	// }
// }

// inline JsonValue::JsonValue(std::nullptr_t) noexcept : type(Empty) { }

// inline JsonValue::JsonValue(JsonObject obj) noexcept
	// : type(Object)
// {
	// new (&u.object) JsonObject(std::move(obj));
// }

// inline JsonValue::JsonValue(JsonArray arr) noexcept
	// : type(Array)
// {
	// new (&u.array) JsonArray(std::move(arr));
// }

// inline JsonValue::JsonValue(int64_t num) noexcept
	// : type(Number)
// {
	// new (&u.number) int64_t(num);
// }

// inline JsonValue::JsonValue(double num) noexcept
	// : type(Float_num)
// {
	// new (&u.float_num) double(num);
// }

// inline void JsonValue::_initString(JsonString&& str) noexcept {
	// new (&u.string) JsonString(std::move(str));
// }

// inline JsonValue::JsonValue(JsonString str) noexcept
	// : type(String)
// {
	// _initString(std::move(str));
// }

// inline JsonValue::JsonValue(const char* str)
	// : type(String)
// {
	// _initString(JsonString(str));
// }

// inline JsonValue::JsonValue(const string_view_t& str) noexcept
	// : type(String_view)
// {
	// u.view = str;
// }

// inline JsonValue::JsonValue(bool val) noexcept : type(val ? Bool_true : Bool_false) { }

// //--------------------------

// inline void JsonObjectField::_initString(JsonString&& str) noexcept {
	// new (&name.s) JsonString(std::move(str));
// }

// inline JsonObjectField::JsonObjectField() noexcept
	// : type(String)
// {
	// _initString(JsonString());
// }

// inline JsonObjectField::JsonObjectField(JsonObjectField&& other) noexcept
	// : type(other.type)
	// , value(std::move(other.value))
// {
	// if (type == String) {
		// _initString(std::move(other.name.s));
	// }
	// else {
		// name.v = other.name.v;
	// }
// }

// inline JsonObjectField::JsonObjectField(JsonString name_, JsonValue value_) noexcept
	// : type(String)
	// , value(std::move(value_))
// {
	// _initString(std::move(name_));
// }

// inline JsonObjectField::JsonObjectField(const string_view_t& name_, JsonValue value_) noexcept
	// : type(String_view)
	// , value(std::move(value_))
// {
	// name.v = name_;
// }

// inline JsonObjectField::JsonObjectField(const char* name_, JsonValue value_)
	// : type(String)
	// , value(std::move(value_))
// {
	// _initString(JsonString(name_));
// }

// inline JsonObjectField::~JsonObjectField() noexcept {
    // if (type == String) {
		// name.s.~JsonString();
	// }
// }

// inline JsonObjectField& JsonObjectField::operator=(const JsonObjectField& other) {
	// JsonObjectField tmp(other);
	// std::swap(*this, tmp);
	// return *this;
// }

// inline size_t JsonObjectField::nameSize() const noexcept {
	// return (type == String) ? name.s.size() : name.v.len;
// }

// inline const char* JsonObjectField::nameStr() const noexcept {
	// return (type == String) ? name.s.c_str() : name.v.str;
// }



} // namespace asu


