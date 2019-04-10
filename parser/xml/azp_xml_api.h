#pragma once

#include <string>
#include <assert.h>
#include "azp_vector.h"
#include "azp_xml.h"


namespace azp {


using alloc_t = default_alloc_t;


typedef std::pair<string_view_t, string_view_t> XmlAttribute, XmlPInstr;
typedef vector<XmlAttribute, alloc_t>  XmlAttributes;

#if defined(_MSC_VER)
#define XmlTagSize      64
#else
#define XmlTagSize      80
#endif

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
void xml_writer(std::string& stm, const XmlDocument& doc);

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

} // namespace asu


