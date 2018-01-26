#include "azp_json_api.h"
#include <ostream>
#include <algorithm>
#include <float.h>
#include "azp_json.h"
#include <memory>
#include <stdlib.h>


#ifndef  DBL_DECIMAL_DIG
// This macro is undefined in VS2012
#define DBL_DECIMAL_DIG 17
#endif // ! DBL_DECIMAL_DIG


namespace azp {


static std::string jsonEscape(const std::string & src);

static void json_writer_object(std::string& stm, const JsonObject& val);
static void json_writer_array(std::string& stm, const JsonArray& val);

static JsonValue* json_build_parent_chain(JsonValue* val, const JsonString& path);

static JsonValue* json_get_immediate_child(JsonValue* val, const std::string& ipath);

static std::string double_to_string(double dbl);
static double string_to_double(const std::string& s);
static long long string_to_longlong(const std::string& s);
static std::string longlong_to_string(long long num);

static void json_writer(vector<char>& out, const JsonValue& val);


template <typename Allocator>
struct parser_callback_ctx_t {
	parser_callback_ctx_t(Allocator& a) : stack(a), a(a) { }
    vector<JsonValue, Allocator> stack;
    JsonValue * result;
	Allocator& a;
};


template <typename Allocator>
static bool parser_callback(void* ctx, enum ParserTypes type, const value_t& val) _NOEXCEPT;


bool operator==(const JsonValue& left, const JsonValue& right) {
	if (left.type == right.type) {
		switch (left.type) {
			case JsonValue::OBJECT:
				return left.object() == right.object();

			case JsonValue::ARRAY:
				return left.array() == right.array();
				
			case JsonValue::STRING:
				return left.string() == right.string();
				
			case JsonValue::STRING_VIEW:
				return left.u.view.len == right.u.view.len &&
					strncmp(left.u.view.str, right.u.view.str, left.u.view.len) == 0;
				
			case JsonValue::NUMBER:
				return left.u.number == right.u.number;
				
			case JsonValue::FLOAT_NUM:
				return left.u.float_num == right.u.float_num;
				
			case JsonValue::BOOL_TRUE:
			case JsonValue::BOOL_FALSE:
			default: // case EMPTY:
				return true;
		}
	}
	
	return false;
}


bool operator==(const JsonObjectField& left, const JsonObjectField& right) {
	return (left.name == right.name) && (left.value == right.value);
}


JsonValue& JsonValue::operator=(const JsonValue& other) {
	if (type == other.type) {
		//
		// This is a small optimization that allows for reusing the available buffers 
		//
		switch (type) {
			case OBJECT:
				object() = other.object();
				break;

			case ARRAY:
				array() = other.array();
				break;
				
			case STRING:
				string() = other.string();
				break;
				
			case STRING_VIEW:
				string().assign(other.u.view.str, other.u.view.len);
				type = STRING;
				break;
				
			case NUMBER:
				u.number = other.u.number;
				break;
				
			case FLOAT_NUM:
				u.float_num = other.u.float_num;
				break;
				
			case BOOL_TRUE:
			case BOOL_FALSE:
			default: // case EMPTY:
			;	// nothing to do
		}

		return *this;
	}
	else {
		switch (other.type) {
			case OBJECT: {
				JsonValue tmp(other.object());
				return operator=(std::move(tmp));
			}
			case ARRAY: {
				JsonValue tmp(other.array());
				return operator=(std::move(tmp));
			}
			case STRING: {
				JsonValue tmp(other.string());
				return operator=(std::move(tmp));
			}
			case STRING_VIEW: {
				JsonValue tmp("");
				tmp.string().assign(other.u.view.str, other.u.view.len);
				return operator=(std::move(tmp));
			}
			case NUMBER: {
				JsonValue tmp(other.u.number);
				return operator=(std::move(tmp));
			}
			case FLOAT_NUM: {
				JsonValue tmp(other.u.float_num);
				return operator=(std::move(tmp));
			}
			case BOOL_TRUE: {
				JsonValue tmp(true);
				return operator=(std::move(tmp));
			}
			case BOOL_FALSE: {
				JsonValue tmp(false);
				return operator=(std::move(tmp));
			}
			default: { // case EMPTY:
				JsonValue tmp(nullptr);
				return operator=(std::move(tmp));
			}
		}
	}
}


JsonValue& JsonValue::operator=(JsonValue&& other) _NOEXCEPT {
	switch (type) {
		case OBJECT:
			object().~JsonObject();
			break;

		case ARRAY:
			array().~JsonArray();
			break;
			
		case STRING:
			string().~JsonString();
			break;
			
		case NUMBER:
		case STRING_VIEW:
		case FLOAT_NUM:
		case BOOL_TRUE:
		case BOOL_FALSE:
		default: // case EMPTY:
		;	// nothing to do
	}
	
	switch (other.type) {
		case OBJECT:
			new (u.buf) JsonObject(std::move(other.object()));
			break;

		case ARRAY:
			new (u.buf) JsonArray(std::move(other.array()));
			break;
			
		case STRING:
			_initString(std::move(other.string()));
			break;
			
		case STRING_VIEW:
			u.view = other.u.view;
			break;
			
		case NUMBER:
			u.number = other.u.number;
			break;
			
		case FLOAT_NUM:
			u.float_num = other.u.float_num;
			break;
			
		case BOOL_TRUE:
		case BOOL_FALSE:
		default: // case EMPTY:
		;	// nothing to do
	}
	
	type = other.type;

	return *this;
}


JsonValue::~JsonValue() {
    switch (type) {
		case OBJECT:
			object().~JsonObject();
			break;

		case ARRAY:
			array().~JsonArray();
			break;
			
		case STRING:
			string().~JsonString();
			break;
			
		case STRING_VIEW:
		case NUMBER:
		case FLOAT_NUM:
		case BOOL_TRUE:
		case BOOL_FALSE:
		default: // case EMPTY:
		;	// nothing to do
	}
}


void json_writer_imp(std::string& stm, const JsonValue& val) {
	switch (val.type) {
		case JsonValue::OBJECT:
			json_writer_object(stm, val.object());
			break;

		case JsonValue::ARRAY:
			json_writer_array(stm, val.array());
			break;
			
		case JsonValue::STRING:
			stm.push_back('"');
			stm.append(jsonEscape(val.string()));
			stm.push_back('"');
			break;
			
		case JsonValue::STRING_VIEW:
			stm.push_back('"');
			stm.append(jsonEscape(std::string(val.u.view.str, val.u.view.len)));
			stm.push_back('"');
			break;
			
		case JsonValue::NUMBER:
			stm.append(std::to_string(val.u.number));
			break;
			
		case JsonValue::FLOAT_NUM:
			stm.append(double_to_string(val.u.float_num));
			break;
			
		case JsonValue::BOOL_TRUE:
			stm.append("true");
			break;
			
		case JsonValue::BOOL_FALSE:
		    stm.append("false");
			break;
			
		default: // case EMPTY:
			stm.append("null");
	}
}


size_t json_writer_size(const JsonValue& val);


void json_writer(std::string& stm, const JsonValue& val) {
	auto size = json_writer_size(val);
	size += size/3;
	stm.reserve(size);
	json_writer_imp(stm, val);
}


static size_t number_size(long num) {
	static const struct num_size_t {
		long val;
		size_t size;
	} tab[] = {
		{ 10, 1 },
		{ 100, 2 },
		{ 1000, 3 },
		{ 10000, 4 },
		{ 100000, 5 },
		{ 1000000, 6 },
		{ 10000000, 7 },
		{ 100000000, 8 },
		{ 1000000000, 9 },
	};
	
	unsigned long n;
	bool negative = 0;
	
	if (num >= 0) n = num;
	else {
		n = -num;
		if (n == 0x80000000UL) return 11;
		negative = 1;	// minus sign
	}
	
	auto tabend = tab + sizeof(tab) / sizeof(tab[0]);
	auto pos = std::lower_bound(tab, tabend, n, [](const num_size_t& el, unsigned long num)
	{
		return num >= el.val;
	});
	
	return negative + ((pos != tabend) ? pos->size : 10);
}

static size_t number_size(long long num) {
	static const struct num_size_t {
		long long val;
		size_t size;
	} tab[] = {
		{ 10000000000LL, 10 },
		{ 100000000000LL, 11 },
		{ 1000000000000LL, 12 },
		{ 10000000000000LL, 13 },
		{ 100000000000000LL, 14 },
		{ 1000000000000000LL, 15 },
		{ 10000000000000000LL, 16 },
		{ 100000000000000000LL, 17 },
		{ 1000000000000000000LL, 18 },		// LLONG_MAX  9,223,372,036,854,775,807LL
	};
	
	bool negative = 0;
	
	unsigned long long n;
	
	if (num >= -2147483647 && num <= 2147483647) return number_size(long(num));
	
	if (num >= 0) n = num;
	else {
		n = -num;
		if (n == 0x8000000000000000ULL) return 20;
		negative = 1;
	}
	
	auto tabend = tab + sizeof(tab) / sizeof(tab[0]);
	auto pos = std::lower_bound(tab, tabend, n, [](const num_size_t& el, unsigned long long num)
	{
		return num >= el.val;
	});
	
	return negative + ((pos != tabend) ? pos->size : 19);
}


static size_t json_writer_object_size(const JsonObject& val) {
	size_t size = 2;
	
	if (val.cbegin() != val.cend()) {
		auto it = val.cbegin();
		size += 3 + it->name.size();
		size += json_writer_size(it->value);
		for (++it; it != val.cend(); ++it) {
			size += 4 + it->name.size();
			size += json_writer_size(it->value);
		}
	}

	return size;
}


static size_t json_writer_array_size(const JsonArray& val) {
	size_t size = 2;
	
	if (val.cbegin() != val.cend()) {
		auto it = val.cbegin();
		size += json_writer_size(*it);
		for (++it; it != val.cend(); ++it) {
			size += json_writer_size(*it) + 1;
		}
	}
	
	return size;
}


static size_t json_writer_size(const JsonValue& val) {
	size_t size = 0;
	
	switch (val.type) {
		case JsonValue::OBJECT:
			size = json_writer_object_size(val.object());
			break;

		case JsonValue::ARRAY:
			size = json_writer_array_size(val.array());
			break;
			
		case JsonValue::STRING:
			size = 2 + val.string().size();
			break;
			
		case JsonValue::STRING_VIEW:
			size = 2 + val.u.view.len;
			break;
			
		case JsonValue::NUMBER:
			size = number_size(val.u.number);
			break;
			
		case JsonValue::FLOAT_NUM:
			size = double_to_string(val.u.float_num).size();
			break;
			
		case JsonValue::BOOL_FALSE:
		    size = 5;
			break;
			
		default:
			size = 4;
	}
	
	return size;
}



JsonValue& json_get_child(JsonValue& val, const std::string& path) {
	JsonValue* presult = &val;
	
	size_t start = 0;
	size_t pos = path.find('.', 0);
	
	do {
		presult = json_get_immediate_child(presult, path.substr(start, pos-start));
        if (presult == nullptr) {
            throw std::exception((__FUNCTION__ + (" not found: " + path)).c_str());
        }

		if (pos == path.npos) break;
		
		start = pos + 1;
		pos = path.find('.', start);
	}
	while (true);

	return *presult;
}


JsonValue& json_get_child(JsonValue& val, const std::string& path, JsonValue& defVal) {
	JsonValue* presult = &val;
	
	size_t start = 0;
	size_t pos = path.find('.', 0);
	
	do {
		presult = json_get_immediate_child(presult, path.substr(start, pos-start));
        if (presult == nullptr) {
            return defVal;
        }

		if (pos == path.npos) break;
		
		start = pos + 1;
		pos = path.find('.', start);
	}
	while (true);

	return *presult;
}


std::pair<JsonValue, std::string> json_reader(const std::string& stm) {
    if (stm.empty()) return std::pair<JsonValue, std::string>();

    parser_t p;

	alloc_t a;
    auto ctx = parser_callback_ctx_t<alloc_t>(a);

	p.set_max_recursion(20);
	ctx.stack.reserve(p.get_max_recursion());
	
	p.set_callback(&parser_callback<alloc_t>, &ctx);

	std::pair<JsonValue, std::string> val;
    ctx.result = &val.first;
	val.second = stm;
	
	if (!parseJson(p, &val.second[0], &val.second[0]+val.second.size())) throw std::exception("cannot parse");
	
	return val;
}


static bool needEscape(char ch)
{
    if ((unsigned char)ch < 0x20) {
        return true;
    }

    switch (ch) {
        case '"':
        case '\\':
        case '/':
            return true;

        default:
            return false;
    }
}


static const char * const esctab[0x20] = {
        "\\u0000", "\\u0001", "\\u0002", "\\u0003",
        "\\u0004", "\\u0005", "\\u0006", "\\u0007",
        "\\u0008", "\\u0009", "\\u000a", "\\u000b",
        "\\u000c", "\\u000d", "\\u000e", "\\u000f",
        "\\u0010", "\\u0011", "\\u0012", "\\u0013",
        "\\u0014", "\\u0015", "\\u0016", "\\u0017",
        "\\u0018", "\\u0019", "\\u001a", "\\u001b",
        "\\u001c", "\\u001d", "\\u001e", "\\u001f",
    };

static const char * escape(char ch)
{
	if (ch == '"') return "\\\"";
	if (ch == '\\') return "\\\\";
	
    if (unsigned(ch) < 0x20) {
        return esctab[ch];
    }

    if (ch == '/')  return "\\/";

    return "";
}

static void escape(vector<char>& out, char ch)
{
	if ((ch == '"') | (ch == '\\') | (ch == '/')) {
		out.push_back('\\');
		out.push_back(ch);
		return;
	}
	else {
		if (unsigned(ch) < 0x20) {
			auto str = esctab[ch];
			while (*str) out.push_back(*str++);
			return;
		}
	}

    out.push_back(ch);
}



static std::string jsonEscape(const std::string & src)
{
    std::string res;
    std::string::size_type start = 0;
    std::string::size_type stop = 0;
    
    while (start < src.length()) {
        stop = start;

        while (!needEscape(src[stop])) {
            stop++;
            if (stop == src.length()) break;
        }

        res.append(&src[start], &src[stop]);

        if (stop >= src.length()) break;

        res.append(escape(src[stop]));

        start = stop+1;
    }

    return res;
}



static void jsonEscape(vector<char>& out, const char* first, size_t len) {
	auto last = first + len;
	
	out.reserve(out.size() + len);
	
    while (first != last) {
        escape(out, *first++);
    }
}


static void jsonEscape(vector<char>& out, const std::string& src)
{
	jsonEscape(out, src.c_str(), src.size());
}



static void json_writer_object(std::string& stm, const JsonObject& val) {
	stm.push_back('{');
	
	if (val.cbegin() != val.cend()) {
		auto it = val.cbegin();
		stm.push_back('"');
		stm.append(jsonEscape(it->name));
		stm.push_back('"');
		stm.push_back(':');
		json_writer_imp(stm, it->value);
		for (++it; it != val.cend(); ++it) {
			stm.push_back(',');
			stm.push_back('"');
			stm.append(jsonEscape(it->name));
			stm.push_back('"');
			stm.push_back(':');
			json_writer_imp(stm, it->value);
		}
	}
	
	stm.push_back('}');
}


static void json_writer_object(vector<char>& out, const JsonObject& val) {
	out.push_back('{');
	
	if (val.cbegin() != val.cend()) {
		auto it = val.cbegin();
		out.push_back('"');
		jsonEscape(out, it->name);
		out.push_back('"');
		out.push_back(':');
		json_writer(out, it->value);
		for (++it; it != val.cend(); ++it) {
			out.push_back(',');
			out.push_back('"');
			jsonEscape(out, it->name);
			out.push_back('"');
			out.push_back(':');
			json_writer(out, it->value);
		}
	}
	
	out.push_back('}');
}



static void json_writer_array(std::string& stm, const JsonArray& val) {
	stm.push_back('[');
	if (val.cbegin() != val.cend()) {
		auto it = val.cbegin();
		json_writer_imp(stm, *it);
		for (++it; it != val.cend(); ++it) {
			stm.push_back(',');
			json_writer_imp(stm, *it);
		}
	}
	stm.push_back(']');
}


static void json_writer_array(vector<char>& out, const JsonArray& val) {
	out.push_back('[');
	if (val.cbegin() != val.cend()) {
		auto it = val.cbegin();
		json_writer(out, *it);
		for (++it; it != val.cend(); ++it) {
			out.push_back(',');
			json_writer(out, *it);
		}
	}
	out.push_back(']');
}


static void append(vector<char>& out, const std::string& s) {
	auto remaining = s.size();
	out.reserve(out.size() + s.size());
	auto first = s.c_str();
	while (remaining--) {
		out.push_back(*first++);
	}
}


static void json_writer(vector<char>& out, const JsonValue& val) {
	switch (val.type) {
		case JsonValue::OBJECT:
			json_writer_object(out, val.object());
			break;

		case JsonValue::ARRAY:
			json_writer_array(out, val.array());
			break;
			
		case JsonValue::STRING:
			out.push_back('"');
			jsonEscape(out, val.string());
			out.push_back('"');
			break;
			
		case JsonValue::STRING_VIEW:
			out.push_back('"');
			jsonEscape(out, val.u.view.str, val.u.view.len);
			out.push_back('"');
			break;

		case JsonValue::NUMBER:
			append(out, std::to_string(val.u.number));
			break;

			
		case JsonValue::FLOAT_NUM:
			append(out, double_to_string(val.u.float_num));
			break;
			
		case JsonValue::BOOL_TRUE:
			append(out, "true");
			break;
			
		case JsonValue::BOOL_FALSE:
		    append(out, "false");
			break;
			
		default: // case EMPTY:
			append(out, "null");
	}
}


void json_writer3(std::string& stm, const JsonValue& val) {
	default_alloc_t a;
	vector<char> out(a, 128);
	json_writer(out, val);
	stm.assign(out.begin(), out.end());
}



static JsonValue* json_get_immediate_child(JsonValue* val, const std::string& ipath) {
    auto type = val->type;
	if (type != val->OBJECT && type != val->ARRAY) {
		return nullptr;
	}
	
	if (ipath[0] != '[') {	// not looking for nth element
		if (type != val->OBJECT)  {	// only objects can be accessed by name
			return nullptr;
		}
		
        auto& obj = val->object();
		for (auto& el : obj) {
			if (el.name == ipath) {
				return &el.value;
			}
		}
		
		return nullptr;
	}
	else {
		throw std::exception((__FUNCTION__ + (" not implemented: " + ipath)).c_str());
	}
}


template <typename Allocator>
static int add_scalar_value(parser_callback_ctx_t<Allocator>& cbCtx, JsonValue&& data) {
    JsonValue& val = cbCtx.stack.back();

    if (val.type == JsonValue::OBJECT) {
        val.object().back().value = std::move(data);
    }
    else if (val.type == JsonValue::ARRAY) {
        val.array().push_back(std::move(data));
    }
    else {
        //val = std::move(data);
		return false;
    }

    return true;
}


template <typename Allocator>
static bool parser_callback(void* ctx, enum ParserTypes type, const value_t& value) _NOEXCEPT {
	auto& cbCtx = *(parser_callback_ctx_t<Allocator> *)ctx;
	
	switch (type)
	{
	case Array_begin:
		cbCtx.stack.push_back(JsonValue(JsonArray(cbCtx.a, 4)));
		break;
	case Object_begin:
		cbCtx.stack.push_back(JsonValue(JsonObject(cbCtx.a, 4)));
		break;
	case Array_end:
	case Object_end: {
		JsonValue obj(std::move(cbCtx.stack.back()));
		cbCtx.stack.pop_back();

		if (cbCtx.stack.begin() != cbCtx.stack.end()) {
			return add_scalar_value(cbCtx, std::move(obj));
		}
		else {
			// we're done
			*cbCtx.result = std::move(obj);
		}
		break;
	}
	case Number_int:
		return add_scalar_value(cbCtx, JsonValue(value.integer));

	case Number_float:
		return add_scalar_value(cbCtx, JsonValue(value.number));

	case Null_val:
		return add_scalar_value(cbCtx, JsonValue());

	case Bool_true:
		return add_scalar_value(cbCtx, JsonValue(true));

	case Bool_false:
		return add_scalar_value(cbCtx, JsonValue(false));

	case String_val: {
		JsonValue data(string_view_t{value.string, value.length});
		return add_scalar_value(cbCtx, std::move(data));
	}
	case Object_key: {
		JsonValue& obj = cbCtx.stack.back();

		obj.object().push_back(JsonObjectField(std::string(value.string, value.length), JsonValue()));
		break;
	}
	default:
		return false;
	}

    return true;
}


struct C_Numeric_Locale {
    _locale_t loc;

    C_Numeric_Locale() : loc(_create_locale(LC_NUMERIC, "C"))
    {
        if (loc == nullptr) throw std::exception("cannot create the locale");
    }

    ~C_Numeric_Locale() {
        _free_locale(loc);
    }

    operator _locale_t() { return loc; }
} g_loc;


static std::string double_to_string(double dbl) {
    char buf[64] = { 0, };
    auto len = _sprintf_s_l(buf, sizeof(buf), "%.*g", g_loc, DBL_DECIMAL_DIG, dbl);

    return std::string(buf, buf+len);
}


} // namespace asu