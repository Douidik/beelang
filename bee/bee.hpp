#ifndef BEE_HPP
#define BEE_HPP

#include <concepts>
#include <cstdint>
#include <cstdio>
#include <stdlib.h>

namespace bee
{

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

#define Bee_Debug _DEBUG

template <typename F>
struct Defer_Scope
{
    F function;

    ~Defer_Scope()
    {
        function();
    }
};
template <typename F>
Defer_Scope<F> defer_scope_new(F function)
{
    return Defer_Scope<F>{function};
}

#define Macro_Concat_(X, Y) x##y
#define Macro_Concat(X, Y) Macro_Concat_(X, Y)
#define defer(Code)                                                   \
    auto Macro_Concat(_defer_, __COUNTER__) = defer_scope_new([&]() { \
        Code;                                                         \
    })

[[noreturn]]
void bee_panic(const char *what, const char *file, const char *function, int line, const char *code,
               const char *fmt = "", auto... args)
{
    fprintf(stderr, "%s! (%s:%s):\n %d\t| %s [", what, file, function, line, code);
    fprintf(stderr, fmt, args...);
    fprintf(stderr, "]\n");
    abort();
}

#ifdef Bee_Debug
#define _Assert_Error(Error, ...) Error
#define _Assert_Fmt(Error, ...) #Error __VA_OPT__(, ) __VA_ARGS__
#define Assert(...)                                 \
    [[unlikely]] if (!(_Assert_Error(__VA_ARGS__))) \
    bee_panic("assertion failed", __FILE__, __func__, __LINE__, _Assert_Fmt(__VA_ARGS__))

#else
#define Assert(Boolean, ...) \
    do {                     \
    } while (0)
#endif

#define Abs(X) (X > 0 ? +X : -X)
#define Min(X, Y) (X < Y ? X : Y)
#define Max(X, Y) (X > Y ? X : Y)
#define Array_Size(Array) (sizeof(Array) / sizeof(Array[0]))

template <typename T>
using Decay = std::decay_t<T>;

template <typename T>
using Remove_Ref = std::remove_reference_t<T>;

template <typename X, typename Y>
concept Same_As = std::same_as<X, Y>;

template <typename T, typename... Ts>
concept One_Of = (Same_As<T, Ts> || ...);
    
} // namespace bee

#endif
