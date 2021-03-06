#include <algorithm>
#include <float.h>
#include <exception>
#include <stdio.h>
#include <string.h>
#include <charconv>
#include "azp_json.h"
#include "azp_json_api.h"


#ifndef  DBL_DECIMAL_DIG
#define DBL_DECIMAL_DIG 17
#endif // ! DBL_DECIMAL_DIG


namespace azp {


// Compares the names of two JsonObjectField objects
struct less {
	bool operator()(const JsonObjectField& left, const JsonObjectField& right) {
		const char * lptr, * rptr;
		size_t lsize, rsize;
		
		if (left.type == JsonObjectField::String) {
			lptr = left.name.s.c_str();
			lsize = left.name.s.size();
		}
		else {
			lptr = left.name.v.str;
			lsize = left.name.v.len;
		}
		
		if (right.type == JsonObjectField::String) {
			rptr = right.name.s.c_str();
			rsize = right.name.s.size();
		}
		else {
			rptr = right.name.v.str;
			rsize = right.name.v.len;
		}
		
		auto msize = std::min(lsize, rsize);
		while (msize--) {
			auto c = *lptr++ - *rptr++;
			if (c) return c < 0;
		}
		
		return lsize < rsize;
	}
};


JsonValue& JsonValue::operator=(const JsonValue& other) {
	if (type == other.type) {
		//
		// This is a small optimization that allows for reusing the available buffers 
		//
		switch (type) {
			case Object:
				u.object = other.u.object;
				break;

			case Array:
				u.array = other.u.array;
				break;
				
			case String:
				u.string = other.u.string;
				break;
				
			case String_view: {    // String_view is not owning the pointer,
				JsonValue tmp(""); // so we convert it to String
				tmp.u.string.assign(other.u.view.str, other.u.view.len);
				return operator=(std::move(tmp));
			}
				
			case Number:
				u.number = other.u.number;
				break;
				
			case Float_num:
				u.float_num = other.u.float_num;
				break;
				
			// case Bool_true:
			// case Bool_false:
			default: // case Empty:
			;	// nothing to do
		}

		return *this;
	}
	else {
		switch (other.type) {
			case Object: {
				JsonValue tmp(other.u.object);
				return operator=(std::move(tmp));
			}
			case Array: {
				JsonValue tmp(other.u.array);
				return operator=(std::move(tmp));
			}
			case String: {
				JsonValue tmp(other.u.string);
				return operator=(std::move(tmp));
			}
			case String_view: {    // String_view is not owning the pointer,
				JsonValue tmp(""); // so we convert it to String
				tmp.u.string.assign(other.u.view.str, other.u.view.len);
				return operator=(std::move(tmp));
			}
			case Number: {
				JsonValue tmp(other.u.number);
				return operator=(std::move(tmp));
			}
			case Float_num: {
				JsonValue tmp(other.u.float_num);
				return operator=(std::move(tmp));
			}
			case Bool_true: {
				JsonValue tmp(true);
				return operator=(std::move(tmp));
			}
			case Bool_false: {
				JsonValue tmp(false);
				return operator=(std::move(tmp));
			}
			default: { // case Empty:
				JsonValue tmp(nullptr);
				return operator=(std::move(tmp));
			}
		}
	}
}


JsonValue& JsonValue::operator=(JsonValue&& other) noexcept {
	switch (type) {
		case Object:
			u.object.~JsonObject();
			break;

		case Array:
			u.array.~JsonArray();
			break;
			
		case String:
			u.string.~JsonString();
			break;
			
		// case Number:
		// case String_view:
		// case Float_num:
		// case Bool_true:
		// case Bool_false:
		default: // case Empty:
		;	// nothing to do
	}
	
	type = other.type;
	
	switch (other.type) {
		case Object:
			new (&u.object) JsonObject(std::move(other.u.object));
			break;

		case Array:
			new (&u.array) JsonArray(std::move(other.u.array));
			break;
			
		case String:
			_initString(std::move(other.u.string));
			break;
			
		case String_view:
			u.view = other.u.view;
			break;
			
		case Number:
			u.number = other.u.number;
			break;
			
		case Float_num:
			u.float_num = other.u.float_num;
			break;
			
		// case Bool_true:
		// case Bool_false:
		default: // case Empty:
		;	// nothing to do
	}

	return *this;
}


JsonValue::~JsonValue() noexcept {
    switch (type) {
		case Object:
			u.object.~JsonObject();
			break;

		case Array:
			u.array.~JsonArray();
			break;
			
		case String:
			u.string.~JsonString();
			break;
			
		// case String_view:
		// case Number:
		// case Float_num:
		// case Bool_true:
		// case Bool_false:
		default: // case Empty:
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
    vector<JsonValue, Allocator> stack;
	Allocator& a;
	
	// cppcheck-suppress noExplicitConstructor
	parser_callback_ctx_t(Allocator& a) 
		: stack(a), a(a)
	{ }
};


template <typename Allocator>
static bool parser_callback(void* ctx, enum ParserTypes type, const value_t& value) noexcept;


std::pair<JsonValue, std::string> json_reader(const std::string& stm) {
	auto val = std::pair<JsonValue, std::string>();

    if (!stm.empty()) {
		parser_t p;
		alloc_t a;

		auto ctx = parser_callback_ctx_t<alloc_t>(a);

		p.set_max_recursion(20);
		p.set_callback(&parser_callback<alloc_t>, &ctx);

		ctx.stack.reserve(p.get_max_recursion() + 1);
		val.second = stm;
        
        // ensure that we have something on the stack. This helps us avoid the empty stack case
        parser_callback<alloc_t>(&ctx, Array_begin, value_t());
		
		if (!parseJson(p, &val.second[0], &val.second[0]+val.second.size())) {
			throw std::exception(/*"cannot parse"*/);
		}
        
        val.first = std::move(*(ctx.stack.begin()->u.array.begin()));
	}
	
	return val;
}


static void jsonEscape(const char* src, size_t len, std::string& dst);
static void json_writer_object(std::string& stm, const JsonObject& val);
static void json_writer_array(std::string& stm, const JsonArray& val);
static void double_to_string(double dbl, std::string& stm);
static void longlong_to_string(long long num, std::string& stm);


static void json_writer_imp(std::string& stm, const JsonValue& val) {
	switch (val.type) {
		case JsonValue::Object:
			json_writer_object(stm, val.u.object);
			break;

		case JsonValue::Array:
			json_writer_array(stm, val.u.array);
			break;
			
		case JsonValue::String:
			stm.push_back('"');{
			auto& s = val.u.string;
			jsonEscape(s.c_str(), s.size(), stm);}
			stm.push_back('"');
			break;
			
		case JsonValue::String_view:
			stm.push_back('"');
			jsonEscape(val.u.view.str, val.u.view.len, stm);
			stm.push_back('"');
			break;
			
		case JsonValue::Number:
			longlong_to_string(val.u.number, stm);
			break;
			
		case JsonValue::Float_num:
			double_to_string(val.u.float_num, stm);
			break;
			
		case JsonValue::Bool_true:
			stm.append("true");
			break;
			
		case JsonValue::Bool_false:
		    stm.append("false");
			break;
			
		default: // case Empty:
			stm.append("null");
	}
}

/*
This code gives us perfect precision at performance cost
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
		if ((uint32_t)num == 0x80000000U) return 11;
		n = -num;
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
		if ((uint64_t)num == 0x8000000000000000ULL) return 20;
		n = -num;
		negative = 1;
	}
	
	auto tabend = g_tabll + sizeof(g_tabll) / sizeof(g_tabll[0]);
	auto pos = std::lower_bound(g_tabll, tabend, n, [](const num_size_ll_t& el, uint64_t num)
	{
		return num >= el.val;
	});
	
	return negative + ((pos != tabend) ? pos->size : 19);
}
//*/

static size_t json_writer_object_size(const JsonObject& val) {
	size_t size = 2;
	
	if (val.cbegin() != val.cend()) {
		auto it = val.cbegin();
		size += 3 + it->nameSize();
		size += json_writer_size(it->value);
		for (++it; it != val.cend(); ++it) {
			size += 4 + it->nameSize();
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
		case JsonValue::Object:
			size = json_writer_object_size(val.u.object);
			break;

		case JsonValue::Array:
			size = json_writer_array_size(val.u.array);
			break;
			
		case JsonValue::String:
			size = 2 + val.u.string.size();
			break;
			
		case JsonValue::String_view:
			size = 2 + val.u.view.len;
			break;
			
		case JsonValue::Number:
			//size = number_size(val.u.number);
            size = 8;
			break;
			
		case JsonValue::Float_num:
			//size = double_to_string_size(val.u.float_num);	// more expensive than potentially doing an allocation
			size = 15;	// any number that cannot be exactly represented will require at least 19 chars
			break;
			
		case JsonValue::Bool_false:
		    size = 5;
			break;
			
		default:
			size = 4;
	}
	
	return size;
}


static bool needEscape(char ch)
{
	if (ch == '"' || ch == '\\') return true;

    if ((uint8_t)ch < (uint8_t)0x20) {
        return true;
    }
	
	return false;
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


static void escape(std::string& dst, char ch)
{
	if (ch == '"' || ch == '\\') {
		dst.push_back('\\');
		dst.push_back(ch);
	}
	else {
		dst.append(esctab[ch], 6);
	}
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
		escape(dst, *stop);

        src = stop+1;
    }
}


static void json_writer_object(std::string& stm, const JsonObject& val) {
	stm.push_back('{');
	
	if (val.cbegin() != val.cend()) {
		auto it = val.cbegin();
		stm.push_back('"');
		jsonEscape(it->nameStr(), it->nameSize(), stm);
		stm.push_back('"');
		stm.push_back(':');
		json_writer_imp(stm, it->value);
		for (++it; it != val.cend(); ++it) {
			stm.push_back(',');
			stm.push_back('"');
			jsonEscape(it->nameStr(), it->nameSize(), stm);
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

    if (val.type == JsonValue::Object) {
        val.u.object.back().value = std::move(data);
    }
    else if (val.type == JsonValue::Array) {
        val.u.array.push_back(std::move(data));
    }
    else {
		return false;
    }

    return true;
}


template <typename Allocator>
static bool parser_callback(void* ctx, enum ParserTypes type, const value_t& value) noexcept {
	auto& cbCtx = *(parser_callback_ctx_t<Allocator> *)ctx;
	
    if (type == Object_key) {
        JsonValue& obj = cbCtx.stack.back();
        obj.u.object.push_back(JsonObjectField(string_view_t{value.string.p, value.string.len}, JsonValue()));
        return true;
    }
    
    if (type == String_val) {
        JsonValue data(string_view_t{value.string.p, value.string.len});
        return add_scalar_value(cbCtx, std::move(data));
    }
    
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
        
        /*
        if (type == Object_end) {
            auto& o = obj.u.object;
            std::sort(o.begin(), o.end(), less());
        }
        //*/
        
        return add_scalar_value(cbCtx, std::move(obj));
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

    default:
        return false;
    }

    return true;
}


#ifdef _MSC_VER
static void double_to_string(double dbl, std::string& stm) {
	size_t len;
    char buf[1024] = { 0, };
    len = sprintf(buf, "%.*g", DBL_DECIMAL_DIG, dbl);
    stm.append(buf, buf+len);
}
#else
static void double_to_string(double dbl, std::string& stm) {
	std::to_chars_result;
	char buf[1024] = {0,};
	res = std::to_chars(buf, std::end(buf), dbl, std::chars_format::general, DBL_DECIMAL_DIG);
	if (res.ec == std::errc()) {
		stm.append(buf, res.ptr);
	}
	else {
		throw std::exception();
	}
}
#endif


static void longlong_to_string(long long num, std::string& stm) {
	auto first = &stm.back();
	auto last = first + (stm.capacity() - stm.size());
	auto res = std::to_chars(first, last, num, 10);
	if (res.ec == std::errc()) {
		stm._Get_data()._Mysize += res.ptr - first;
	}
	else {
		char buf[64] = { 0, };
		res = std::to_chars(buf, std::end(buf), num, 10);
		if (res.ec != std::errc()) throw std::exception();
		stm.append(buf, res.ptr);
	}
}


JsonObjectField& JsonObjectField::operator=(JsonObjectField&& other) noexcept {
	if (type == String) {
		name.s.~JsonString();
	}
	
	if (other.type == String) {
		_initString(std::move(other.name.s));
	}
	else {
		name.v = other.name.v;
	}

	type = other.type;
	value = std::move(other.value);

	return *this;
}


JsonObjectField::JsonObjectField(const JsonObjectField& other) 
	: type(String)
	, value(other.value)
{
	if (other.type == String) {
		_initString(JsonString(other.name.s));
	}
	else {
		// String_view is not owning the pointer, so we convert it to String
		auto& v = other.name.v;
		_initString(JsonString(v.str, v.str + v.len));
	}
}


void optimize_for_search(JsonValue& root) noexcept {
	if (root.type == JsonValue::Object) {
		auto& obj = root.u.object;
		
		std::sort(obj.begin(), obj.end(), less());
		
		for (auto& v : obj) {
			optimize_for_search(v.value);
		}
	}
	else if (root.type == JsonValue::Array) {
		for (auto& v : root.u.array) {
			optimize_for_search(v);
		}
	}
	else { }
}

} // namespace asu
