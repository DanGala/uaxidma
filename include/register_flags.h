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
    flags_wrapper(T& f) : flags(f) {};
    void set_flags(T f) { flags |= f; }
    bool check_flags(T f) { return ((flags & f) == f); }
    void clear_flags(T f) { flags &= ~f; }
    T& flags;
};

/**
 * @brief Template thin wrapper around a volatile bitfield to provide field setters and getters
 */
template <typename T>
struct vflags_wrapper
{
    vflags_wrapper(volatile T& f) : vflags(f) {};
    void set_flags(T f) volatile { vflags |= f; }
    bool check_flags(T f) volatile { return ((vflags & f) == f); }
    void clear_flags(T f) volatile { vflags &= ~f; }
    volatile T& vflags;
};

#endif
