#include "core/basic.hpp"
#include "core/io.hpp"
#include "core/context.hpp"
#include "engine/json.hpp"
#include "core/conversions.hpp"

namespace rg
{

constexpr char SKIP_SPACE_OR_COMA_CHAR = ',' + 1;

intern JsonKind determine_kind_by_character(char c);

Maybe<JsonRepr> parse_json_from_path(Path* file_path)
{
    Maybe<JsonRepr> res;

    auto* talloc = get_temp_allocator();
    TEMP_ALLOC_SCOPE(talloc);

    auto [contents, is_ok] = file_read(talloc, file_path);
    if (!is_ok) return res;

    JsonParser parser = contents.view();
    res.set_val(parser.parse(talloc));
    return res;
}

JsonRepr JsonParser::parse(Arena* talloc)
{
    ASSERT_INITIALIZED(this);

    this->trim_space_both();
    auto [val, non_empty] = this->parse_value(talloc);
    ASSERT(non_empty);
    ASSERT_MSG(val.kind == JsonKind::TABLE, "Must be a table");
    return val.table;
}

Maybe<JsonValue> JsonParser::parse_value(Arena* talloc)
{
    ASSERT_MSG(!this->is_at_end(), "Expected json value parsing, but parser at end");
    this->trim_space_start();
    Maybe<JsonValue> res;
    char curr = this->curr_char();

    switch (curr)
    {
        case '{':
            // Test for empty table.
            if (this->next_char() == '}')
            {
                this->step();
                return res;
            }
            res.set_val(this->parse_table(talloc));
            return res;
        case '[':
            // Test for empty array.
            if (this->next_char() == ']')
            {
                this->step();
                return res;
            }
            res.set_val(this->parse_array(talloc));
            return res;
        case '"':
            res.set_val(this->parse_string());
            return res;
        case 't':
        case 'f':
            res.set_val(this->parse_bool());
            return res;
        case '-':
            res.set_val(this->parse_number());
            return res;
        default:
            if (is_digit(curr))
            {
                res.set_val(this->parse_number());
                return res;
            }
            UNREACHABLE("Unknown character encountered when trying to determine a next json parsing type, current string: " FMT_PLACEHOLDER_LEN,
                FMT_STR_VIEW_PTR(this));
    }
}

intern JsonKind determine_kind_by_character(char c)
{
    switch (c)
    {
        case '{':
            return JsonKind::TABLE;
        case '[':
            return JsonKind::ARRAY;
        case '"':
            return JsonKind::STRING;
        case '-':
            return JsonKind::NUMBER;
        case 't':
        case 'f':
            return JsonKind::BOOL;
        default:
            if (is_digit(c)) return JsonKind::NUMBER;
        UNREACHABLE("Unknown character encountered when trying to determine a next json parsing type, character encountered: %c", c);
    }
}

JsonValue JsonParser::parse_table(Arena* talloc)
{
    ASSERT(!this->is_at_end());
    this->expect_char_and_step('{');
    ASSERT(this->curr_char() != '}');

    JsonTable table;
    table.init(talloc, DEFAULT_JSON_TABLE_CAPACITY);
    this->skip_space();

    for (;;)
    {
        StrView key = this->parse_table_key();
        auto [value, non_empty] = this->parse_table_value(talloc);

        [[likely]] if (non_empty)
        {
            table.put(key, value);
        }

        if (this->curr_char() == '}') break;
    }

    this->expect_char_and_step('}');

    JsonValue val;
    val.init_as_table(table);
    return val;
}

JsonValue JsonParser::parse_array(Arena* talloc)
{
    ASSERT(!this->is_at_end());
    this->expect_char_and_step('[');
    ASSERT(this->curr_char() != ']');

    DArray<JsonValue> arr;
    arr.init(talloc, DEFAULT_JSON_ARRAY_CAPACITY);
    this->trim_space_start();

    JsonKind parsing_type = determine_kind_by_character(this->curr_char());
    ASSERT_MSG(parsing_type != JsonKind::ARRAY,
        "Can't be array inside of array directly in json, current string: ", FMT_PLACEHOLDER_LEN,
        FMT_STR_VIEW_PTR(this));

    switch (parsing_type)
    {
        case JsonKind::NUMBER:
            for (;;)
            {
                JsonValue num = this->parse_number();
                arr.push(num);
                this->skip_space_and_comma();
                if (this->curr_char() == ']') break;
            }
            break;
        case JsonKind::STRING:
            for (;;)
            {
                JsonValue str = this->parse_string();
                arr.push(str);
                this->skip_space_and_comma();
                if (this->curr_char() == ']') break;
            }
            break;
        case JsonKind::TABLE:
            for (;;)
            {
                // Test for empty table.
                if (this->next_char() == '}')
                {
                    this->skip_space_and_comma();
                    continue;
                }
                
                JsonValue table = this->parse_table(talloc);
                arr.push(table);
                this->skip_space_and_comma();
                if (this->curr_char() == ']') break;
            }
            break;
        case JsonKind::BOOL:
            for (;;)
            {
                JsonValue boolean = this->parse_bool();
                arr.push(boolean);
                this->skip_space_and_comma();
                if (this->curr_char() == ']') break;
            }
            break;
        default:
            UNREACHABLE("Unexpected parsing type: %d", parsing_type);
    }

    this->expect_char_and_step(']');

    JsonValue val;
    val.init_as_array(arr);
    return val;
}

Maybe<JsonValue> JsonParser::parse_table_value(Arena* talloc)
{
    Maybe<JsonValue> res = this->parse_value(talloc);
    this->skip_space_and_comma();
    return res;
}

StrView JsonParser::parse_table_key()
{
    ASSERT_MSG(this->curr_char() == '"', "Must be at quote when parsing table key");

    JsonValue table_key = this->parse_string();
    this->expect_char_and_step(':');
    this->trim_space_start();
    return table_key.string;
}

JsonValue JsonParser::parse_string()
{
    this->expect_char_and_step('"');

    const char* start = this->ptr;
    const char* curr = start;
    const char* end = this->end();
    char prev;

    while (*curr != '"' && prev != '\\')
    {
        prev = *curr;
        ++curr;
    }

    sz dist = curr - start;
    StrView str = this->view(0, dist);
    JsonValue val;
    val.init_as_str(str);
    this->step(dist + 1);

    return val;
}

void JsonParser::skip_space_and_comma()
{
    const char* start = this->ptr;
    const char* curr = start;

    while (is_space(*curr) || *curr == ',')
    {
        ++curr;
    }

    this->step(curr - start);
}

void JsonParser::skip_space()
{
    const char* start = this->ptr;
    const char* curr = start;

    while (is_space(*curr))
    {
        ++curr;
    }

    this->step(curr - start);
}

JsonValue JsonParser::parse_number()
{
    ASSERT_MSG(is_digit_or_sign(this->curr_char()), "Number must start with a digit or sign");

    JsonValue val;
    f32 res = 0.0;
    f32 sign = 1.0;

    const char* start = this->ptr;
    const char* curr = start;
    const char* end = this->end();

    if (*curr == '-' || *curr == '+')
    {
        if (*curr == '-') sign = -1;
        curr++;
    }

    ASSERT_MSG(curr != end, "Should be digits after sign in a number");
    ASSERT_MSG(is_digit(*curr), "Must be a digit after space / sign");

    // Integer part.
    while (curr != end && is_digit_or_dot(*curr))
    {
        res = res * 10.0 + (*curr - '0');
        ++curr;
    }

    if (*curr != '.')
    {
        res = res * sign;
        val.init_as_num(res);
        this->step(curr - start);
        return val;
    }

    ++curr;
    ASSERT_MSG(curr != end && is_digit(*curr), "Must be digits after dot in a floating point number");

    // Fractional part.
    f32 factor = 0.1; 

    while (curr != end && is_digit(*curr))
    {
        res += (*curr - '0') * factor;
        factor *= 0.1;
        ++curr;
    }

    res = res * sign;
    val.init_as_num(res);
    this->step(curr - start);
    return val;
}

JsonValue JsonParser::parse_bool()
{
    ASSERT_NON_EMPTY(this);
    ASSERT_MSG(this->curr_char() == 't' || this->curr_char() == 'f',
        "Attemp to slice a boolean which doesn't start from 't' or 'f', current string is: " FMT_PLACEHOLDER_LEN,
        FMT_STR_VIEW_PTR(this));

    JsonValue val;

    if (this->curr_char() == 't')
    {
        this->step(BOOL_TRUE_AS_STR.count);
        val.init_as_bool(true);
    }
    else
    {
        this->step(BOOL_FALSE_AS_STR.count);
        val.init_as_bool(false);
    }

    return val;
}

} // rg
