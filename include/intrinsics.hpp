#ifndef MINECRAFTPP_INTRINSICS_HPP
#define MINECRAFTPP_INTRINSICS_HPP

#if defined(__clang__) || defined (__GNUC__)
    #define MINECRAFTPP_UNREACHABLE() __builtin_unreachable()
    #define MINECRAFTPP_FORCEINLINE inline __attribute__((__always_inline__))
#elif _MSC_VER
    #define MINECRAFTPP_UNREACHABLE() __assume(0)
    #define MINECRAFTPP_FORCEINLINE inline
#else 
    #error "unknown compiler"
#endif

// Dunno where this one is used
#ifdef _MSC_VER
	#define assume(Cond) __assume(Cond)
#elif defined(__clang__)
    #define assume(Cond) __builtin_assume(Cond)
#else
	#define assume(Cond) ((Cond) ? static_cast<void>(0) : __builtin_unreachable())
#endif

// Compatibility defs
// TODO: Remove.
#define UNREACHABLE() MINECRAFTPP_UNREACHABLE()
#define forceinline MINECRAFTPP_FORCEINLINE

#endif // !MINECRAFTPP_INTRINSICS_HPP
