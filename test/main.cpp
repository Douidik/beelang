#include "regex_test.hpp"
#include "format.hpp"

using namespace bee;

int main(int argc, char *argv[])
{
    fmt::print("[Regex]\n");
    regex_string();
    regex_range();
    regex_set();
    regex_sequence();
    regex_plus();
    regex_star();
    regex_quest();
    regex_or();
    regex_wave();
    regex_not();
    regex_dash();
}
