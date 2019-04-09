// THIS HEADER IS FOR THE GMTK GAME JAM

#pragma once

// #define SDL_MAIN_HANDLED
#include "SDL2.h"

#define NG_NO_EXTERNS
#define NG_UNITY_BUILD

#ifndef null
#define null nullptr
#endif
#ifndef cast
#define cast(...) (__VA_ARGS__) // variadic so templates work
#endif

#define NG_CAT_(x, y) x##y
#define NG_CAT(x, y) NG_CAT_(x, y)
#define NG_ANON(x) NG_CAT(x, __LINE__) // anonymous variable
#define NG_STRINGIZE_(x) #x
#define NG_STRINGIZE(x) NG_STRINGIZE_(x)

#ifdef NG_NO_FORCE_INLINE
#define NG_INLINE inline
#else
#if defined(__GNUC__) || defined(__clang__)
#define NG_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define NG_INLINE inline __forceinline
#else
#define NG_INLINE inline
#endif // platform
#endif // NG_NEVER_FORCE_INLINE
#ifndef force_inline
#define force_inline NG_INLINE
#endif
#if defined(__GNUC__) || defined(__clang__)
#define NG_LAMBDA_INLINE __attribute__((always_inline))
// #define NG_LAMBDAINLINE
#else
#define NG_LAMBDA_INLINE // MSVC doesn't support __forceinline on lambdas yet
#endif
#ifdef NG_UNITY_BUILD
#ifndef NG_DEFINE
#define NG_DEFINE
#endif // NG_DEFINE
#if 0//def __COUNTER__
#define NG_UNITY_BUILD_BEGIN_(COUNTER)                                         \
    namespace ng {                                                             \
    NG_INLINE void run_global_code();                                          \
    }                                                                          \
    template <int N> struct ng_global_runner {                                 \
        NG_INLINE ng_global_runner() { ng_global_fn<(N - 1)>({}); }            \
    };                                                                         \
    template <int N> void ng_global_fn(ng_global_runner<N>) {}                 \
    template <> struct ng_global_runner<(COUNTER)> {                           \
        NG_INLINE ng_global_runner() {}                                        \
    };                                                                         \
    template <> void ng_global_fn<(COUNTER)>(ng_global_runner<(COUNTER)>) {}
#define NG_UNITY_BUILD_BEGIN() NG_UNITY_BUILD_BEGIN_(__COUNTER__)
#define ng_global_(N)                                                          \
    template <> NG_INLINE void ng_global_fn<N>(ng_global_runner<N>)
#define ng_global ng_global_(__COUNTER__)
#define ng_construct(x, ...) (new (&(x)) decltype(x) __VA_ARGS__)
// Usage: auto ng_init(x, {new int[100]});
// RAIIClass ng_init(y, {}); // gets default-constructed with no CRT static init
#define ng_init(x, ...)                                                        \
    x __VA_ARGS__;                                                             \
    ng_global {                                                                \
        if (!ng::has_static_init) {                                            \
            ng_construct(x, __VA_ARGS__);                                      \
        }                                                                      \
    }
#define NG_UNITY_BUILD_END()                                                   \
    NG_INLINE void ng::run_global_code() {                                     \
        ng_global_fn<(__COUNTER__ - 1)>({});                                   \
    }
#endif // __COUNTER__
#endif // NG_UNITY_BUILD
///////////////////////////////// Assert macro /////////////////////////////////
#if defined(__GNUC__) || defined(__clang__)
#define ng_break() __asm__ __volatile__("int $3")
#elif defined(_MSC_VER)
#define ng_break() __debugbreak(); // __asm { int 3}
#else                              // we tried
#define ng_break()                                                             \
    do {                                                                       \
        (*(int *)0 = 0);                                                       \
    } while (1)
#endif // platform
#if __STDC_VERSION__ >= 199901L || defined(__clang__)
#define NG_FUNCTION __func__
#define NG_FNPARENL "("
#define NG_FNPARENR ")"
#elif __GNUC__ >= 2 || defined(_MSC_VER)
#define NG_FUNCTION __FUNCTION__
#define NG_FNPARENL "("
#define NG_FNPARENR ")"
#else
#define NG_FUNCTION ""
#define NG_FNPARENL ""
#define NG_FNPARENR ""
#endif // platform
#ifndef NDEBUG
#define ng_assert(e, ...)                                                      \
    do {                                                                       \
        if (!(e)) {                                                            \
            ng::assert_failed(__FILE__ ":" NG_STRINGIZE(__LINE__) NG_FNPARENL, \
                              NG_FUNCTION,                                     \
                              NG_FNPARENR ": '" #e,                            \
                              "'\n\t" __VA_ARGS__);                            \
            ng_break();                                                        \
        }                                                                      \
    } while (0)
#else
#define ng_assert(e, ...)                                                      \
    if ((e)) {                                                                 \
    } else                                                                     \
        (void)("" __VA_ARGS__, 0)
#endif // NDEBUG
#define ng_foreach(...) for (auto __VA_ARGS__)

#if defined(__GNUC__) || defined(__clang__)
#define NG_SILENCE_UNUSED(...)                                                 \
    _Pragma("GCC diagnostic push")                                             \
        _Pragma("GCC diagnostic ignored \"-Wunused-variable\"")                \
            __VA_ARGS__ _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
#define NG_SILENCE_UNUSED(...)                                                 \
    __pragma("warning(push)") __pragma("warning(disable : 4101)")              \
        __VA_ARGS__ __pragma("warning(pop)")
#else
#define NG_SILENCE_UNUSED(...) __VA_ARGS__
#endif // platform

#define ng_for(...) NG_SILENCE_UNUSED(for (auto &&it : __VA_ARGS__))

#ifndef foreach
#define foreach ng_foreach
#endif
namespace ng {
constexpr struct defer_dummy_t {
} defer_dummy;
template <class F> struct deferrer {
    F f;
    NG_INLINE ~deferrer() { f(); }
};
template <class F> NG_INLINE deferrer<F> operator*(defer_dummy_t, F f) {
    return {f};
}
#define ng_defer auto NG_ANON(_defer) = ng::defer_dummy *[&]() NG_LAMBDA_INLINE
#ifndef defer
#define defer ng_defer
#endif

namespace int_types {
#if defined(__INT32_TYPE__)
using s8 = __INT8_TYPE__;
using u8 = __UINT8_TYPE__;
using s16 = __INT16_TYPE__;
using u16 = __UINT16_TYPE__;
using s32 = __INT32_TYPE__;
using u32 = __UINT32_TYPE__;
using s64 = __INT64_TYPE__;
using u64 = __UINT64_TYPE__;
using usize = __SIZE_TYPE__;
using uptr = __UINTPTR_TYPE__;
#elif defined(_MSC_VER)
using s8 = signed __int8;
using u8 = unsigned __int8;
using s16 = signed __int16;
using u16 = unsigned __int16;
using s32 = signed __int32;
using u32 = unsigned __int32;
using s64 = signed __int64;
using u64 = unsigned __int64;
#ifdef _WIN64
using usize = unsigned __int64;
using uptr = unsigned __int64;
#else
using usize = unsigned __int32;
using uptr = unsigned __int32;
#endif // _WIN64
#else
using s8 = signed char;
using u8 = unsigned char;
using s16 = signed short;
using u16 = unsigned short;
using s32 = signed int;
using u32 = unsigned int;
using s64 = signed long long;
using u64 = unsigned long long;
using usize = decltype(sizeof(void *));
using uptr = usize;
#endif // platform
using f32 = float;
using f64 = double;
using f128 = long double;
static_assert(sizeof(s32) == 4, "s32 not 32-bit");
static_assert(sizeof(s64) == 8, "s64 not 64-bit");
static_assert(sizeof(usize) == sizeof(void *), "usize not pointer size");
static_assert(sizeof(uptr) == sizeof(void *), "uptr not pointer size");
} // namespace int_types
using namespace int_types;
extern bool has_static_init; // True if the CRT initialized globals at startup.
template <class T, usize N> constexpr auto countof(T (&)[N]) { return N; }
#define ng_size_of(...) (static_cast<ng::s64>(sizeof(__VA_ARGS__)))
#define ng_count_of(...) (static_cast<ng::s64>(ng::countof(__VA_ARGS__)))
#ifndef size_of
#define size_of ng_size_of
#endif // size_of
#ifndef count_of
#define count_of ng_count_of
#endif // count_of
#ifndef NG_SIMPLE_BITCAST
// This bitcast avoids strict aliasing with 0 runtime overhead. The macro casts
// the RHS to this type, using unions to type-pun.
template <class T> struct bitcaster {
    union {
        T to;
    };
    template <class U> NG_INLINE bitcaster(const U &from) {
        union {
            U from;
            T to;
        } converter{from};
        to = static_cast<T &&>(converter.to);
    }
    NG_INLINE T &&operator~() { return static_cast<T &&>(to); }
};
// References cannot be bitcast in a standard way.
template <class T> struct bitcaster<T &> {};
template <class T> struct bitcaster<T &&> {};
#define ng_bitcast(...) ~(ng::bitcaster<__VA_ARGS__>)
#else
#define ng_bitcast(...) *(__VA_ARGS__ *)&
#endif // NG_SIMPLE_BITCAST
#ifndef bitcast
#define bitcast ng_bitcast
#endif
// This is the maximally general swap macro.
#define ng_swap(a, b)                                                          \
    {                                                                          \
        auto &&a_{(a)};                                                        \
        auto &&b_{(b)};                                                        \
        auto c{a_};                                                            \
        a_ = b_;                                                               \
        b_ = c;                                                                \
    }
#define ng_min(a, b) ((a) < (b) ? (a) : (b))
#define ng_max(a, b) ((a) > (b) ? (a) : (b))
#define ng_clamp(t, min, max) ((t) > (min) ? (t) < (max) ? (t) : (max) : (min))
#define ng_lerp(a, b, t) ((a) * (1 - (t)) + (b) * (t))
#define ng_abs(x) ((x) > 0 ? (x) : -(x))
template <class T> void swap(T &a, T &b) { ng_swap(a, b); }
template <class T> NG_INLINE auto(min)(T a, T b) { return ng_min(a, b); }
template <class T> NG_INLINE auto(max)(T a, T b) { return ng_max(a, b); }
template <class T> T clamp(T t, T minimum, T maximum) {
    return ng_clamp(t, minimum, maximum);
}
template <class T, class U> NG_INLINE auto lerp(T a, U b, float t) {
    return ng_lerp(a, b, t);
}
NG_INLINE s8 abs(s8 x) { return ng_abs(x); }
NG_INLINE s16 abs(s16 x) { return ng_abs(x); }
NG_INLINE s32 abs(s32 x) { return ng_abs(x); }
NG_INLINE s64 abs(s64 x) { return ng_abs(x); }

u32 f32bits(f32 f) { return ng_bitcast(u32) f; }
u64 f64bits(f64 f) { return ng_bitcast(u64) f; }

bool isnan(f32 f) { return f != f; } // @Robustness: Float optimizations break
bool isnan(f64 f) { return f != f; } // the self-equality method.
bool isinf(f32 f) { return !isnan(f) && isnan(f - f); }
bool isinf(f64 f) { return !isnan(f) && isnan(f - f); }

inline double round(double value) {
    if (ng::isnan(value) || SDL_fabs(value) > 4503599627370495.5) {
        return value; // bigger than biggest fractional double
    }
    return (s64)(value > 0 ? value + 0.5 : value - 0.5);
}

inline float round(float value) {
    if (ng::isnan(value) || SDL_fabs(value) > 8388607.5f) {
        return value; // bigger than biggest fractional float
    }
    return (s64)(value > 0 ? value + 0.5f : value - 0.5f);
}

constexpr static const f32 TAU32 = 6.28318530717958647692528676655900576839434f;
constexpr static const f64 TAU64 = 6.283185307179586476925286766559005768394338;
constexpr static const f64 EULERS32 = 2.71828182845904523536028747135266249776f;
constexpr static const f64 EULERS64 = 2.718281828459045235360287471352662497757;
void exit(int code);
void *malloc(usize size);
void *realloc(void *ptr, usize new_size);
void free(void *ptr);

} // namespace ng
#ifndef NG_NO_PLACEMENT_NEW_INLINE
#ifdef _WIN32
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
#define NG_DO_PLACE_NEW
#endif // __PLACEMENT_NEW_INLINE
#ifndef __PLACEMENT_VEC_NEW_INLINE
#define __PLACEMENT_VEC_NEW_INLINE
#define NG_DO_PLACE_NEW_ARR
#endif // __PLACEMENT_VEC_NEW_INLINE
#else
#define NG_DO_PLACE_NEW
#define NG_DO_PLACE_NEW_ARR
#endif // platform
#ifdef NG_DO_PLACE_NEW
NG_INLINE void *operator new(ng::usize, void *location) { return location; }
NG_INLINE void operator delete(void *, void *) {}
#endif // NG_DO_PLACE_NEW
#ifdef NG_DO_PLACE_NEW_ARR
void *operator new[](ng::usize, void *) = delete;
void operator delete[](void *, void *) = delete;
#endif // NG_DO_PLACE_NEW_ARR
#undef NG_DO_PLACE_NEW
#undef NG_DO_PLACE_NEW_ARR
#endif // NG_NO_PLACEMENT_NEW_INLINE
#ifdef NG_DEFINE
// // The compiler will generate code to run this function with the other static
// // initializers for global variables. This function cannot be inlined, so the
// // variable will have the default BSS value, 0, until the CRT runs the static
// // initializers. If there is no CRT (like when compiling with cl.exe
// // -nodefaultlib) then it will stay 0.
// static bool run_static_init() {
//     volatile bool result = true;
//     return result;
// }
// bool ng::has_static_init = run_static_init();

#if defined(_MSC_VER)
#define NG_CDECL __cdecl
#define NG_RESTRICT __declspec(restrict)
#define NG_NORETURN __declspec(noreturn)
#if (defined(_M_IX86) || defined(_M_IA64) || defined(_M_AMD64) ||              \
     defined(_M_ARM)) &&                                                       \
    !defined(MIDL_PASS)
#define NG_DECLSPEC __declspec(dllimport)
#else
#define NG_DECLSPEC
#endif
// #elif defined(__GNUC__) || defined(__clang__)
#else
#define NG_CDECL
#define NG_DECLSPEC
#define NG_RESTRICT
#endif // platform
#ifdef _WIN32
#pragma comment(lib, "kernel32.lib")
extern "C" {
NG_DECLSPEC void *__stdcall HeapAlloc(void *hHeap, ng::u32 dwFlags,
                                      ng::usize dwBytes);
NG_DECLSPEC void *__stdcall HeapReAlloc(void *hHeap, ng::u32 dwFlags,
                                        void *lpMem, ng::usize dwBytes);
NG_DECLSPEC int __stdcall HeapFree(void *hHeap, ng::u32 dwFlags, void *lpMem);
NG_DECLSPEC void *__stdcall GetProcessHeap();
NG_DECLSPEC __declspec(noreturn) void __stdcall ExitProcess(int status);
}
namespace ng {
void *malloc(usize size) { return HeapAlloc(GetProcessHeap(), 0, size); }
void *realloc(void *ptr, usize new_size) {
    if (!ptr) {
        return HeapAlloc(GetProcessHeap(), 0, new_size);
    }
    if (!new_size) {
        HeapFree(GetProcessHeap(), 0, ptr);
        return nullptr;
    }
    return HeapReAlloc(GetProcessHeap(), 0, ptr, new_size);
}
void free(void *ptr) { HeapFree(GetProcessHeap(), 0, ptr); }
void exit(int code) { ExitProcess(code); }
} // namespace ng
#else
#error This platform is not yet supported without the C Runtime.
#endif
#endif // NG_DEFINE
namespace ng {
struct mt19937_64 {
    enum { n = 312, default_seed = 5489 };
    u64 index = 0;
    u64 data[n];
    mt19937_64(u64 seed = default_seed) { this->seed(seed); }
    void seed(u64 seed = default_seed);
    u64 next_u64();
    // yields a float in [0, 1)
    double next_double() { return (next_u64() >> 11) * (1.0 / (1ll << 53)); }
    // yields a float in [0, 1)
    NG_INLINE float next_float() { return next_double(); }
};
struct mt127_64 { // https://github.com/MersenneTwister-Lab/TinyMT
    u64 data[2];
    u32 mat1 = 0;
    u32 mat2 = 0;
    u64 tmat = 0;
    mt127_64(u64 seed = 5489) { this->seed(seed); }
    void seed(u64 seed = 5489);
    void advance();
    u64 temper();

    u64 next_u64();
    // yields a float in [0, 1)
    double next_double() { return (next_u64() >> 11) * (1.0 / (1ll << 53)); }
    // yields a float in [0, 1)
    NG_INLINE float next_float() { return next_double(); }
};
#ifdef NG_DEFINE
namespace mt19937_constants {
enum the_constants : u64 {
    w = 64,
    n = 312,
    m = 156,
    r = 31,
    a = 0xB5026F5AA96619E9,
    u = 29,
    d = 0x5555555555555555,
    s = 17,
    b = 0x71D67FFFEDA60000,
    t = 37,
    c = 0xFFF7EEE000000000,
    l = 43,
    f = 6364136223846793005,
    lower_mask = 0x7FFFFFFF,
    upper_mask = ~lower_mask,
};
}
void mt19937_64::seed(u64 seed) {
    using namespace mt19937_constants;
    index = n + 1;
    data[0] = seed;
    for (u64 i = 1; i < n; ++i) {
        data[i] = (f * (data[i - 1] ^ (data[i - 1] >> (w - 2))) + i);
    }
}
u64 mt19937_64::next_u64() {
    using namespace mt19937_constants;
    if (index == 0 || index > n + 1) {
        // uninitialized
        seed();
    }
    if (index > n) {
        // do the twist
        for (u64 i = 0; i < n; ++i) {
            u64 x = (data[i] & upper_mask) + (data[(i + 1) % n] & lower_mask);
            u64 xA = x >> 1;
            if (x % 2 != 0) {
                xA ^= a;
            }
            data[i] = data[(i + m) % n] ^ xA;
        }
        index = 1;
    }
    u64 y = data[index - 1];
    y = y ^ ((y >> u) & d);
    y = y ^ ((y << s) & b);
    y = y ^ ((y << t) & c);
    y = y ^ (y >> l);
    index += 1;
    return y;
}
void mt127_64::seed(u64 seed) {
    data[0] = seed ^ ((u64)mat1 << 32);
    data[1] = mat2 ^ tmat;
    for (int i = 1; i < 8; i++) {
        data[i & 1] ^= i + 6364136223846793005ull *
                               (data[(i - 1) & 1] ^ (data[(i - 1) & 1] >> 62));
    }
    // period_certification()
    if ((data[0] & 0x7fffffffffffffff) == 0 && data[1] == 0) {
        data[0] = 'T';
        data[1] = 'M';
    }
}
void mt127_64::advance() {
    data[0] &= 0x7fffffffffffffff;
    u64 x = data[0] ^ data[1];
    x ^= x << 12;
    x ^= x >> 32;
    x ^= x << 32;
    x ^= x << 11;
    data[0] = data[1];
    data[1] = x;
    data[0] ^= -((s64)(x & 1)) & mat1;
    data[1] ^= -((s64)(x & 1)) & (((u64)mat2) << 32);
}
u64 mt127_64::temper() {
    u64 x = data[0] ^ data[1];
    x ^= data[0] >> 8;
    x ^= -((s64)(x & 1)) & tmat;
    return x;
}

u64 mt127_64::next_u64() {
    advance();
    return temper();
}
#endif // NG_DEFINE
} // namespace ng
namespace ng {
// print functions
struct print_buffer {
    u8 *str = nullptr;
    u64 len = 0;
    u64 cap = 0;
    void putchar(u32 c);
    void put_u8s(u8 *u8s, u64 count);
    void ensure_space(u64 bytes_left);
};
using print_buffer_proc = void(print_buffer);
extern print_buffer_proc *output_print_buffer;
struct fmt_int {
    s64 x = 0;
    bool is_signed = false;
    int leading_zeroes = 0;
    int leading_spaces = 0;
    int radix = 10;
    fmt_int(s64 x, bool is_signed = true, int radix = 10,
            int leading_zeroes = 0, int leading_spaces = 0);
};
struct fmt_char {
    u32 c;
    fmt_char(u32 x) { c = x; }
};
struct fmt_float {
    f64 x;
    int num_chars;
    fmt_float(f64 x, int num_chars);
};
void print_init();
void print_item(print_buffer *buf, print_buffer src);
void print_item(print_buffer *buf, bool b);
void print_item(print_buffer *buf, s8 i);
void print_item(print_buffer *buf, u8 u);
void print_item(print_buffer *buf, s16 i);
void print_item(print_buffer *buf, u16 u);
void print_item(print_buffer *buf, s32 i);
void print_item(print_buffer *buf, u32 u);
void print_item(print_buffer *buf, s64 i);
void print_item(print_buffer *buf, u64 u);
void print_item(print_buffer *buf, f32 f);
void print_item(print_buffer *buf, f64 f);
void print_item(print_buffer *buf, const fmt_int &fmt);
void print_item(print_buffer *buf, const fmt_float &fmt);
void print_item(print_buffer *buf, const fmt_char &fmt);
void print_item(print_buffer *buf, const char *ptr);
void print_item(print_buffer *buf, void *ptr);
u64 print_get_item_size(print_buffer src);
u64 print_get_item_size(bool b);
u64 print_get_item_size(s8 i);
u64 print_get_item_size(u8 u);
u64 print_get_item_size(s16 i);
u64 print_get_item_size(u16 u);
u64 print_get_item_size(s32 i);
u64 print_get_item_size(u32 u);
u64 print_get_item_size(s64 i);
u64 print_get_item_size(u64 u);
// u64 print_get_item_size(f32 f);
// u64 print_get_item_size(f64 f);
u64 print_get_item_size(const fmt_int &f);
u64 print_get_item_size(const fmt_float &f);
u64 print_get_item_size(const fmt_char &f);
u64 print_get_item_size(const char *ptr);
u64 print_get_item_size(void *ptr);
bool str_to_s64(const char **str, s64 *result);
template <class Arg, class... Args>
NG_INLINE u64 print_get_size(const Arg &arg, const Args &... args) {
    return print_get_item_size(arg) + print_get_size(args...);
}
NG_INLINE u64 print_get_size() { return 0; }
template <class T, class... Ts>
void sprint(print_buffer *buf, const char *s, const T &t, const Ts &... ts) {
    while (*s) {
        if (*s == '%') {
            // if (s[1] == '%') {
            //     s += 1;
            // } else {
            print_item(buf, t);
            return sprint(buf, s + 1, ts...);
            // }
        }
        buf->putchar(*s);
        s += 1;
    }
}
NG_INLINE void sprint(print_buffer *buf, const char *s) { print_item(buf, s); }
// Free the buffer returned by this using ng::free(buffer.str)
template <class... Args>
print_buffer mprint(const char *s, const Args &... args) {
    print_buffer result = {};
    result.cap = print_get_size(s, args...);
    result.str = (u8 *)SDL_malloc(result.cap);
    if (result.str) {
        sprint(&result, s, args...);
    }
    return result;
}
template <class... Args> void print(const char *s, const Args &... args) {
    if (!output_print_buffer) {
        return;
    }
    u8 stack_buffer[1024];
    print_buffer buffer = {};
    buffer.cap = print_get_size(s, args...);
    buffer.str = stack_buffer;
    bool allocate = (buffer.cap >= countof(stack_buffer));
    if (allocate) {
        buffer.str = (u8 *)SDL_malloc(buffer.cap);
        if (!buffer.str) {
            return;
        }
    }
    sprint(&buffer, s, args...);
    output_print_buffer(buffer);
    if (allocate) {
        SDL_free(buffer.str);
    }
}

#ifdef NG_DEFINE
bool str_to_s64(const char **str, s64 *result) {
    if (!str || !*str || !result) {
        return false;
    }
    s64 number = 0;
    bool negative = false;
    if (**str == '-') {
        negative = true;
        *str += 1;
    }
    bool is_number = false;
    for (; **str >= '0' && **str <= '9'; *str += 1) {
        is_number = true;
        number = (number * 10) + (**str - '0');
        if (number < 0) {
            return false; // overflowed: too many digits
        }
    }
    if (negative) {
        number = -number;
    }
    if (is_number) {
        *result = number;
    }
    return is_number;
}
void ng::print_buffer::putchar(u32 c) { // @Unicode: should take u32
#ifndef NDEBUG
    if (len + 1 > cap) { // Can't call assert because it uses print
        ng_break();
    }
#endif // NDEBUG
    str[len] = c;
    len += 1;
}
void ng::print_buffer::put_u8s(u8 *u8s, u64 count) {
#ifndef NDEBUG
    if (len + count > cap) {
        ng_break();
    }
#endif // NDEBUG
    SDL_memcpy(str + len, u8s, cast(ng::usize) count);
    len += count;
}
void ng::print_buffer::ensure_space(u64 count) {
#ifndef NDEBUG
    if (len + count > cap) {
        ng_break();
    }
#endif // NDEBUG
}
} // namespace ng
#ifdef NG_NO_EXTERNS
#ifdef _WIN32
#pragma comment(lib, "kernel32.lib")
extern "C" {
NG_DECLSPEC int __stdcall WriteFile(void *hFile, const void *lpBuffer,
                                    ng::u32 nNumberOfBytesToWrite,
                                    ng::u32 *lpNumberOfBytesWritten,
                                    struct _OVERLAPPED *lpOverlapped);
NG_DECLSPEC void *__stdcall GetStdHandle(ng::u32 nStdHandle);
}
namespace ng {
static void write_print_buffer_default(print_buffer buffer) {
    u32 bytes_written;
    u32 size_to_write = (u32)((ng::min)(buffer.len, buffer.cap));
    WriteFile(GetStdHandle((u32)-11),
              buffer.str,
              size_to_write,
              &bytes_written,
              nullptr);
}
} // namespace ng
#else
#error This platform is not yet supported without the C Runtime.
#endif // platform
#else
static void write_print_buffer_default(ng::print_buffer buffer) {
    ::printf("%.*s", (int)((ng::min)(buffer.len, buffer.cap)), buffer.str);
}
#endif // NG_NO_EXTERNS
namespace ng {
print_buffer_proc *output_print_buffer = &write_print_buffer_default;
void print_init() { output_print_buffer = &write_print_buffer_default; }
fmt_int::fmt_int(s64 x, bool is_signed, int radix, int leading_zeroes,
                 int leading_spaces) {
    this->x = x;
    this->is_signed = is_signed;
    this->leading_zeroes = leading_zeroes;
    this->leading_spaces = leading_spaces;
    this->radix = radix;
}
u64 print_get_item_size(const fmt_int &fmt) {
    if (fmt.radix < 2 || fmt.radix > 64) {
        return 0;
    }
    // floor(log_b(2^64)) for b = 2 to 64
    static const int log_table[] = {
        64, 40, 32, 27, 24, 22, 21, 20, 19, 18, 17, 17, 16, 16, 16, 15,
        15, 15, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 13, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 11, 11, 11, 11, 11, 11, 11, 11, 11,
        11, 11, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10, 10, 10};
    auto max_integer_size = log_table[fmt.radix - 2];
    auto max_extras_size = fmt.leading_spaces + fmt.leading_zeroes;
    return (max_integer_size + max_extras_size);
}
void print_item(print_buffer *buf, const fmt_int &fmt) {
    auto radix = fmt.radix;
    if (radix < 2 || radix > 64) {
        return;
    }
    auto x = fmt.x;
    if (x == 0) {
        buf->putchar('0');
        return;
    }
    if (x < 0 && fmt.is_signed) {
        buf->putchar('-');
        x = -x;
    }
    auto ux = (u64)x;
    char num_buffer[64];
    const auto end = &num_buffer[countof(num_buffer)];
    auto p = end - 1;
    bool has_printed_nonzeroes = false;
    do {
        int digit = ux % radix;
        if (digit > 0 || has_printed_nonzeroes || p == end - 1) {
            *p = "0123456789"
                 "abcdefghijklmnopqrstuvwxyz"
                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                 "@$"[digit];
            has_printed_nonzeroes = true;
            p -= 1;
        }
    } while (ux /= radix);
    p += 1;
    u64 length = end - p;
    for (int i = fmt.leading_spaces; i > 0; i -= 1) {
        buf->putchar(' ');
    }
    for (int i = fmt.leading_zeroes - length; i > 0; i -= 1) {
        buf->putchar('0');
    }
    for (; p < end; p += 1) {
        buf->putchar(*p);
    }
}
fmt_float::fmt_float(f64 x, int num_chars) {
    this->x = x;
    this->num_chars = num_chars;
}

// yields upper bound
static s64 log10_approx(f64 x) {
    s64 raw_exponent = ng_bitcast(u64) x >> 52 & 0x7ff;
    s64 exponent2 = raw_exponent - 1023;
    f64 exponent10 = exponent2 * 0.3010299956639811952; // log_10(2)
    return (s64)ng::round(exponent10);
}
#ifndef NG_NO_EXTERNS
static f64 pow10(s64 x) {
    if (x > 308) {
        return 1e+300 * 1e+300; // inf
    }
    if (x < -308) {
        return 0;
    }
    return ng::pow(10, (f64)x);
}
#else
static f64 pow10(s64 x) { // @Todo: hardware implementation
#define NG0(x) x##0, x##1, x##2, x##3, x##4, x##5, x##6, x##7, x##8, x##9
#define NG1(x)                                                                 \
    NG0(x##0)                                                                  \
    , NG0(x##1), NG0(x##2), NG0(x##3), NG0(x##4), NG0(x##5), NG0(x##6),        \
        NG0(x##7), NG0(x##8), NG0(x##9)
    constexpr f64 plut[] = {NG1(1e0),
                            NG1(1e1),
                            NG1(1e2),
                            1e300,
                            1e301,
                            1e302,
                            1e303,
                            1e304,
                            1e305,
                            1e306,
                            1e307,
                            1e308};
    constexpr f64 nlut[] = {NG1(1e-0),
                            NG1(1e-1),
                            NG1(1e-2),
                            1e-300,
                            1e-301,
                            1e-302,
                            1e-303,
                            1e-304,
                            1e-305,
                            1e-306,
                            1e-307,
                            1e-308};
#undef NG1
#undef NG0
    if (x > 308) {
        return 1e+300 * 1e+300; // inf
    }
    if (x < -308) {
        return 0;
    }
    if (x < 0) {
        return nlut[-x];
    }
    return plut[x];
}
#endif
void print_item(print_buffer *buf, const fmt_float &fmt) { // @Todo: num_chars
    auto x = fmt.x;
    if (f64bits(x) & f64bits(-0.0)) { // slow vs < 0, but finds -nan and -0
        buf->putchar('-');
        x = -x;
    }
    if (x == 0.0) {
        buf->putchar('0');
        return;
    }
    if ((f64bits(x) >> 52 & 0x7ff) == 0) { // exp == 0, i.e., subnormal
        buf->putchar('('), buf->putchar('s'), buf->putchar('u');
        buf->putchar('b'), buf->putchar(')');
        return;
    }
    if ((f64bits(x) >> 52 & 0x7ff) == 0x7ff) {
        if ((f64bits(x) & ((1ull << 52) - 1ull)) == 0) {
            buf->putchar('i'), buf->putchar('n'), buf->putchar('f');
        } else {
            buf->putchar('n'), buf->putchar('a'), buf->putchar('n');
        }
        return;
    }
    auto log10_x = log10_approx(x);
    if (log10_x > 18) { // bigger than a u64.
        auto first_digit = log10_x + 1;
        auto last_digit = first_digit - 19;
        for (auto i = first_digit; i >= last_digit; i -= 1) {
            auto scale = pow10(-i);
            auto num = x * scale;
            auto digit = static_cast<u64>(num) % 10;
            if (i == first_digit && digit == 0) {
                // first_digit is rounded up, so this is a leading zero.
                continue;
            }
            buf->putchar(digit + '0');
        }
        for (auto i = last_digit; i > 0; i -= 1) {
            buf->putchar('0');
        }
    } else {
        auto whole_part = static_cast<u64>(x);
        u64 divisor = 10000000000000000000ull;
        bool has_printed_nonzeroes = false;
        do {
            auto digit = whole_part / divisor;
            if (has_printed_nonzeroes || digit > 0 || divisor == 1) {
                has_printed_nonzeroes = true;
                buf->putchar(digit % 10 + '0');
            }
        } while (divisor /= 10);
        if (x > whole_part) { // fractional part exists
            buf->putchar('.');
            auto digits_to_skip = -log10_x;
            for (int i = 0; i < digits_to_skip - 1; i += 1) {
                buf->putchar('0');
            }
            f64 x_skipped;
            if (digits_to_skip > 0) {
                x_skipped = x * pow10(digits_to_skip);
            } else {
                x_skipped = x * 10; // Magic?
            }
            for (u64 i = 0; i < 20; i += 1) {
                f64 scaled = x_skipped * pow10(i);
                auto digit = static_cast<u64>(scaled);
                buf->putchar(digit % 10 + '0');
                if (scaled - digit < 1e-5) {
                    break;
                }
            }
        }
    }
}
u64 print_get_item_size(const fmt_float &fmt) {
    if (fmt.x == 0.0) {
        return 2; // +1 for minus sign if x == -0
    }
    if ((f64bits(fmt.x) >> 52 & 0x7ff) == 0) { // subnormal
        return 5;
    }
    if ((f64bits(fmt.x) >> 52 & 0x7ff) == 0x7ff) {
        return 4; // either 'inf' or 'nan', +1 for negative sign
    }
    return (SDL_abs(log10_approx(fmt.x)) + 2) + 24; // lots of slack
}
u64 print_get_item_size(const fmt_char &) { return 4; }
void print_item(print_buffer *buf, const fmt_char &fmt) { buf->putchar(fmt.c); }
void print_item(print_buffer *buf, print_buffer src) {
    // The user requested that we print this buffer, so if len is longer than
    // cap, then this buffer is technically invalid, but there's nothing we can
    // really do about it.
    u64 len = ng_min(src.len, src.cap);
    for (u64 i = 0; i < len; i += 1) {
        buf->putchar(src.str[i]);
    }
}
u64 print_get_item_size(print_buffer src) { return ng_min(src.len, src.cap); }
void print_item(print_buffer *buf, bool b) {
    print_item(buf, b ? "true" : "false");
}
u64 print_get_item_size(bool) { return 5; }
void print_item(print_buffer *buf, s8 i) { print_item(buf, (s64)i); }
u64 print_get_item_size(s8) { return 8; }
void print_item(print_buffer *buf, u8 u) { print_item(buf, (u64)u); }
u64 print_get_item_size(u8) { return 8; }
void print_item(print_buffer *buf, s16 i) { print_item(buf, (s64)i); }
u64 print_get_item_size(s16) { return 16; }
void print_item(print_buffer *buf, u16 u) { print_item(buf, (u64)u); }
u64 print_get_item_size(u16) { return 16; }
void print_item(print_buffer *buf, s32 i) { print_item(buf, (s64)i); }
u64 print_get_item_size(s32) { return 32; }
void print_item(print_buffer *buf, u32 u) { print_item(buf, (u64)u); }
u64 print_get_item_size(u32) { return 32; }
void print_item(print_buffer *buf, s64 i) { print_item(buf, fmt_int(i)); }
u64 print_get_item_size(s64) { return 64; }
void print_item(print_buffer *buf, u64 u) {
    print_item(buf, fmt_int(u, false));
}
u64 print_get_item_size(u64) { return 64; }
void print_item(print_buffer *buf, void *ptr) {
    print_item(buf, fmt_int((uptr)ptr, false, 16));
}
u64 print_get_item_size(void *) { return 16; } // @Volatile: platform
constexpr auto f32_chars = 9 + sizeof('-') + sizeof('.');
void print_item(print_buffer *buf, f32 f) {
    print_item(buf, fmt_float(f, f32_chars));
}
u64 print_get_item_size(f32 f) {
    return print_get_item_size(fmt_float(f, f32_chars));
}
constexpr auto f64_chars = 17 + sizeof('-') + sizeof('.');
void print_item(print_buffer *buf, f64 f) {
    print_item(buf, fmt_float(f, f64_chars));
}
u64 print_get_item_size(f64 f) {
    return print_get_item_size(fmt_float(f, f64_chars));
}
void print_item(print_buffer *buf, const char *ptr) {
    if (ptr) {
        buf->put_u8s((u8 *)ptr, SDL_strlen(ptr));
    } else {
        print_item(buf, "(null)");
    }
}
u64 print_get_item_size(const char *ptr) { return ptr ? SDL_strlen(ptr) : 6; }
#endif // NG_DEFINE
template <class... Args>
void assert_failed(const char *start, const char *func, const char *end,
                   const char *fmt, const Args &... args) {
    ng::print("Assert fired at %%%", start, func, end);
    ng::print(fmt, args...);
    {
        u8 lf = '\n';
        ng::print_buffer buf;
        buf.str = &lf;
        buf.cap = 1;
        buf.len = 1;
        ng::output_print_buffer(buf);
    }
}
// string class
struct string {
    u64 len = 0;
    u8 *ptr = nullptr;
    NG_INLINE constexpr string() = default;
    NG_INLINE constexpr string(u64 len, u8 *ptr) : len{len}, ptr{ptr} {}
    operator bool() { return len > 0 && ptr != nullptr; }
    bool null_terminated();
    const char *c_str();
    u8 &operator[](u64 n);
    string(const char *c_str);
    string &operator=(const char *c_str);
    string substr(u64 index, u64 length);
    NG_INLINE auto begin() { return ptr; }
    NG_INLINE auto end() { return ptr + len; }

    NG_INLINE string(print_buffer b) : len{ng_min(b.len, b.cap)}, ptr{b.str} {}
};
void print_item(print_buffer *buf, string s);
string copy_string(string s);
void free_string(string *s);
NG_INLINE constexpr static const string operator""_s(const char *str,
                                                     usize length) {
    return string{(u64)length, (u8 *)str};
}
struct c_str {
    char *mem;
    u64 length;
    c_str(string source);
    c_str(const c_str &) = delete;
    ~c_str();
    NG_INLINE operator const char *() { return mem; }
};
} // namespace ng
bool operator==(ng::string lhs, ng::string rhs);
bool operator!=(ng::string lhs, ng::string rhs);
bool operator==(ng::string lhs, const char *rhs);
bool operator!=(ng::string lhs, const char *rhs);
bool operator==(const char *lhs, ng::string rhs);
bool operator!=(const char *lhs, ng::string rhs);
#ifdef NG_DEFINE
namespace ng {
string print_buffer_to_string(print_buffer buf) {
    return string{buf.len, buf.str};
}
// string class
void print_item(print_buffer *buf, string s) {
    if (s.ptr != nullptr) {
        for (int i = 0, end = (int)s.len; i < end; i += 1) {
            buf->putchar(s.ptr[i]);
        }
    }
}
bool string::null_terminated() {
    if (len > 0 && ptr != nullptr) {
        for (u64 i = len; i > 0; i -= 1) {
            if (ptr[i - 1] == '\0') {
                return true;
            }
        }
    }
    return false;
}
static const char empty_c_str[] = "";
const char *string::c_str() {
    if (null_terminated()) {
        return (const char *)ptr;
    } else {
        return empty_c_str;
    }
}
u8 &string::operator[](u64 n) {
    ng_assert(n >= 0 && n < len);
    return ptr[n];
}
string::string(const char *c_str) {
    ptr = (u8 *)c_str;
    while (c_str && *c_str++)
        ;
    len = c_str - (char *)ptr - 1;
}
string &string::operator=(const char *c_str) { return *this = string(c_str); }
string string::substr(u64 index, u64 length) {
    auto real_index = (min)(index, len);
    auto real_length = length;
    if (real_index + real_length > len) {
        real_length = len - real_index;
    }
    return string{real_length, ptr + real_index};
}
string copy_string(string s) {
    string result;
    result.ptr = (u8 *)SDL_malloc(s.len);
    result.len = s.len;
    SDL_memcpy(result.ptr, s.ptr, s.len);
    return result;
}
void free_string(string *s) {
    if (s->ptr) {
        SDL_free(s->ptr);
    }
    s->ptr = nullptr;
    s->len = 0;
}
c_str::c_str(string source) {
    length = source.len + 1;
    mem = (char *)SDL_malloc(length);
    ng_assert(mem != nullptr);
    SDL_memcpy(mem, source.ptr, source.len);
    mem[length - 1] = '\0';
}
c_str::~c_str() { SDL_free(this->mem); }
} // namespace ng
bool operator==(ng::string lhs, ng::string rhs) {
    if (lhs.len != rhs.len) {
        return false;
    }
    if (lhs.ptr == rhs.ptr) {
        return true;
    }
    if (lhs.ptr == nullptr || rhs.ptr == nullptr) {
        return false;
    }
    return SDL_memcmp(lhs.ptr, rhs.ptr, (ng::usize)lhs.len) == 0;
}
bool operator!=(ng::string lhs, ng::string rhs) { return !(lhs == rhs); }
bool operator==(ng::string lhs, const char *rhs) {
    if (rhs == nullptr) {
        return lhs.ptr == nullptr;
    }
    return lhs == ng::string{SDL_strlen(rhs), (ng::u8 *)rhs};
}
bool operator!=(ng::string lhs, const char *rhs) { return !(lhs == rhs); }
bool operator==(const char *lhs, ng::string rhs) { return rhs == lhs; }
bool operator!=(const char *lhs, ng::string rhs) { return rhs != lhs; }
#endif // NG_DEFINE
namespace ng {
int utf8strlen(const char *str);
#ifdef NG_DEFINE
int utf8strlen(const char *str) {
    int result = 0;
    while (*str) {
        if (*str & 0x80) {        // high bit set
            if (!(*str & 0x40)) { // continuation bit not set
                result += 1;      // it's not a continuation byte
            }
        } else { // it's ascii
            result += 1;
        }
        str += 1;
    }
    return result;
}
#endif // NG_DEFINE
// containers
template <class T> struct array {
    s64 count = 0;
    u64 allocated = 0;
    T *data = nullptr;

    template <class U> operator array<U>() {
        static_assert(sizeof(T) == sizeof(U), "");
        array<U> result;
        result.count = count;
        result.allocated = allocated;
        result.data = (U *)data;
        return result;
    }

    inline T &operator[](int n) {
        ng_assert(n < count);
        return data[n];
    }
    array copy() const {
        array result = {};
        result.count = count;
        if (count > 0) {
            result.allocated = count;
            result.data = SDL_malloc(this->count * sizeof(T));
            SDL_memcpy(result.data, data, result.allocated * sizeof(T));
        }
        return result;
    }
    void amortize(s64 new_count) {
        auto new_capacity = allocated;
        while ((u64)new_count > new_capacity) {
            new_capacity = new_capacity * 3 / 2;
            new_capacity += 8;
        }
        data = (T *)SDL_realloc(data, new_capacity * sizeof(T));
        allocated = new_capacity;
    }
    void reserve(u64 new_capacity) {
        if (new_capacity > allocated) {
            data = (T *)SDL_realloc(data, new_capacity * sizeof(T));
            allocated = new_capacity;
        }
    }
    void resize(s64 new_len, T value = {}) {
        reserve(new_len);
        for (s64 i = count; i < new_len; i += 1) {
            data[i] = value;
        }
        count = new_len;
    }
    void release() {
        if (data) {
            SDL_free(data);
            data = nullptr;
        }
        allocated = 0;
        count = 0;
    }
    T *push(T value = {}) {
        if (count >= 0 && (u64)count >= allocated) {
            amortize(count + 1);
        }
        return &(data[(count += 1) - 1] = value);
    }
    void pop() {
        ng_assert(count > 0, "Popped an empty ng::array");
        if (count > 0) {
            count -= 1;
        }
    }
    T *insert(s64 index, T value = {}) {
        if (index >= count) {
            return push(value);
        } else {
            if (count >= 0 && (u64)count >= allocated) {
                amortize(count + 1);
            }
            ng::memmove(
                data[index + 1], data[index], (count - index - 1) * sizeof(T));
            count += 1;
            return (data[index] = value);
        }
    }

    void remove(s64 index) {
        ng_assert(index < count, "index = %, count = %", index, count);
        ng::swap(data[index], data[count - 1]);
        pop();
    }
    void remove_ordered(s64 index) {
        ng_assert(index < count);
        ng::memmove(
            data + index, data + index + 1, (count - index - 1) * sizeof(T));
        count -= 1;
    }
    NG_INLINE void clear() { count = 0; }
    NG_INLINE auto begin() const { return data; }
    NG_INLINE auto end() const { return data + count; }
};
template <class T> void print_item(print_buffer *buf, const array<T> &a) {
    print_item(buf, "array(count = ");
    print_item(buf, a.count);
    print_item(buf, ", allocated = ");
    print_item(buf, a.allocated);
    buf->putchar(')'), buf->putchar(' '), buf->putchar('{');
    for (int i = 0; i < a.count; i += 1) {
        print_item(buf, a.data[i]);
        if (i < a.count - 1) {
            print_item(buf, ", ");
        }
    }
    buf->putchar('}');
}
template <class T> u64 print_get_item_size(const array<T> &a) {
    u64 total = sizeof("array(count = , allocated = ) {");
    total += print_get_item_size(a.count);
    total += print_get_item_size(a.allocated);
    for (int i = 0; i < a.count; i += 1) {
        total += print_get_item_size(a.data[i]);
        total += sizeof(", ");
    }
    total += sizeof('}');
    return total;
}
template <class T> struct auto_array : array<T> {
    using base = array<T>;
    auto_array copy() const {
        auto_array result = {};
        result.count = this->count;
        if (this->count > 0) {
            result.allocated = this->count;
            result.data = (T *)SDL_malloc(this->count * sizeof(T));
            for (s64 i = 0; i < result.count; i += 1) {
                new (result.data + i) T{this->data[i]};
            }
        }
        return result;
    }
    auto_array() : base{} {}
    auto_array(auto_array &&rhs) : base{static_cast<base &&>(rhs)} { rhs = {}; }
    auto_array(const auto_array &rhs) : auto_array{rhs.copy()} {}
    auto_array &operator=(auto_array &&rhs) {
        base::operator=(static_cast<base &&>(rhs));
        rhs = {};
        return *this;
    }
    auto_array &operator=(const auto_array &rhs) {
        base::operator=(rhs.copy());
        return *this;
    }
    ~auto_array() { base::release(); }
};
template <class Key, class Value> struct map {
    struct slot {
        bool occupied = false;
        u64 hash = 0;
        Key key = {};
        Value value = {};
    };
    slot *slots = nullptr;
    u64 allocated = 0;
    s64 count = 0;
    map copy() {
        map result = {};
        if (this->allocated > 0) {
            result.allocate(this->allocated);
            SDL_memcpy(
                result.slots, this->slots, this->allocated * sizeof(slot));
        }
        result.count = this->count;
        return result;
    }
    void allocate(u64 new_capacity) {
        allocated = new_capacity;
        slots = (slot *)SDL_malloc(allocated * sizeof(slot));
    }
    void expand() {
        auto old_slots = slots;
        auto old_cap = allocated;

        auto new_cap = allocated * 3 / 2;
        if (new_cap < 8) {
            new_cap = 8;
        }
        allocate(new_cap);
        if (old_slots) {
            for (u64 i = 0; i < old_cap; i += 1) {
                auto slot = old_slots[i];
                if (slot.occupied) {
                    insert(slot.key, slot.value);
                }
            }
            SDL_free(old_slots, old_cap);
        } else {
            for (u64 i = 0; i < allocated; i += 1) {
                new (slots + i) slot{};
            }
        }
    }
    void release() {
        if (slots) {
            SDL_free(slots, allocated * sizeof(slot));
        }
    }
    Value *insert(Key key, Value value) {
        auto hash = get_hash(key);
        auto index = find_index(hash, key);
        if (index == -1) {
            if ((u64)count >= allocated) {
                expand();
            }
            index = hash % allocated;
            while (slots[index].occupied) { // linear probe
                index += 1;
                if ((u64)index >= allocated) {
                    index = 0;
                }
            }
            count += 1;
        }
        auto slot = &slots[index];
        slot->occupied = true;
        slot->hash = hash;
        slot->key = key;
        slot->value = value;
        return &slot->value;
    }
    void remove(Key key) {
        auto hash = get_hash(key);
        auto index = find_index(hash, key);
        if (index != -1) {
            slots[index].occupied = false;
        }
    }
    s64 find_index(u64 hash, Key key) {
        if (count <= 0) {
            return -1;
        }
        auto slot = hash % allocated;
        auto index = slot;
        while (slots[index].occupied) {
            if (slots[index].hash == hash) {
                if (slots[index].key == key) {
                    return index;
                }
            }
            // linear probe
            index += 1;
            if (index >= allocated) {
                index = 0;
            }
            if (index == slot) { // Looped; all slots are full.
                return -1;
            }
        }
        return -1;
    }
    struct find_result {
        bool found = false;
        Value *value = nullptr;
    };
    find_result operator[](Key key) {
        find_result result = {};
        auto hash = get_hash(key);
        auto index = find_index(hash, key);
        if (index != -1) {
            result.value = &slots[index].value;
            result.found = true;
        }
        return result;
    }
    struct slot_iterator {
        slot *ptr;
        slot *end;
        void operator++() {
            do {
                ptr += 1;
            } while (ptr < end && !ptr->occupied);
        }
        bool operator!=(const slot *) { return ptr < end; }
        slot &operator*() { return *ptr; }
    };
    slot_iterator begin() { return {slots, slots + allocated}; }
    slot *end() { return nullptr; }
};
u64 get_hash(string key);
u64 get_hash(u64 key);
u64 get_hash(void *ptr);
#ifdef NG_DEFINE
u64 get_hash(string key) {
    u64 result = 0xcbf29ce484222325;
    ng_for(key) {
        result *= 0x100000001b3;
        result ^= it;
    }
    return result;
}
u64 get_hash(u64 x) {
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
    x ^= x >> 31;
    return x;
}
u64 get_hash(void *ptr) { return get_hash((u64)ptr); }
#endif // NG_DEFINE
} // namespace ng


#define STBI_ASSERT ng_assert

#define STBI_MALLOC ng::malloc
#define STBI_FREE ng::free
#define STBI_REALLOC ng::realloc

#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF

#define STBI_NO_STDIO
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION

#define STBI_memcpy SDL_memcpy
#define STBI_memset SDL_memset
#define STBI_abs ng::abs
#define STBI_HAS_LROTL 0

#include "stb_image.h"
