#ifndef OPTIMIZATIONS_HPP
#define OPTIMIZATIONS_HPP

/*
 * INFO:
 * Macro HOT_FUNC
 *
 * On GNU/Clang compilers:
 * marks the target function *hot* for better performance
 * On MSVC:
 * does nothing
 */
#if defined(__GNUC__) || defined(__clang__)
#define HOT_FUNC [[gnu::hot]]
#else
#define HOT_FUNC
#endif

#endif
