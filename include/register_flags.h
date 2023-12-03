#ifndef _REGISTER_FLAGS_H
#define _REGISTER_FLAGS_H

#include <cstdint>

template <typename flags>
inline constexpr flags operator|= (flags f1, flags f2)
{
    return static_cast<flags>(static_cast<uint32_t>(f1) | static_cast<uint32_t>(f2));
}

template <typename flags>
inline constexpr flags operator| (flags f1, flags f2)
{
    return static_cast<flags>(static_cast<uint32_t>(f1) | static_cast<uint32_t>(f2));
}

template <typename flags>
inline constexpr flags operator& (flags f1, flags f2)
{
    return static_cast<flags>(static_cast<uint32_t>(f1) & static_cast<uint32_t>(f2));
}

template <typename flags>
inline constexpr flags operator&= (flags f1, flags f2)
{
    return static_cast<flags>(static_cast<uint32_t>(f1) & static_cast<uint32_t>(f2));
}

template <typename flags>
inline constexpr flags operator& (flags f, uint32_t v)
{
    return static_cast<flags>(static_cast<uint32_t>(f) & v);
}

template <typename flags>
inline constexpr flags operator&= (flags f, uint32_t v)
{
    return static_cast<flags>(static_cast<uint32_t>(f) & v);
}

template <typename flags>
inline constexpr flags operator~ (flags f)
{
    return static_cast<flags>( ~static_cast<uint32_t>(f));
}

/**
 * @brief Template thin wrapper around a bitfield to provide field setters and getters
 */
template <typename T>
struct flags_wrapper
{
    flags_wrapper(T& f1) : f(f1) {};
    void set(T f1) { f |= f1; }
    bool check(T f1) { return ((f & f1) == f1); }
    void clear(T f1) { f &= ~f1; }
    T& f;
};

/**
 * @brief Template thin wrapper around a bitfield to provide field setters and getters
 */
template <typename T>
struct cflags_wrapper
{
    cflags_wrapper(const T& f1) : cf(f1) {};
    bool check(T f1) const { return ((cf & f1) == f1); }
    const T& cf;
};

/**
 * @brief Template thin wrapper around a volatile bitfield to provide field setters and getters
 */
template <typename T>
struct vflags_wrapper
{
    vflags_wrapper(volatile T& f1) : vf(f1) {};
    void set(T f1) volatile { vf |= f1; }
    bool check(T f1) volatile { return ((vf & f1) == f1); }
    void clear(T f1) volatile { vf &= ~f1; }
    volatile T& vf;
};

/**
 * @brief Template thin wrapper around a volatile bitfield to provide field setters and getters
 */
template <typename T>
struct cvflags_wrapper
{
    cvflags_wrapper(const volatile T& f1) : cvf(f1) {};
    bool check(T f1) const volatile { return ((cvf & f1) == f1); }
    const volatile T& cvf;
};

#endif
