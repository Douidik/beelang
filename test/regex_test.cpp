#include "regex_test.hpp"
#include "regex.hpp"
#include "test.hpp"

namespace bee
{
using namespace regex;

#define Lorem_Ipsum_ \
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut \
    labore et dolore magna aliqua. Id porta nibh venenatis cras sed felis eget velit. Viverra tellus  \
    in hac habitasse. Sed risus pretium quam vulputate dignissim suspendisse in est. In eu mi \
    bibendum neque egestas congue quisque egestas. Mi proin sed libero enim sed faucibus turpis in. \
    Aliquam vestibulum morbi blandit cursus. Tellus in hac habitasse platea dictumst vestibulum. \
    Massa ultricies mi quis hendrerit. Molestie a iaculis at erat pellentesque adipiscing commodo. \
    Vulputate eu scelerisque felis imperdiet proin fermentum. Vitae congue eu consequat ac felis. Nec \
    ultrices dui sapien eget mi proin sed. Nunc mattis enim ut tellus elementum sagittis vitae et. \
    Mauris ultrices eros in cursus turpis massa tincidunt dui ut. Nisi porta lorem mollis aliquam ut \
    porttitor leo a diam. Diam phasellus vestibulum lorem sed risus ultricies. Arcu vitae elementum \
    curabitur vitae nunc sed velit dignissim. Ut eu sem integer vitae justo eget magna fermentum \
    iaculis.In eu mi bibendum neque."

const string Lorem_Ipsum = Lorem_Ipsum_;
const string Lorem_Ipsum_Quoted = "'" Lorem_Ipsum_ "'";

Match regex_match(string source, string expr)
{
    Regex regex = compile_regex(source);
    defer(regex.deinit());

    return regex.match(expr);
}

#define Match_Ok(source, expr) Expect(regex_match(source, expr).ok)
#define Match_Npos(source, expr) Expect(!regex_match(source, expr).ok)
#define Match(source, expr) Expect_Eq(regex_match(source, expr).view, expr)
#define Match_Eq(source, expr, eq) Expect_Eq(regex_match(source, expr).view, eq)

void regex_string()
{
    Test("string");

    Match("'abc'", "abc");
    Match_Ok("'abc'", "abcccccccccc");
    Match("'hello' ' ' 'world'", "hello world");
    Match("'hello\nworld'", "hello\nworld");
    Match(Lorem_Ipsum_Quoted, Lorem_Ipsum);

    Match("`abc`", "abc");
    Match_Ok("`abc`", "abcccccccccc");
    Match("`hello` ` ` `world`", "hello world");
    Match("`hello\nworld`", "hello\nworld");

    Match_Npos("'cba'", "abc");
    Match_Npos("'cbaa'", "abcc");
    Match_Npos(Lorem_Ipsum_Quoted, Lorem_Ipsum.substr(1, Lorem_Ipsum.len - 2));
}

void regex_range()
{
    Test("range");

    Match("[0-9]+", "0123456789");
    Match("[a-f]+", "abcedef");
    Match("[a-a]+", "aaaaaaaaaaaa");
    Match("[[-]]+", "[\\\\\\\\\\]");
    Match("[---]+", "------------");

    Match_Npos("[a-z]", "`");
    Match_Npos("[a-z]", "{");
}

void regex_set()
{
    Test("set");

    Match("a", "a");
    Match("o", "+");
    Match("n", "7");
    Match("Q", "\"");
    Match("q", "'");

    Match_Npos("_", "b");
    Match_Npos("a", "4");
    Match_Npos("o", "\t");
    Match_Npos("n", "|");
    Match_Npos("Q", "^");
    Match_Npos("q", "&");
}

void regex_sequence()
{
    Test("sequence");

    Match("{'abc'}", "abc");
    Match("{'ab'} {'c'}", "abc");
    Match("{{{{{{'ab'} {'c'}}}}}}", "abc");
}

void regex_plus()
{
    Test("plus");

    Match("{'abc'}+", "abcabcabc");
    Match("{'ab'n}+", "ab1ab2ab3");
    Match("n+n+", "12");
}

void regex_star()
{
    Test("star");

    Match("{'abc'}*", "abc");
    Match("{'abc'}*", "");
    Match("{'ab'n}*", "ab1ab2ab3");
    Match("{{{'hello'}}}*", "");
    Match("{{{'hello'}}}*", "hellohellohello");
}

void regex_quest()
{
    Test("star");

    Match("{'abc'}?", "abc");
    Match("{'abc'}?", "");
    Match("{'ab'n}?", "ab1");
    Match("{{{'hello'}}}?", "");
    Match("{{{'hello'}}}?", "hello");
}

void regex_or()
{
    Test("or");

    Match("{'a'|'b'}", "a");
    Match("{'a'|'b'}", "a");
    Match("{'a' | 'b'}", "a");
    Match("{'a' | 'b'}", "b");
    Match("a{a|'_'|n}*", "snake_case_variable123");
}

void regex_wave()
{
    Test("wave");

    Match("^~'c'", "abc");
    Match("a~'z'", "ahjklz");
    Match_Eq("'//' {a|' '} ~ '//'", "// The program starts here // int main() {",
             "// The program starts here //");
    Match("n ~ {'z'|'9'}", "0123456789");
    Match("n ~ {'z'|'9'}", "012345678z");
    Match_Ok("{' '} ~ 'sus'", "                           sus               ");
    Match_Npos("{' '} ~ 'sus'", "            |             sus               ");
}

void regex_not()
{
    Test("not");

    Match_Npos("'abc' !'d'", "abcd");
    Match_Eq("'abc' !'d'", "abc_", "abc_");
    Match_Eq("{!'\n'}*", "lorem ipsum\n hello", "lorem ipsum");
}

void regex_dash()
{
    Test("dash");

    Match_Eq("'abc'/'d'", "abcd", "abc");
    Match_Eq("^~/_", "words words", "words");
}

} // namespace bee
