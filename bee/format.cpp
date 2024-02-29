#include "format.hpp"

namespace bee::fmt
{

char *write_itoa(char *it, string alphabet, i32 base, u64 x)
{
    char *end = it + 1;

    for (auto n = Abs(x) / base; n >= base; end++)
        n /= base;
    for (char *w = end; w != it - 1; w--) {
	*w = alphabet[size_t(x % base)];
	x /= base;
    }

    return end;
}

Write_Status new_write_status(char *buf, size_t size)
{
    return Write_Status{buf, size, 0};
}

Context new_context(string fmt)
{
    return Context{fmt};
}

string format_char_fn(Context *context, Device *dev, string input, View<char> output, int (*fn)(int))
{
    Assert(output.len >= input.len);
    for (size_t i = 0; i < input.len; i++)
        output[i] = (*fn)(input[i]);
    return string{output.data, output.len};
}

size_t string_escaped_len(Context *context, Device *dev, string s)
{
    size_t len = s.len;
    for (const char &esc : Escape_Sequences)
        len += s.count(esc) * context->escapes;
    return len;
}

string format_escape(Context *context, Device *dev, string input, View<char> output)
{
    Assert(string_escaped_len(context, dev, input) >= output.len);

    char *it = &output[0];
    auto escape = [&](char esc) {
        memset(it, '\\', context->escapes);
        it[context->escapes] = esc;
        it = &it[context->escapes + 1];
    };

    for (size_t i = 0; i < input.len; i++) {
        switch (input[i]) {
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
            *it++ = input[i];
        }
    }

    return string{output.data, output.len};
}

string parse_sequence(Context *context, Device *dev)
{
    const char *modulo = NULL;
    while (!modulo) {
        const char *match = context->fmt.find('%');
        bool has_verb = match + 1 < context->fmt.end();

        if (!has_verb) {
            dev->errorf("expected verb after '%'");
            context->fmt = {context->fmt.end(), 1};
            return "";
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
    if (modulo >= context->fmt.end()) {
        dev->warnf("unused argument");
        return "";
    }

    string sequence = {&modulo[1], context->fmt.end()};
    if (modulo[1] != '(') {
        sequence.len = 1;
        context->verb = &sequence[0];
        context->fmt = context->fmt.begin_at(context->verb + 1);
        context->spec = NULL;
    } else {
        context->verb = &sequence[1];
        context->spec = sequence.find(":");
        if (context->spec != sequence.end()) {
            context->spec++;
        } else {
            context->spec = &context->verb[1];
        }
    }

    switch (*context->verb) {
    case 'p':
        context->verb = "d";
        context->hash = true;
        context->base = 16;
        context->base_upcase = false;
        break;

    case 'x':
    case 'X':
    case 'b':
    case 'B':
        context->verb = "d";
        context->hash = false;
        context->base_upcase = isupper(*context->verb);
        switch (*context->verb) {
        case 'x':
        case 'X':
            context->base = 16;
            break;
        case 'b':
        case 'B':
            context->base = 2;
            break;
        }
        break;
    }

    return sequence;
}

} // namespace bee::fmt
