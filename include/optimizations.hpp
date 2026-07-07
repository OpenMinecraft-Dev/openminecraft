#ifndef OPTIMIZATIONS_HPP
#define OPTIMIZATIONS_HPP

#if defined(__GNUC__) || defined(__clang__)
#define HOT_FUNC [[gnu::hot]]
#else
#define HOT_FUNC
#endif

#endif
