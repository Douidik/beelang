#ifndef BEE_FORMAT_HPP
#define BEE_FORMAT_HPP

#include "ds.hpp"
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstring>

namespace bee::fmt
{
// Bee format library (not {fmt} c++ library, inspired by go)

// todo! update examples
// example:
// string name = "world";
// f32 ns[] = {1.2, 2.3, 3.4, -4.5678};
// fmt::print("hello %s, %(d) %(2:.5) %(r:[*] b[] s' < ':%(f:+))\n", name, 256, 0, ns, Len(ns));
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
// %r > ranges types (has .begin() and .end(), contains %v types)
// %d > float types (f(32, 64))
// %d > integer types (i(8,16,32,64), u(8,16,32,64))
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
// <{pad}{size}   > %(v) argument width (pad after print)
//                  > {pad} char : defines the argument padding
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
// %{sequence}    > %(a,r) define array element format sequence e.g. %(a:b[]%(d:.2))
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
concept Fmt_Float = One_Of<Remove_Ref<T>, f32, f64>;

template <typename T>
concept Fmt_String = One_Of<Remove_Ref<T>, const char *, char *, struct string>;

template <typename T>
concept Fmt_Range = requires(T range) {
    requires !Same_As<Remove_Ref<T>, string>;
    { range.begin() } -> Same_As<typename T::iterator>;
    { range.end() } -> Same_As<typename T::iterator>;
};

struct Write_Status
{
    char *it;
    size_t remaining;
    size_t overwrite;
};

Write_Status new_write_status(char *buf, size_t size);

struct Context;
struct Device;
void stream(FILE *f, string fmt, auto... args);
Write_Status write(char *buf, size_t cap, string fmt, auto... args);
Write_Status append(Write_Status ws, string fmt, auto... args);
void print(string fmt, auto... args);
void error(string fmt, auto... args);
Vec<char> heap(string fmt, auto... args);
void format_spec(Context *context, Device *dev, auto... args);
string parse_sequence(Context *context, Device *dev);
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
    Spec_Width,
    Spec_Pad,
    Spec_Bounds_Open,
    Spec_Bounds_Close,
    Spec_Size_Open,
    Spec_Size_Close,
};

const u32 Max_Escapes = 32;

struct Context
{
    string fmt;
    const char *spec; // specifier token iterator
    const char *verb;
    Spec_State state;

    // %(v)
    char align = '<';
    i32 width = -1;
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

    // %(r)
    i64 size = -1;
    string bounds[2] = {"[", "]"};
    string separator = ", ";
    Context *sub_context;
    Context *up_context;
};

Context new_context(string fmt);

struct Device
{
    void format(string fmt, auto... args)
    {
        Context context = new_context(fmt);
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
            size_t size = Min(n, sizeof(buf));
            n -= size;
            memset(buf, c, size);
            print(string{buf, size});
        }
    }

    void print_argument(Context *context, string s)
    {
        if (context->width < 0) {
            return print(s);
        }
        if (context->width < s.len) {
            s.len = context->width;
            return print(s);
        }

        i64 pad_size = context->width - s.len;

        switch (context->align) {
        case '>': {
            print_n(context->pad, pad_size);
            print(s);
        } break;

        case '<': {
            print(s);
            print_n(context->pad, pad_size);
        } break;

        case '^': {
            size_t before = ceil((f32)pad_size / 2);
            size_t after = floor((f32)pad_size / 2);

            print_n(context->pad, before);
            print(s);
            print_n(context->pad, after);
        } break;
        }
    }

    string format_message(char *buf, size_t size, string type, string fmt, auto... args)
    {
        Assert(size > 4);
        const string elipsis = "...";
        const size_t len = size - elipsis.len - 2; // cap - ... - ')' - '\0'

        Write_Status ws = {buf, size, 0};

        ws = write(ws.it, ws.remaining, "%%(%s! ", type);
        ws = write(ws.it, ws.remaining, fmt, args...);
        if (ws.overwrite > 0) {
            strcpy(&buf[len], elipsis);
        }
        ws = write(ws.it, ws.remaining, ")");

        return string(buf);
    }

    void errorf(string fmt, auto... args)
    {
        char buf[512] = {0};
        error(format_message(buf, sizeof(buf), "error", fmt, args...));
    }

    void warnf(string fmt, auto... args)
    {
        char buf[512] = {0};
        warn(format_message(buf, sizeof(buf), "warn", fmt, args...));
    }
};

struct Buf_Device : Device
{
    char *buf;
    char *it;
    size_t size;
    size_t overwrite;

    size_t remaining() const
    {
        return &buf[size] - it;
    }

    void print(string s) override
    {
        // size of copy including \0
        size_t printed = Min(remaining(), s.len + 1);
        overwrite += (s.len + 1 - printed);
        it = stpncpy(it, s.begin(), printed - 1);
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

struct Vec_Device : Device
{
    Vec<char> *vec;

    void print(string s) override
    {
        vec->concat(s.begin(), s.end());
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

Write_Status write(char *buf, size_t size, string fmt, auto... args)
{
    Buf_Device dev;
    dev.buf = buf;
    dev.it = buf;
    dev.size = size;
    dev.overwrite = 0;

    dev.format(fmt, args...);

    return Write_Status{
        dev.it,
        dev.remaining(),
        dev.overwrite,
    };
}

Write_Status append(Write_Status ws, string fmt, auto... args)
{
    return write(ws.it, ws.remaining, fmt, args...);
}

Vec<char> heap(string fmt, auto... args)
{
    Vec vec = new_vec<char>(fmt.len);
    defer(vec.deinit());

    Vec_Device dev;
    dev.vec = &vec;
    dev.format(fmt, args...);
    return vec;
}

void expect_token(Context *context, Device *dev, char token, string set, string fmt = "", auto... args)
{
    [[unlikely]] if (!set.has(token))
        dev->warnf(fmt, args...);
}

const string Number_Alphabet[2] = {
    "0123456789abcdefghijklmnopqrstuvwxyz",
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ",
};

void format(Context *context, Device *dev, Fmt_Int auto v)
{
    expect_token(context, dev, *context->verb, "vd", "expected %%d with integer type");

    char buf[64];
    size_t radix = 1;
    size_t size = 1;
    char *it = &buf[0];
    string alphabet = Number_Alphabet[context->base_upcase];

    for (auto n = Abs(v); n >= context->base; radix++, size++)
        n /= context->base;
    if (context->sign_mode & Sign_Negative and v < 0)
        *it++ = '-';
    else if (context->sign_mode & Sign_Positive and v >= 0)
        *it++ = '+';
    else if (context->sign_mode & Sign_Positive_With_Space and v >= 0)
        *it++ = ' ';

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

    it += radix;
    for (int r = 0; r < radix; r++) {
        it[-r - 1] = alphabet[v % context->base];
        v /= context->base;
    }

    dev->print_argument(context, string{buf, it});
}

inline void format(Context *context, Device *dev, const void *v)
{
    format(context, dev, (size_t)v);
}

size_t string_escaped_len(Context *context, Device *dev, string s);
string format_escape(Context *context, Device *dev, string input, View<char> output);
string format_char_fn(Context *context, Device *dev, string input, View<char> output, int (*fn)(int));

const string Uppercase = "qwertyuiopasdfghjklzxcvbnm";
const string Lowercase = "QWERTYUIOPASDFGHJKLZXCVBNM";
const string Escape_Sequences = "\t\v\0\b\f\n\r\\\"";

inline bool do_string_transform(Context *context, string s)
{
    return (context->upcase and s.has(Lowercase)) or (context->downcase and s.has(Uppercase)) or
           (context->escapes != 0 and s.has(Escape_Sequences));
}

void format(Context *context, Device *dev, Fmt_String auto v)
{
    expect_token(context, dev, *context->verb, "vs", "expected %%s with string type");

    string s = v;
    size_t len = s.len;
    char *buf = NULL;
    View<char> view = {};

    if (do_string_transform(context, s)) {
        if (context->escapes != 0)
            len = string_escaped_len(context, dev, s);
        buf = new char[len + 1];
        view = {&buf[0], len};
        strncpy(view.data, s.data, s.len);
        s = {buf, len};

        string as_string = {view.begin(), view.end()};
        if (context->upcase)
            format_char_fn(context, dev, as_string, view, toupper);
        if (context->downcase)
            format_char_fn(context, dev, as_string, view, tolower);
        if (context->escapes != 0)
            format_escape(context, dev, as_string, view);
    }

    dev->print_argument(context, s);
    if (buf != NULL)
        delete[] buf;
}

static void format(Context *context, Device *dev, char c)
{
    expect_token(context, dev, *context->verb, "vc", "expected %%c with char type");

    if (!do_string_transform(context, string{&c, 1}))
        return dev->print_argument(context, string{&c, 1});

    // Cannot escape more than 32 times
    char buf[64] = {c, '\0'};
    size_t len = 1;

    if (context->upcase)
        format_char_fn(context, dev, {buf, 1}, {buf, 1}, toupper);
    if (context->downcase)
        format_char_fn(context, dev, {buf, 1}, {buf, 1}, tolower);
    if (context->escapes != 0) {
        len = string_escaped_len(context, dev, string{buf, 1});
        format_escape(context, dev, {buf, 1}, {buf, len});
    }

    return dev->print_argument(context, string{buf, len});
}

void format_range(Context *context, Device *dev, auto begin, auto end)
{
    Context sub_context_v = new_context(context->fmt);
    sub_context_v.verb = "v";

    if (!context->sub_context) {
        context->sub_context = &sub_context_v;
    }

    Vec_Device vec_dev = {};
    Vec vec = new_vec<char>();
    vec_dev.vec = &vec;
    defer(vec.deinit());

    vec_dev.print(context->bounds[0]);
    for (auto it = begin; it != end; it++) {
        format(context->sub_context, &vec_dev, *it);
        if (it != end - 1) {
            vec_dev.print(context->separator);
        }
    }
    vec_dev.print(context->bounds[1]);

    string s = {vec.begin(), vec.end()};
    dev->print_argument(context, s);
}

void format(Context *context, Device *dev, Fmt_Range auto range)
{
    expect_token(context, dev, *context->verb, "vr", "expected %%r with range type");
    auto end = range.end();
    if (context->size != -1) {
        end = range.begin() + context->size;
    }
    return format_range(context, dev, range.begin(), end);
}

inline char parse_spec_char(Context *context, Device *dev)
{
    return *context->spec++;
}
char parse_argument_char(Context *context, Device *dev, auto v, char argument, auto...)
{
    return (u64)argument;
}
char parse_argument_char(Context *context, Device *dev, auto...)
{
    dev->errorf("missing char * argument type");
    return 0;
}

inline u64 parse_spec_int(Context *context, Device *dev)
{
    char *end = NULL;
    errno = 0;

    u64 v = strtoul(context->spec, &end, 10);
    if (errno != 0) {
        dev->errorf("int decode error (%s)", strerror(errno));
    }
    context->spec = end;
    return v;
}
u64 parse_argument_int(Context *context, Device *dev, auto v, Fmt_Int auto argument, auto...)
{
    return (u64)argument;
}
u64 parse_argument_int(Context *context, Device *dev, auto...)
{
    dev->errorf("missing int * argument type");
    return 0;
}

inline string parse_spec_str(Context *context, Device *dev)
{
    if (*context->spec != '\'') {
        return string{context->spec++, 1};
    }
    string s = {
        context->spec + 1,
        context->fmt.end(),
    };
    const char *end = s.find('\'');
    if (end == context->fmt.end()) {
        dev->errorf("unmatched string quote");
    }
    context->spec = end + 1;
    return s.end_at(end);
};
string parse_argument_str(Context *context, Device *dev, auto v, Fmt_String auto argument, auto...)
{
    return argument;
}
string parse_argument_str(Context *context, Device *dev, auto...)
{
    dev->errorf("missing string * argument type");
    return "";
}

void format_next_spec(Context *context, Device *dev, auto v, auto... args)
{
    return format_spec(context, dev, v, args...);
}
void format_next_spec(Context *context, Device *dev, auto v, auto, auto... args)
{
    return format_spec(context, dev, v, args...);
}

void vformat(Context *context, Device *dev, auto v, auto... args)
{
    format(context, dev, v);
    format_argument(context, dev, args...);
}

inline void vformat(Context *context, Device *dev)
{
    dev->errorf("missing argument");
}
inline void format_spec(Context *context, Device *dev)
{
    dev->errorf("missing argument");
}

void format_spec(Context *context, Device *dev, auto v, auto... args)
{
#define Parse(V, Spec_Fn, Argument_Fn)                     \
    if (*context->spec == '*') {                           \
        context->spec++;                                   \
        V = Argument_Fn(context, dev, v, args...);         \
        return format_next_spec(context, dev, v, args...); \
    } else {                                               \
        V = Spec_Fn(context, dev);                         \
        return format_spec(context, dev, v, args...);      \
    }

#define Parse_Int(Int) Parse(Int, parse_spec_int, parse_argument_int)
#define Parse_Str(Str) Parse(Str, parse_spec_str, parse_argument_str)
#define Parse_Char(Char) Parse(Char, parse_spec_char, parse_argument_char)
#define Parse_Next() return format_spec(context, dev, v, args...)

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
        context->spec++;
        context->state = Spec_None;
        Parse_Next();

    default:
        break;
    }

    Assert(context->spec < context->fmt.end());
    auto expect_verbs = [&](string set) {
        expect_token(context, dev, *context->verb, set, "'%c': in '%c'", *context->spec, *context->verb);
    };

    switch (*context->spec) {
    case ' ':
    case '\f':
    case '\n':
    case '\r':
    case '\t':
    case '\v':
        context->spec++;
        Parse_Next();

    case '^':
    case '>':
    case '<':
        context->align = *context->spec++;
        context->state = Spec_Pad;
        Parse_Next();

    case '#':
        context->spec++;
        context->hash = true;
        Parse_Next();

    case '-':
        context->spec++;
        context->sign_mode |= (Sign_Negative);
        Parse_Next();

    case '+':
        context->spec++;
        if (context->sign_mode & Sign_Positive_With_Space)
            dev->errorf("'+': discordant '_' sign specifier");
        context->sign_mode |= (Sign_Negative | Sign_Positive);
        Parse_Next();

    case '_':
        context->spec++;
        if (context->sign_mode & Sign_Positive)
            dev->errorf("'_': discordant '+' sign specifier");
        context->sign_mode |= (Sign_Negative | Sign_Positive_With_Space);
        Parse_Next();

    case 'b':
        expect_verbs("dfr");
        context->spec++;
        switch (*context->verb) {
        case 'd':
        case 'f':
            Parse_Int(context->base);

        case 'r':
            context->state = Spec_Bounds_Open;
            Parse_Next();
        }

    case '[':
        expect_verbs("r");
        context->spec++;
        context->state = Spec_Size_Open;
        Parse_Next();

    case 's':
        expect_verbs("r");
        context->spec++;
        Parse_Str(context->separator);

    case '%': {
        expect_verbs("r");
        Context sub_context = new_context({context->spec, context->fmt.end()});
        parse_sequence(&sub_context, dev);
        context->sub_context = &sub_context;
        context->sub_context->up_context = context;
        return format_spec(&sub_context, dev, v, args...);
    }

    case '!':
        expect_verbs("dfs");
        context->spec++;
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
        expect_verbs("sc");
        context->spec++;
        if (context->upcase)
            dev->errorf("'~': discordant '!' case specifier");
        context->downcase = true;
        Parse_Next();

    case '?':
        expect_verbs("sc");
        context->spec++;
        if (context->escapes < Max_Escapes)
            context->escapes++;
        else
            dev->errorf("cannot escape more than %d", Max_Escapes);
        Parse_Next();

    case ')': {
        const char *spec = context->spec;

        if (context->up_context != NULL) {
            context = context->up_context;
            context->spec = spec + 1;
            Parse_Next();
        } else {
            context->fmt = context->fmt.begin_at(spec + 1);
            return vformat(context, dev, v, args...);
        }
    }

    default:
        dev->errorf("'%c': unknown specifier", *context->spec);
        return vformat(context, dev, v, args...);
    }

    Assert(0);
#undef Parse
#undef Parse_Int
#undef Parse_Str
#undef Parse_Char
#undef Parse_Next
}

inline void format_argument(Context *context, Device *dev)
{
    dev->print(context->fmt);
}

void format_argument(Context *context, Device *dev, auto v, auto... args)
{
    *context = new_context(context->fmt);
    parse_sequence(context, dev);

    if (!context->spec) {
        vformat(context, dev, v, args...);
    } else {
        format_spec(context, dev, v, args...);
    }
}

} // namespace bee::fmt

#endif
