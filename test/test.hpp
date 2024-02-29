#ifndef BEE_TEST_HPP
#define BEE_TEST_HPP

#include "bee.hpp"
#include "ds.hpp"
#include "format.hpp"
#include <time.h>

namespace bee
{

static struct
{
    string name;
    struct timespec start;
} Current_Test = {"No test", 0};

#define Expect(X) Assert(X)
#define Expect_Eq(X, Y, ...) Expect(X == Y)
#define Expect_Not_Eq(X, Y, ...) Expect(X != Y)

static void when_test_begin()
{
    fmt::print("case! %(s:< 16) :: ", Current_Test.name);
    fflush(stdout);
    timespec_get(&Current_Test.start, TIME_UTC);
}

static void when_test_succeed()
{
    struct timespec start = Current_Test.start;
    struct timespec end = {};
    timespec_get(&end, TIME_UTC);

    u64 sec = end.tv_sec - start.tv_sec;
    u64 nsec = end.tv_nsec - start.tv_nsec;
    string unit = "?";
    u64 elapsed = (u64)-1;

    if (sec > 0)
        unit = "s", elapsed = sec + nsec / 1e9;
    else if (nsec > 1e6)
        unit = "ms", elapsed = sec * 1e3 + nsec / 1e6;
    else if (nsec > 1e3)
        unit = "us", elapsed = sec * 1e6 + nsec / 1e3;
    else
        unit = "ns", elapsed = sec * 1e9 + nsec;

    fmt::print("success! (in %d%s)\n", elapsed, unit);
}

#define Test(Name)            \
    Current_Test.name = Name; \
    when_test_begin();        \
    defer(when_test_succeed())

} // namespace bee

#endif
