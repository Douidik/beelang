#include <format.hpp>

int main(int argc, char *argv[])
{
    using namespace bee;
    fmt::print("hello %(d:!#b16)\n", 64);
    fmt::print("hello %x\n", 64);
}
