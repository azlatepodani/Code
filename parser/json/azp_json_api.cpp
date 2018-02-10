#include <algorithm>
#include <float.h>
#include <exception>
#include <stdio.h>
#include "azp_json.h"
#include "azp_json_api.h"


#ifndef  DBL_DECIMAL_DIG
#define DBL_DECIMAL_DIG 17
#endif // ! DBL_DECIMAL_DIG


namespace azp {


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


JsonValue& JsonValue::operator=(JsonValue&& other) noexcept {
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


static size_t json_writer_size(const JsonValue& val);
static void json_writer_imp(std::string& stm, const JsonValue& val);


void json_writer(std::string& stm, const JsonValue& val) {
	auto old = setlocale(LC_NUMERIC, "C");
	if (!old) throw std::exception(/*"runtime error"*/);
	
	auto size = json_writer_size(val);
	size += size/3;
	stm.reserve(size);
	json_writer_imp(stm, val);
	
	if (!setlocale(LC_NUMERIC, old)) throw std::exception(/*"runtime error"*/);
}


template <typename Allocator>
struct parser_callback_ctx_t {
	bool firstCall;
    vector<JsonValue, Allocator> stack;
    JsonValue * result;
	Allocator& a;
	
	parser_callback_ctx_t(Allocator& a) 
		: stack(a), firstCall(1), a(a), result(0)
	{ }
};


template <typename Allocator>
static bool parser_callback(void* ctx, enum ParserTypes type, const value_t& val) noexcept;


std::pair<JsonValue, std::string> json_reader(const std::string& stm) {
	auto val = std::pair<JsonValue, std::string>();

    if (!stm.empty()) {
		parser_t p;
		alloc_t a;

		auto ctx = parser_callback_ctx_t<alloc_t>(a);

		p.set_max_recursion(20);
		p.set_callback(&parser_callback<alloc_t>, &ctx);

		ctx.stack.reserve(p.get_max_recursion());

		ctx.result = &val.first;
		val.second = stm;
		
		if (!parseJson(p, &val.second[0], &val.second[0]+val.second.size())) throw std::exception(/*"cannot parse"*/);
	}
	
	return val;
}


static void jsonEscape(const char* src, size_t len, std::string& dst);
static void json_writer_object(std::string& stm, const JsonObject& val);
static void json_writer_array(std::string& stm, const JsonArray& val);
static std::string double_to_string(double dbl);


static void json_writer_imp(std::string& stm, const JsonValue& val) {
	switch (val.type) {
		case JsonValue::OBJECT:
			json_writer_object(stm, val.object());
			break;

		case JsonValue::ARRAY:
			json_writer_array(stm, val.array());
			break;
			
		case JsonValue::STRING:
			stm.push_back('"');{
			auto& s = val.string();
			jsonEscape(s.c_str(), s.size(), stm);}
			stm.push_back('"');
			break;
			
		case JsonValue::STRING_VIEW:
			stm.push_back('"');
			jsonEscape(val.u.view.str, val.u.view.len, stm);
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


static const struct num_size_t {
	uint32_t val;
	size_t size;
} g_tabl[] = {
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


static size_t number_size(int32_t num) {
	uint32_t n;
	int32_t negative = 0;
	
	if (num >= 0) n = num;
	else {
		n = -num;
		if (n == 0x80000000UL) return 11;
		negative = 1;	// minus sign
	}
	
	auto tabend = g_tabl + sizeof(g_tabl) / sizeof(g_tabl[0]);
	auto pos = std::lower_bound(g_tabl, tabend, n, [](const num_size_t& el, uint32_t num)
	{
		return num >= el.val;
	});
	
	return negative + ((pos != tabend) ? pos->size : 10);
}


static const struct num_size_ll_t {
	uint64_t val;
	size_t size;
} g_tabll[] = {
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


static size_t number_size(int64_t num) {
	int32_t negative = 0;
	
	uint64_t n;
	
	if (num >= -2147483647 && num <= 2147483647) return number_size(int32_t(num));
	
	if (num >= 0) n = num;
	else {
		n = -num;
		if (n == 0x8000000000000000ULL) return 20;
		negative = 1;
	}
	
	auto tabend = g_tabll + sizeof(g_tabll) / sizeof(g_tabll[0]);
	auto pos = std::lower_bound(g_tabll, tabend, n, [](const num_size_ll_t& el, uint64_t num)
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
			//size = double_to_string_size(val.u.float_num);	// more expensive than potentially doing an allocation
			size = 15;	// any number that cannot be exactly represented will require at least 19 chars
			break;
			
		case JsonValue::BOOL_FALSE:
		    size = 5;
			break;
			
		default:
			size = 4;
	}
	
	return size;
}


static bool needEscape(char ch)
{
    if ((uint8_t)ch < 0x20) {
        return true;
    }

    switch (ch) {
        case '"':
        case '\\':
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
	
    if (uint32_t(ch) < 0x20) {
        return esctab[ch];
    }

    return "";
}


static void jsonEscape(const char* src, size_t len, std::string& dst)
{
    auto last = src + len;
    
    while (src != last) {
        auto stop = src;

        while (!needEscape(*stop)) {
            stop++;
            if (stop == last) {
				dst.append(src, stop);
				return;
			}
        }

        dst.append(src, stop);
        dst.append(escape(*stop));

        src = stop+1;
    }
}


static void json_writer_object(std::string& stm, const JsonObject& val) {
	stm.push_back('{');
	
	if (val.cbegin() != val.cend()) {
		auto it = val.cbegin();
		stm.push_back('"');
		jsonEscape(it->name.c_str(), it->name.size(), stm);
		stm.push_back('"');
		stm.push_back(':');
		json_writer_imp(stm, it->value);
		for (++it; it != val.cend(); ++it) {
			stm.push_back(',');
			stm.push_back('"');
			jsonEscape(it->name.c_str(), it->name.size(), stm);
			stm.push_back('"');
			stm.push_back(':');
			json_writer_imp(stm, it->value);
		}
	}
	
	stm.push_back('}');
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


template <typename Allocator>
static bool add_scalar_value(parser_callback_ctx_t<Allocator>& cbCtx, JsonValue&& data) {
    JsonValue& val = cbCtx.stack.back();

    if (val.type == JsonValue::OBJECT) {
        val.object().back().value = std::move(data);
    }
    else if (val.type == JsonValue::ARRAY) {
        val.array().push_back(std::move(data));
    }
    else {
		return false;
    }

    return true;
}


template <typename Allocator>
static bool parser_callback(void* ctx, enum ParserTypes type, const value_t& value) noexcept {
	auto& cbCtx = *(parser_callback_ctx_t<Allocator> *)ctx;
	
	if (!cbCtx.firstCall) {
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
			JsonValue data(string_view_t{value.string.p, value.string.len});
			return add_scalar_value(cbCtx, std::move(data));
		}
		case Object_key: {
			JsonValue& obj = cbCtx.stack.back();

			obj.object().push_back(JsonObjectField(std::string(value.string.p, value.string.len), JsonValue()));
			break;
		}
		default:
			return false;
		}
	}
	else {
		cbCtx.firstCall = false;
		
		if (type == Object_begin) {
			cbCtx.stack.push_back(JsonValue(JsonObject(cbCtx.a, 4)));
			return true;
		}
		
		if (type == Array_begin) {
			cbCtx.stack.push_back(JsonValue(JsonArray(cbCtx.a, 4)));
			return true;
		}
		
		switch (type)
		{
		case Number_int:
			*cbCtx.result = JsonValue(value.integer);
			break;

		case Number_float:
			*cbCtx.result = JsonValue(value.number);
			break;

		case Null_val:
			*cbCtx.result = JsonValue();
			break;

		case Bool_true:
			*cbCtx.result = JsonValue(true);
			break;

		case Bool_false:
			*cbCtx.result = JsonValue(false);
			break;

		case String_val: {
			*cbCtx.result = JsonValue(string_view_t{value.string.p, value.string.len});
			break;
		}
		default: return false;
		}
	}

    return true;
}


static std::string double_to_string(double dbl) {
    char buf[340] = { 0, };
    auto len = sprintf(buf, "%.*g", DBL_DECIMAL_DIG, dbl);
    return std::string(buf, buf+len);
}


} // namespace asu
