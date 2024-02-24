#ifndef BEE_FORMAT_HPP
#define BEE_FORMAT_HPP

#include "ds.hpp"
#include <cctype>
#include <cstring>

namespace bee::fmt
{
// Bee format library (not {fmt} c++ library, inspired by go)

// example:
// string name = "world";
// f32 ns[] = {1.2, 2.3, 3.4, -4.5678};
// fmt::print("hello %s, %(d) %(2:.5) %(a:[*] b[] s' < ':%(f:+))\n", name, 256, 0, ns, Len(ns));
// stdout > hello world, 256     0 begin[+1.2 < +2.3 < +3.4 < -4.5678]end

// Format sequence defines the way the sequence is replaced by the argument.
// For example %d => format the current argument as an integer
// Format sequences only parses the next character (a verb) unless you add
// parenthesis after the '%', you can now use format specifiers with ':'
// For example %(s:.2) => format the current argument as a 2-width string
// Syntax: %verb, %(verb:specifiers)

// Verbs, describes the type of the format argument.
// The way the argument is formatted is implemented by
// a function: void format(Sequence *, Device *, type)
// Specifiers are verb dependent.
// ................................................
// %v > any types that overloads format
// %s > string types (const char *, char *, string)
// %c > char type
// %a > array types (c-like arrays)
// %r > ranges types (has .begin() and .end(), contains %v types)
// %d > float types (f(32, 64))
// %d > integer types (i(8,16,32,64), u(8,16,32,64))
// %z > size type (arch dependent)
// %t > boolean types
// %p > pointer
// %% > modulo escape sequence, '%%' => '%'
// ................................................

// Specifiers, describes the way that the argument is formatted.
// %(v:specifiers...)
// ................................................
// {is-space}     > ignored characters
//                  > {is-space} char: one-of ' ',\f,\n,\r,\t,\v
// .{size}        > %(v) restrict output to size
//                  > {pad} char : defines the max number of character being printed
// >{pad}{size}   > %(v) argument width (pad before print)
//                  > {pad} char : defines the argument padding character
//                  > {size} int : defines the number of characters being printed
// <{pad}{size}   > %(v) argument width (pad after print)
//                  > {pad} char : defines the argument padding character
//                  > {size} int : defines the minimal number of characters being printed
// /{pad}{size}   > %(v) minimal argument width (cannot shrink)
//                  > {pad} char : defines the argument padding character
//                  > {size} int : defines the minimal number of characters being printed
// !              > %(s) uppercase string
// ~              > %(s) lowercase string
// ?              > %(s,c) escape strings sequences by the amount of '?' (\',\",\?,\\,\a,\b,\f,\n,\r,\t,\v)
// +              > %(d,f) sign all numbers
// -              > %(d,f) sign negative numbers, default behavior
// _              > %(d,f) sign negative numbers and ' ' pad positives
// b{base}        > %(d,f) number base, ! wise
//                  > {base} int: non-negative number base
// !              > %(d,f) uppercase numbers and '#' of bases > 10
// #              > %(d,f) show number base representation before number, ! wise
//                  > 0b/0B: 2, 0o/0O: 8, 0d/0D: 10, 0x/0X: 16
// b{open}{close} > %(a,r) define array bounds, defaulted to b[]
//                  > {open} string  : printed before array elements
//                  > {close} string : printed after array elements
// s{separator}   > %(a,r) define array separator
//                  > {separator} string : printed between array elements
// [{size}]       > %(a,r) define number of elements
//                  > {size} int : number of elements
// :{sequence}    > %(a,r) define array element format sequence e.g. %(a:b[]:%(d:.2))
//                  > {sequence} format-sequence: defines how the array elements are formatted

#if 0
// Custom format types example
void format(Context *context, Device *dev, type x)
{
    string s = to_string(x);
    dev->print_argument(s);
}
#endif

template <typename T>
concept Fmt_Int = One_Of<Remove_Ref<T>, i16, i32, i64, u16, u32, u64>;

template <typename T>
concept Fmt_String = One_Of<Remove_Ref<T>, const char *, char *, struct string>;

template <typename T>
concept Fmt_Range = requires(T range) {
    requires !Same_As<Remove_Ref<T>, string>;
    { range.begin() } -> Same_As<typename T::iterator>;
    { range.end() } -> Same_As<typename T::iterator>;
};

template <typename T>
concept Fmt_Array = std::is_bounded_array_v<T>;

struct Write_Status
{
    char *it;
    size_t remaining;
    size_t overwrite;
};

struct Context;
struct Device;
void stream(FILE *f, string fmt, auto... args);
Write_Status write(char *buffer, size_t cap, string fmt, auto... args);
void print(string fmt, auto... args);
void error(string fmt, auto... args);
Array<char> array(string fmt, auto... args);
void format_spec(Context *context, Device *dev, auto... args);
void format_argument(Context *context, Device *dev, auto argument, auto... args);

enum Sign_Mode : u32
{
    Sign_Negative = 1,
    Sign_Positive = 2,
    Sign_Positive_With_Space = 4,
};

enum Spec_State
{
    Spec_None,
    Spec_Range,
    Spec_Pad,
    Spec_Width,
    Spec_Bounds_Open,
    Spec_Bounds_Close,
    Spec_Size_Open,
    Spec_Size_Close,
};

enum Width_Mode : u32
{
    Width_None = 0,
    Width_Shrink = 1,
    Width_Grow = 2,
};

struct Context
{
    string fmt;
    string sequence;
    string specifier;
    const char *spec; // current parsed specifier token
    const char *verb;
    Spec_State state;

    // %(v)
    u32 width_mode = Width_None;
    i32 width;
    char pad = ' ';

    // %(d, f)
    i32 base = 10;
    bool hash = false;
    u32 sign_mode = Sign_Negative;
    bool base_upcase = false;

    // %(f)
    i32 prec = -1;

    // %(s, c)
    i32 escapes = 0;
    bool upcase = false;
    bool downcase = false;

    // %(a, r)
    size_t size;
    string bounds[2];
    string separator = ",";
    Context *parent_ctx = NULL;
    Context *child_ctx = NULL;
};

struct Device
{
    void format(string fmt, auto... args)
    {
        Context context = {fmt};
        return format_argument(&context, this, args...);
    }

    virtual void print(string s) = 0;

    // Use the default device channel to print format messages (errors, warnings)
    virtual void error(string s)
    {
        print(s);
    }
    virtual void warn(string s)
    {
        print(s);
    }

    void print_n(char c, size_t n)
    {
        char buf[512];
        while (n != 0) {
            size_t size = Min(n, 512);
            n -= size;
            memset(buf, c, size);
            print(string{buf, size});
        }
    }

    void print_argument(Context *context, string s)
    {
        if (!context->width_mode) {
            print(s);
        } else if (context->width_mode & Width_Grow and context->width > s.len) {
            size_t pad_size = context->width - s.len;
            print_n(context->pad, pad_size);
        } else if (context->width_mode & Width_Shrink and context->width < s.len) {
            s.len = context->width;
            print(s);
        }
    }

    string format_message(char *buffer, size_t size, string type, string fmt, auto... args)
    {
        Assert(size > 4);
        const string elipsis = "...";
        const size_t len = size - elipsis.len - 2; // cap - ... - ')' - '\0'

        Write_Status ws = {buffer, size, 0};

        ws = write(ws.it, ws.remaining, "%%(%s! ", type);
        ws = write(ws.it, ws.remaining, fmt, args...);
        if (ws.overwrite > 0) {
            strcpy(&buffer[len], elipsis);
        }
        ws = write(ws.it, ws.remaining, ")");

        return string(buffer);
    }

    void errorf(string fmt, auto... args)
    {
        char buffer[512];
        error(format_message(buffer, sizeof buffer, "error", fmt, args...));
    }

    void warnf(string fmt, auto... args)
    {
        char buffer[512];
        warn(format_message(buffer, sizeof buffer, "warn", fmt, args...));
    }
};

struct Buffer_Device : Device
{
    char *buffer;
    char *it;
    size_t size;
    size_t overwrite;

    size_t remaining() const
    {
        return &buffer[size] - it;
    }

    void print(string s) override
    {
        size_t printed = Min(remaining(), s.len);
        overwrite += (s.len - printed);
        memcpy(it, s.begin(), printed);
        it[printed] = '\0';
        it = &it[printed];
    };
};

struct File_Device : Device
{
    FILE *f;

    void print(string s) override
    {
        fwrite(s.begin(), 1, s.len, f);
    };
};

struct Array_Device : Device
{
    Array<char> *array;

    void print(string s) override
    {
        array->concat(s.begin(), s.end());
    };
};

void stream(FILE *f, string fmt, auto... args)
{
    File_Device dev;
    dev.f = f;
    dev.format(fmt, args...);
}

void print(string fmt, auto... args)
{
    stream(stdout, fmt, args...);
}

void error(string fmt, auto... args)
{
    stream(stderr, fmt, args...);
}

Write_Status write(char *buffer, size_t size, string fmt, auto... args)
{

    Buffer_Device dev;
    dev.buffer = buffer;
    dev.it = buffer;
    dev.size = size;
    dev.overwrite = 0;

    dev.format(fmt, args...);

    return Write_Status{
        dev.it,
        dev.remaining(),
        dev.overwrite,
    };
}

Array<char> array(string fmt, auto... args)
{
    auto array = new_array<char>(fmt.len);
    defer(array.deinit());

    Array_Device dev;
    dev.array = &array;
    dev.format(fmt, args...);
    return array;
}

const string Number_Alphabet[2] = {
    "0123456789abcdefghijklmnopqrstuvwxyz",
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ",
};

void format(Context *context, Device *dev, Fmt_Int auto v)
{
    if (*context->verb != 'd')
        dev->warnf("expected %%d with integer type");

    char buf[64];
    size_t radix = 1;
    size_t size = 1;
    char *it = &buf[0];

    if (context->hash) {
        string hash[2] = {"", ""};
        switch (context->base) {
        case 2:
            hash[0] = "0b", hash[1] = "0B";
            break;
        case 8:
            hash[0] = "0o", hash[1] = "0O";
            break;
        case 10:
            hash[0] = "0d", hash[1] = "0D";
            break;
        case 16:
            hash[0] = "0x", hash[1] = "0X";
            break;
        default:
            dev->warnf("cannot print base hash");
            break;
        }

        const char *case_hash = hash[context->base_upcase];
        *it++ = case_hash[0];
        *it++ = case_hash[1];
    }
    for (int n = Abs(v); n >= context->base; radix++, size++)
        n /= context->base;
    if (context->sign_mode & Sign_Negative and v < 0)
        *it++ = '-';
    else if (context->sign_mode & Sign_Positive and v >= 0)
        *it++ = '+';
    else if (context->sign_mode & Sign_Positive_With_Space and v >= 0)
        *it++ = ' ';

    it += radix;
    for (int r = 0; r < radix; r++) {
        it[-r - 1] = Number_Alphabet[context->base_upcase][v % context->base];
        v /= context->base;
    }

    dev->print_argument(context, string{buf, it});
}

const string Alpha_Uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const string Alpha_Lowercase = "abcdefghijklmnopqrstuvwxyz";
const string Escape_Sequences = "\t\v\0\b\f\n\r\\\"";

void format(Context *context, Device *dev, Fmt_String auto v)
{
    if (*context->verb != 's')
        dev->warnf("expected %%s with string type");

    string s = v;
    char *replace = NULL;

    if (context->upcase)
        s = replace = s.replace(Alpha_Lowercase, Alpha_Uppercase);
    if (context->downcase)
        s = replace = s.replace(Alpha_Uppercase, Alpha_Lowercase);

    if (context->escapes != 0) {
        size_t size = s.len + 1;
        for (const char &esc : Escape_Sequences)
            size += s.count(esc) * context->escapes;

        replace = new char[size];
        size_t n = 0;

        auto escape = [&](char esc) {
            memset(&replace[n], '\\', context->escapes);
            n += context->escapes;
            replace[n] = esc;
            n++;
        };

        for (size_t i = 0; i < s.len; i++) {
            switch (s[i]) {
            case '\t':
                escape('t');
                break;
            case '\v':
                escape('v');
                break;
            case '\0':
                escape('0');
                break;
            case '\b':
                escape('b');
                break;
            case '\f':
                escape('f');
                break;
            case '\n':
                escape('n');
                break;
            case '\r':
                escape('r');
                break;
            case '\\':
                escape('\\');
                break;
            case '\"':
                escape('"');
                break;
            default:
                replace[n++] = s[i];
            }
        }
        s = replace;
    }

    dev->print_argument(context, s);
    if (replace != NULL) {
        delete[] replace;
    }
}

inline void format(Context *context, Device *dev, char v)
{
    format(context, dev, string{&v, 1});
}

void format_range(Context *context, Device *dev, auto begin, auto end)
{
    Context element_ctx = {};
    if (context->child_ctx)
        element_ctx = *context->child_ctx;

    Array_Device array_dev = {};
    Array array = new_array<char>();
    array_dev.array = &array;
    defer(array.deinit());

    array_dev.print(context->bounds[0]);
    for (auto it = begin; it != end; it++) {
        format(element_ctx, array_dev, *it);
        if (it != end - 1) {
            array_dev.print(context->separator);
        }
    }
    array_dev.print(context->bounds[1]);

    string s = {array.begin(), array.end()};
    dev->print_argument(context, s);
}

void format(Context *context, Device *dev, Fmt_Range auto range)
{
    return format_range(context, dev, range.begin(), range.end());
}
void format(Context *context, Device *dev, Fmt_Array auto array)
{
    return format_range(context, dev, &array[0], &array[context->size]);
}

inline char parse_spec_char(Context *context, Device *dev)
{
    return *context->spec++;
}
char parse_argument_char(Context *context, Device *dev, char v, auto...)
{
    return (u64)v;
}
char parse_argument_char(Context *context, Device *dev, auto...)
{
    dev->errorf("expected char * argument type");
    return 0;
}

inline u64 parse_spec_int(Context *context, Device *dev)
{
    char *end = NULL;
    u64 v = strtoul(context->spec, &end, 10);
    context->spec = end;
    return v;
}
u64 parse_argument_int(Context *context, Device *dev, Fmt_Int auto v, auto...)
{
    return (u64)v;
}
u64 parse_argument_int(Context *context, Device *dev, auto...)
{
    dev->errorf("expected int * argument type");
    return 0;
}

inline string parse_spec_str(Context *context, Device *dev)
{
    if (*context->spec != '\'') {
        return string{context->spec++, 1};
    }
    string s = {
        context->spec + 1,
        context->specifier.end(),
    };
    const char *end = s.find('\'');
    if (end == context->specifier.end()) {
        dev->errorf("unmatched string quote");
    }
    context->spec = end + 1;
    return s.end_at(end);
};
string parse_argument_str(Context *context, Device *dev, Fmt_String auto s, auto...)
{
    return s;
}
string parse_argument_str(Context *context, Device *dev, auto...)
{
    dev->errorf("expected string * argument type");
    return "";
}

void format_next_spec(Context *context, Device *dev, auto, auto... args)
{
    return format_spec(context, dev, args...);
}
void format_next_spec(Context *context, Device *dev, auto... args)
{
    return format_spec(context, dev, args...);
}

void vformat(Context *context, Device *dev, auto argument, auto... args)
{
    format(context, dev, argument);
    format_argument(context, dev, args...);
}

inline void vformat(Context *context, Device *dev)
{
    dev->errorf("missing argument");
}

void format_spec(Context *context, Device *dev, auto... args)
{
#define Parse(V, Spec_Fn, Argument_Fn)                  \
    if (*context->spec != '*') {                        \
        V = Spec_Fn(context, dev);                      \
        return format_spec(context, dev, args...);      \
    } else {                                            \
        context->spec++;                                \
        V = Argument_Fn(context, dev, args...);         \
        return format_next_spec(context, dev, args...); \
    }

#define Parse_Int(Int) Parse(Int, parse_spec_int, parse_argument_int)
#define Parse_Str(Str) Parse(Str, parse_spec_str, parse_argument_str)
#define Parse_Char(Char) Parse(Char, parse_spec_char, parse_argument_char)
#define Parse_Next() return format_spec(context, dev, args...)

    char spec = *context->spec++;

    switch (context->state) {
    case Spec_Pad:
        context->state = Spec_Width;
        Parse_Char(context->pad);

    case Spec_Width:
        context->state = Spec_None;
        Parse_Int(context->width);

    case Spec_Bounds_Open:
        context->state = Spec_Bounds_Close;
        Parse_Str(context->bounds[0]);

    case Spec_Bounds_Close:
        context->state = Spec_None;
        Parse_Str(context->bounds[1]);

    case Spec_Size_Open:
        context->state = Spec_Size_Close;
        Parse_Int(context->size);

    case Spec_Size_Close:
        context->state = Spec_None;
        Parse_Next();

    default:
        break;
    }

    auto assert_verbs = [&](string set) {
        char v = *context->verb;
        if (!set.has(v))
            dev->errorf("'%c': in '%c'", spec, v);
    };

    switch (spec) {
    case '>':
        context->width_mode = Width_Shrink | Width_Grow;
        context->state = Spec_Pad;
        Parse_Next();

    case '/':
        context->width_mode = Width_Grow;
        context->state = Spec_Pad;
        Parse_Next();

    case '#':
        context->hash = true;
        Parse_Next();
	
    case '-':
        context->sign_mode |= (Sign_Negative);
        Parse_Next();

    case '+':
        if (context->sign_mode & Sign_Positive_With_Space)
            dev->errorf("'+': discordant '_' sign specifier");
        context->sign_mode |= (Sign_Negative | Sign_Positive);
        Parse_Next();

    case '_':
        if (context->sign_mode & Sign_Positive)
            dev->errorf("'_': discordant '+' sign specifier");
        context->sign_mode |= (Sign_Negative | Sign_Positive_With_Space);
        Parse_Next();

    case 'b':
        assert_verbs("dfar");
        switch (*context->verb) {
        case 'd':
        case 'f':
            Parse_Int(context->base);

        case 'a':
        case 'r':
            context->state = Spec_Bounds_Open;
            Parse_Next();
        }

    case '[':
        assert_verbs("ar");
        context->state = Spec_Size_Open;
        Parse_Next();

    case ':': {
        assert_verbs("ar");
        Context child_ctx = {};
        context->child_ctx = &child_ctx;
        return format_spec(&child_ctx, dev, args...);
    }

    case '!':
        assert_verbs("dfs");
        switch (*context->verb) {
        case 's':
        case 'c':
            if (context->downcase)
                dev->errorf("'!': discordant '~' case specifier");
            context->upcase = true;
            Parse_Next();

        case 'd':
        case 'f':
            context->base_upcase = true;
            Parse_Next();
        }

    case '~':
        assert_verbs("sc");
        if (context->upcase)
            dev->errorf("'~': discordant '!' case specifier");
        context->downcase = true;
        Parse_Next();

    case '?':
        assert_verbs("sc");
        context->escapes++;
        Parse_Next();

    case ')':
        while (context->parent_ctx != NULL)
            context = context->parent_ctx;
        return vformat(context, dev, args...);

    default:
        dev->errorf("'%c': unknown specifier", spec);
        return vformat(context, dev, args...);
    }

    Assert(0);
#undef Parse
#undef Parse_Int
#undef Parse_Str
#undef Parse_Char
#undef Parse_Next
}

void format_argument(Context *context, Device *dev, auto... args)
{
    dev->print(context->fmt);
}

void format_argument(Context *context, Device *dev, auto argument, auto... args)
{
    const char *modulo = NULL;
    while (!modulo) {
        const char *match = context->fmt.find('%');
        bool has_verb = match + 1 < context->fmt.end();

        if (!has_verb) {
            dev->errorf("expected verb after '%'");
            context->fmt = {context->fmt.end(), 1};
            return;
        }

        if (match != context->fmt.end()) {
            if (match[1] == '%') {
                context->fmt = context->fmt.begin_at(&match[2]);
                dev->print("%");
                continue;
            }
            modulo = match;
        }
    }

    dev->print(context->fmt.end_at(modulo));
    if (modulo >= context->fmt.end())
        return dev->warnf("unused argument");

    string sequence = {&modulo[1], context->fmt.end()};
    if (modulo[1] != '(') {
        sequence.len = 1;
        context->verb = &sequence[0];
        context->fmt = context->fmt.begin_at(context->verb + 1);
        vformat(context, dev, argument, args...);
    } else {
        const char *sequence_end = sequence.find(')');
        if (sequence_end == sequence.end()) {
            return dev->errorf("unmatched sequence parenthesis");
        }

        context->fmt = context->fmt.begin_at(sequence_end + 1);
        context->verb = &sequence[1];
        context->spec = sequence.find(":");
        if (context->spec != sequence.end()) {
            context->spec++;
        }
        context->specifier = string{context->spec, sequence_end};
        return format_spec(context, dev, argument, args...);
    }
}

} // namespace bee::fmt

#endif
