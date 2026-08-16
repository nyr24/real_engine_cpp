#ifndef _RG_JSON_HPP_
#define _RG_JSON_HPP_

#include "core/basic.hpp"
#include "core/allocators.hpp"
#include "core/io.hpp"
#include "collections/darray.hpp"
#include "collections/hashmap.hpp"
#include "collections/string.hpp"

namespace rg
{

enum struct JsonKind
{
    NUMBER,
    STRING,
    BOOL,
    ARRAY,
    TABLE
};

const StrView BOOL_TRUE_AS_STR = CSTR_SIZED("true");
const StrView BOOL_FALSE_AS_STR = CSTR_SIZED("false");
constexpr sz DEFAULT_JSON_ARRAY_CAPACITY = 32;
constexpr sz DEFAULT_JSON_TABLE_CAPACITY = 32;

struct JsonValue
{
    JsonKind kind;
    union
    {
        HashMap<StrView, JsonValue> table;
        DArray<JsonValue> array;
        f32 number;
        StrView string;
        bool boolean;
    };

    JsonValue();
    JsonValue(const JsonValue& rhs);
    JsonValue& operator=(const JsonValue& rhs);
    void init_as_str(StrView str);
    void init_as_num(f32 num);
    void init_as_bool(bool boolean);
    void init_as_table(const HashMap<StrView, JsonValue>& table);
    void init_as_array(const DArray<JsonValue>& array);
};

inline JsonValue::JsonValue() {}

inline JsonValue::JsonValue(const JsonValue& rhs)
{
    rg::mem_copy(this, &rhs, sizeof(*this));
}

inline JsonValue& JsonValue::operator=(const JsonValue& rhs)
{
    rg::mem_copy(this, &rhs, sizeof(*this));
    return *this;
}

inline void JsonValue::init_as_str(StrView str)
{
    this->kind = JsonKind::STRING; 
    this->string = str;
}
inline void JsonValue::init_as_num(f32 num)
{
    this->kind = JsonKind::NUMBER;
    this->number = num;
}
inline void JsonValue::init_as_bool(bool boolean)
{
    this->kind = JsonKind::BOOL;
    this->boolean = boolean;
}
inline void JsonValue::init_as_table(const HashMap<StrView, JsonValue>& table)
{
    this->kind = JsonKind::TABLE;
    this->table = table;
}
inline void JsonValue::init_as_array(const DArray<JsonValue>& array)
{
    this->kind = JsonKind::ARRAY;
    this->array = array;
}

alias JsonTable = HashMap<StrView, JsonValue>;
alias JsonArray = DArray<JsonValue>;
alias JsonString = StrView;
alias JsonRepr = JsonTable;

// Entrypoint.
Maybe<JsonRepr> parse_json_from_path(Path* file_path);

struct JsonParser : StrView
{
    JsonParser(StrView sv);

    JsonRepr parse(Arena* talloc);
    Maybe<JsonValue> parse_value(Arena* talloc);
    JsonValue parse_table(Arena* talloc);
    Maybe<JsonValue> parse_table_value(Arena* talloc);
    StrView parse_table_key();
    JsonValue parse_array(Arena* talloc);
    JsonValue parse_string();
    JsonValue parse_number();
    JsonValue parse_bool();

    void expect_char(char expected);
    void expect_char_and_step(char expected);
    void skip_space_and_comma();
    void skip_space();

    char curr_char() { return this->ptr[0]; }
    char next_char() { return this->ptr[1]; }
    void step(sz count = 1) { this->ptr += count; }
    bool is_at_end() { return this->count == 0; }
};

inline JsonParser::JsonParser(StrView sv)
{
    mem_copy(this, &sv, sizeof(*this));
    char curr_char(StrView parser);
}

inline void JsonParser::expect_char(char expected)
{
    ASSERT_MSG(expected == this->curr_char(), "Expected character: %c, found: %c",
        expected, this->curr_char());
}

inline void JsonParser::expect_char_and_step(char expected)
{
    this->expect_char(expected);
    this->step();
}

} // rg

#endif // _RG_JSON_HPP_
