#include <bee.hpp>
#include <regex.hpp>

using namespace bee;

int main(int argc, char *argv[])
{
    auto regex = compile_regex("'abc'* 'def'?");
    defer(regex.deinit());

    fmt::print("%v\n", regex);
}
