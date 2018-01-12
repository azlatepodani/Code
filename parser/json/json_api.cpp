//#include "stdafx.h"
#include "json_api.h"
#include <ostream>
#include <algorithm>
#include <float.h>
#include "azp_json.h"
#include <memory>


#ifndef  DBL_DECIMAL_DIG
// This macro is undefined in VS2012
#define DBL_DECIMAL_DIG 17
#endif // ! DBL_DECIMAL_DIG


namespace asu {

using namespace azp;

static std::string jsonEscape(const std::string & src);

static void json_writer_object(std::ostream& stm, const JsonObject& val);
static void json_writer_array(std::ostream& stm, const JsonArray& val);

static JsonValue* json_build_parent_chain(JsonValue* val, const JsonString& path);

static JsonValue* json_get_immediate_child(JsonValue* val, const std::string& ipath);

static std::string double_to_string(double dbl);
static double string_to_double(const std::string& s);
static long long string_to_longlong(const std::string& s);
static std::string longlong_to_string(long long num);


struct parser_callback_ctx_t {
    std::vector<JsonValue> stack;
    JsonValue * result;
};

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
			
		case NUMBER:
		case FLOAT_NUM:
		case BOOL_TRUE:
		case BOOL_FALSE:
		default: // case EMPTY:
		;	// nothing to do
	}
}


void json_writer(std::ostream& stm, const JsonValue& val) {
	switch (val.type) {
		case val.OBJECT:
			json_writer_object(stm, val.object());
			break;

		case val.ARRAY:
			json_writer_array(stm, val.array());
			break;
			
		case val.STRING:
			stm << '"' << jsonEscape(val.string()) << '"';
			break;
			
		case val.NUMBER:
			stm << val.u.number;
			break;
			
		case val.FLOAT_NUM:
			stm << double_to_string(val.u.float_num);
			break;
			
		case val.BOOL_TRUE:
			stm << "true";
			break;
			
		case val.BOOL_FALSE:
		    stm << "false";
			break;
			
		default: // case EMPTY:
			stm << "null";
	}
}


JsonValue& json_get_child(JsonValue& val, const std::string& path) {
	JsonValue* presult = &val;
	
	size_t start = 0;
	size_t pos = path.find('.', 0);
	
	do {
		presult = json_get_immediate_child(presult, path.substr(start, pos-start));
        if (presult == nullptr) {
            throw std::exception((__FUNCTION__ " not found: " + path).c_str());
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


JsonValue& json_put_child(JsonValue& val, const std::string& path, JsonValue child) {
    size_t pos = path.rfind('.');
    JsonValue* parent = &val;

    if (pos != path.npos) {
        parent = json_build_parent_chain(&val, path.substr(0, pos));
    }
    else {
        if (val.type != val.OBJECT) {
            val = JsonValue(JsonObject());
        }
    }

    auto name = path.substr(pos+1);
    JsonValue * ptr = json_get_immediate_child(parent, name);
    if (ptr == nullptr) {
        // add a new entry
        auto& obj = parent->object();
        obj.push_back(JsonObjectField(name, std::move(child)));
        return obj.back().value;
    }
    else {
        // replace the old entry
        *ptr = child;
        return *ptr;
    }
}


JsonValue json_remove_child(JsonValue& val, const std::string& path) {
    JsonValue* presult = &val;
	
	size_t start = 0;
	size_t pos = path.find('.', 0);
	
	do {
        if (pos != path.npos) {
		    presult = json_get_immediate_child(presult, path.substr(start, pos-start));
            if (presult == nullptr) {
                return JsonValue();
            }
        }
        else {
            if (presult->type != JsonValue::OBJECT) {
                return JsonValue();
            }

            std::string name = path.substr(start);
            auto it = std::find_if(presult->object().begin(), presult->object().end(), [&](JsonObjectField& f) {
                return f.name == name;
            });

            if (it != presult->object().end()) {
                JsonValue tmp(std::move(it->value));
                presult->object().erase(it);
                return std::move(tmp);
            }

            return JsonValue();
        }

		start = pos + 1;
		pos = path.find('.', start);
	}
	while (true);

	return JsonValue();
}


// void json_reader(std::istream & stm, JsonValue & val) {
    // JSON_config config = {0};

    // init_JSON_config(&config);

    // parser_callback_ctx_t  ctx;
    // config.callback = &parser_callback;
    // config.callback_ctx = &ctx;
    // config.depth = 30;

    // ctx.result = &val;

    // JSON_parser parser = new_JSON_parser(&config);
    // if (parser == nullptr) {
        // throw std::bad_alloc();
    // }

    // std::unique_ptr<JSON_parser_struct, call_delete_JSON_parser> onExit(parser);

    // char buf[4096] = {0,};
    // std::streamsize avail = 0;
    // BOOL valid = TRUE;
    // JSON_error parseError = JSON_E_NONE;

    // do {
        // stm.read(buf, sizeof(buf));

        // avail = stm.gcount();
        // if (avail == 0) break;

        // for (std::streamsize i=0; i<avail; i++) {
            // valid = JSON_parser_char(parser, (unsigned char)buf[i]);
            // if (!valid) {
                // parseError = (JSON_error)JSON_parser_get_last_error(parser);
                // throw ParserError(parseError, buf, sizeof(buf), (size_t)i);
            // }
        // }
    // }
    // while (!stm.fail());

    // valid = JSON_parser_done(parser);
    // if (!valid) {
        // parseError = (JSON_error)JSON_parser_get_last_error(parser);
        // throw ParserError(parseError, buf, sizeof(buf), sizeof(buf));
    // }
// }


void json_reader(const std::vector<char>& stm, JsonValue & val) {
    if (stm.empty()) return;

    parser_t p;

    parser_callback_ctx_t  ctx;
	
	p.set_callback(&parser_callback, &ctx);
    //config.depth = 30;

    ctx.result = &val;

	std::string str(stm.begin(), stm.end());
	if (!parseJson(p, &str[0], &str[0]+str.size())) throw std::exception("cannot parse");
}


static bool needEscape(char ch)
{
    if (ch >= 0 && ch < 0x20) {
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
    if (ch >= 0 && ch < 0x20) {
        return esctab[ch];
    }

    switch (ch) {
        case '"':  return "\\\"";
        case '\\': return "\\\\";
        case '/':  return "\\/";

        default: break;
    }

    _ASSERT_EXPR(0, "BUG: nothing to escape");
    return "";
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


static void json_writer_object(std::ostream& stm, const JsonObject& val) {
	stm << '{';
	if (val.cbegin() != val.cend()) {
		auto it = val.cbegin();
		stm << '"' << jsonEscape(it->name) << "\":";
		json_writer(stm, it->value);
		for (++it; it != val.cend(); ++it) {
			stm << ",\"" << jsonEscape(it->name) << "\":";
			json_writer(stm, it->value);
		}
	}
	stm << '}';
}


static void json_writer_array(std::ostream& stm, const JsonArray& val) {
	stm << '[';
	if (val.cbegin() != val.cend()) {
		auto it = val.cbegin();
		json_writer(stm, *it);
		for (++it; it != val.cend(); ++it) {
			stm << ",";
			json_writer(stm, *it);
		}
	}
	stm << ']';
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
		throw std::exception((__FUNCTION__ " not implemented: " + ipath).c_str());
	}
}


static JsonValue* json_append_object_to_object(JsonValue* presult, std::string name) {
    presult->object().push_back(JsonObjectField(name, JsonValue(JsonObject())));
    return &presult->object().back().value;
}


static JsonValue* json_build_parent_chain(JsonValue* val, const JsonString& path) {
    JsonValue* presult = val;

	size_t start = 0;
	size_t pos = path.find('.', 0);
	
	do {
        JsonValue* last = presult;

		presult = json_get_immediate_child(presult, path.substr(start, pos-start));
        if (presult == nullptr) {
            presult = last;
            break;
        }

		if (pos == path.npos) {
            if (presult->type != JsonValue::OBJECT) {
                // convert this entry to an object
                *presult = JsonValue(JsonObject());
            }

            return presult;
        }
		
		start = pos + 1;
		pos = path.find('.', start);
	}
	while (true);

    if (presult->type != JsonValue::OBJECT) {
        // convert this entry to an object
        *presult = JsonValue(JsonObject());
    }

    do {
        presult = json_append_object_to_object(presult, path.substr(start, pos-start));
        if (pos == path.npos) {
            return presult;
        }

        start = pos + 1;
		pos = path.find('.', start);
    }
    while (true);
}


static int add_scalar_value(parser_callback_ctx_t * cbCtx, JsonValue data) {
    if (cbCtx->stack.begin() == cbCtx->stack.end()) {
        return false;   // this should be possible only if there is a bug in this function or in the parser
    }

    JsonValue& val = cbCtx->stack.back();

    if (val.type == val.ARRAY) {
        val.array().push_back(std::move(data));
    }
    else if (val.type == val.OBJECT) {
        val.object().back().value = std::move(data);
    }
    else {
        return false;   // this should be possible only if there is a bug in this function or in the parser
    }

    return true;
}


static bool parser_callback(void* ctx, enum ParserTypes type, const value_t& value) _NOEXCEPT {
    try {
        parser_callback_ctx_t * cbCtx = (parser_callback_ctx_t *)ctx;

        switch (type)
        {
        case Array_begin:
            cbCtx->stack.push_back(JsonValue(JsonArray()));
            break;
        case Object_begin:
            cbCtx->stack.push_back(JsonValue(JsonObject()));
            break;
        case Array_end:
        case Object_end: {
            if (cbCtx->stack.begin() == cbCtx->stack.end()) {
                return false;   // this should be possible only if there is a bug in this function or in the parser
            }

            JsonValue obj(cbCtx->stack.back());
            cbCtx->stack.pop_back();

            if (cbCtx->stack.begin() != cbCtx->stack.end()) {
                return add_scalar_value(cbCtx, obj);
            }
            else {
                // we're done
                *cbCtx->result = std::move(obj);
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
            JsonValue data((std::string(value.string, value.length)));
            return add_scalar_value(cbCtx, std::move(data));
        }
        case Object_key: {
            if (cbCtx->stack.begin() == cbCtx->stack.end()) {
                return false;   // this should be possible only if there is a bug in this function or in the parser
            }

            JsonValue& obj = cbCtx->stack.back();

            if (obj.type != obj.OBJECT) {
                return false;   // this should be possible only if there is a bug in this function or in the parser
            }

            obj.object().push_back(JsonObjectField(std::string(value.string, value.length), JsonValue()));
            break;
        }
        default:
            return false;
        }
    }
    catch (std::exception& ) {
        return false;
    }

    return true;
}


long long json_to_number(const JsonValue& val) {
    if (val.type == val.NUMBER) return val.u.number;
    if (val.type == val.STRING) {
        return string_to_longlong(val.string());
    }

    switch (val.type) {
    case JsonValue::BOOL_FALSE:
    case JsonValue::EMPTY: return 0LL;
    case JsonValue::FLOAT_NUM: return (long long)(val.u.float_num + 0.5); // round
    case JsonValue::BOOL_TRUE: return 1LL;
    default:
        throw std::exception("bad conversion");
    }
}


bool json_to_bool(const JsonValue& val) {
    if (val.type == val.BOOL_FALSE) return false;
    if (val.type == val.BOOL_TRUE) return true;
    if (val.type == val.STRING) {
        return val.string() == "true";
    }

    switch (val.type) {
    case JsonValue::NUMBER: return val.u.number != 0LL;
    case JsonValue::EMPTY: return 0LL;
    case JsonValue::FLOAT_NUM: return val.u.float_num != 0.;
    default:
        throw std::exception("bad conversion");
    }
}


std::string json_to_string(const JsonValue& val) {
    if (val.type == val.STRING) {
        return val.string();
    }

    if (val.type == val.NUMBER) {
        return longlong_to_string(val.u.number);
    }
    
    switch (val.type) {
    case JsonValue::EMPTY: return "null";
    case JsonValue::FLOAT_NUM: return double_to_string(val.u.float_num);
    case JsonValue::BOOL_FALSE: return "false";
    case JsonValue::BOOL_TRUE: return "true";
    default:
        throw std::exception("bad conversion");
    }
}


double json_to_float(const JsonValue& val) {
    if (val.type == val.FLOAT_NUM) return val.u.float_num;
    if (val.type == val.STRING) {
        return string_to_double(val.string());
    }

    switch (val.type) {
    case JsonValue::NUMBER: return (double)val.u.number;
    case JsonValue::BOOL_FALSE:
    case JsonValue::EMPTY: return 0.;
    case JsonValue::BOOL_TRUE: return 1.;
    default:
        throw std::exception("bad conversion");
    }
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


static double string_to_double(const std::string& s) {
    double dbl = 0;
    _snscanf_s_l(s.c_str(), s.size(), "%lg", g_loc, &dbl);
    return dbl;
}


static long long string_to_longlong(const std::string& s) {
    long long num = 0;
    _snscanf_s_l(s.c_str(), s.size(), "%lld", g_loc, &num);
    return num;
}


static std::string longlong_to_string(long long num) {
    char buf[64] = { 0, };
    auto len = _sprintf_s_l(buf, sizeof(buf), "%lld", g_loc, num);

    return std::string(buf, buf+len);
}


} // namespace asu