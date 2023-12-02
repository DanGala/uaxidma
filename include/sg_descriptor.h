#ifndef _SG_DESCRIPTOR_H
#define _SG_DESCRIPTOR_H

#include "register_flags.h"
#include <cstdint>
#include <cstddef>
#include <iterator>

/**
 * @brief Scatter/Gather control register flags
 */
enum class controlf : uint32_t
{
    buf_len = 0x3fffffffu << 0,
    eof = 1u << 26,
    sof = 1u << 27,
    all = 0xffffffff
};

static constexpr uint32_t sg_max_buf_len = static_cast<uint32_t>(controlf::buf_len);

/**
 * @brief Scatter/Gather status register flags
 */
enum class statusf : uint32_t
{
    xfer_bytes = 0x3ffffffu << 0,
    rxeof = 1u << 26,
    rxsof = 1u << 27,
    dma_int_err = 1u << 28,
    dma_slv_err = 1u << 29,
    dma_dec_err = 1u << 30,
    dma_errors = (dma_int_err | dma_slv_err | dma_dec_err),
    complete = 1u << 31,
    all = 0xffffffff
};

/**
 * @brief Scatter/Gather Descriptor (Non-multichannel mode) for AXI DMA
 * @note Descriptors must be 16-word aligned. Any other alignment has undefined results.
 */
struct alignas(64) sg_descriptor
{
    uint32_t next_desc;     //!< Next Descriptor Pointer @0x00
    uint32_t next_desc_msb; //!< MSB of Next Descriptor Pointer @0x04
    uint32_t buf_addr;      //!< Buffer address @0x08
    uint32_t buf_addr_msb;  //!< MSB of Buffer address @0x0C
    uint32_t reserved1[2];  //!< Reserved @0x10 - 0x14
    controlf control;       //!< Control @0x18
    statusf status;         //!< Status field @0x1C
    uint32_t app[5];        //!< User Application Fields @0x20 - 0x30
    uint32_t reserved2[3];  //!< Used to ensure 16-word alignment
};

class sg_descriptor_chain
{
public:
    class sg_desc_iterator
    {
        sg_descriptor* p_;
    public:
        sg_desc_iterator(sg_descriptor *ptr);
        sg_desc_iterator(const sg_desc_iterator& it);
        sg_descriptor* operator-> () const;
        sg_descriptor& operator* () const;
        sg_descriptor& operator [] (int n) const;
        sg_desc_iterator& operator++ ();
        sg_desc_iterator operator++ (int);
        sg_desc_iterator& operator-- ();
        sg_desc_iterator operator-- (int);
        sg_desc_iterator& operator+= (int n);
        sg_desc_iterator& operator-= (int n);
        bool operator== (const sg_desc_iterator& other);
        bool operator!= (const sg_desc_iterator& other);
        bool operator== (const sg_desc_iterator& other) const;
        bool operator!= (const sg_desc_iterator& other) const;

        friend std::ptrdiff_t operator- (sg_desc_iterator lhs, sg_desc_iterator rhs);
        friend sg_desc_iterator operator+ (sg_desc_iterator lhs, int rhs);
        friend sg_desc_iterator operator- (sg_desc_iterator lhs, int rhs);
        friend sg_desc_iterator operator+ (int lhs, sg_desc_iterator rhs);
    };

    class sg_desc_const_iterator
    {
        const sg_descriptor* cp_;
    public:
        sg_desc_const_iterator(const sg_descriptor *ptr);
        sg_desc_const_iterator(const sg_desc_const_iterator& it);
        const sg_descriptor* operator-> () const;
        const sg_descriptor& operator* () const;
        const sg_descriptor& operator [] (int n) const;
        sg_desc_const_iterator& operator++ ();
        sg_desc_const_iterator operator++ (int);
        sg_desc_const_iterator& operator-- ();
        sg_desc_const_iterator operator-- (int);
        sg_desc_const_iterator& operator+= (int n);
        sg_desc_const_iterator& operator-= (int n);
        bool operator== (const sg_desc_const_iterator& other);
        bool operator!= (const sg_desc_const_iterator& other);
        bool operator== (const sg_desc_const_iterator& other) const;
        bool operator!= (const sg_desc_const_iterator& other) const;

        friend std::ptrdiff_t operator- (sg_desc_const_iterator lhs, sg_desc_const_iterator rhs);
        friend sg_desc_const_iterator operator+ (sg_desc_const_iterator lhs, int rhs);
        friend sg_desc_const_iterator operator- (sg_desc_const_iterator lhs, int rhs);
        friend sg_desc_const_iterator operator+ (int lhs, sg_desc_const_iterator rhs);
    };

    sg_descriptor_chain() = default;
    sg_descriptor_chain(sg_descriptor *ptr, std::size_t sz);
    sg_descriptor& operator[](std::size_t idx);
    const sg_descriptor& operator[](std::size_t idx) const;
    std::size_t size() const;
    sg_desc_iterator begin();
    sg_desc_const_iterator const_begin() const;
    const sg_desc_iterator begin() const;
    sg_desc_iterator end();
    sg_desc_const_iterator const_end() const;
    const sg_desc_iterator end() const;
    sg_desc_iterator next(sg_desc_iterator& it);
    sg_desc_const_iterator next(sg_desc_const_iterator& it);
    const sg_desc_iterator next(const sg_desc_iterator& it) const;
    const sg_desc_const_iterator next(const sg_desc_const_iterator& it) const;

    static std::size_t distance(const sg_desc_iterator& first, const sg_desc_iterator& last);
    static std::size_t distance(const sg_desc_const_iterator& first, const sg_desc_const_iterator& last);

private:
    sg_descriptor* head_;
    std::size_t size_;
};

/**
 * @brief Thin wrapper around a controlf object to provide field setters and getters
 */
struct controlf_wrapper : public flags_wrapper<controlf>
{
    controlf_wrapper(controlf& f) : flags_wrapper<controlf>{f} {}
    void set_buf_len(size_t len) { set_flags(controlf::buf_len & len); }
};

/**
 * @brief Thin wrapper around a volatile controlf object to provide field setters and getters
 */
struct vcontrolf_wrapper : public vflags_wrapper<controlf>
{
    vcontrolf_wrapper(volatile controlf& f) : vflags_wrapper<controlf>{f} {}
    void set_buf_len(size_t len) volatile { set_flags(controlf::buf_len & len); }
};

/**
 * @brief Thin wrapper around a statusf object to provide field setters and getters
 */
struct statusf_wrapper : public flags_wrapper<statusf>
{
    statusf_wrapper(statusf& f) : flags_wrapper<statusf>{f} {}
    size_t get_xfer_bytes() const { return static_cast<size_t>(flags & statusf::xfer_bytes); }
};

/**
 * @brief Thin wrapper around a const statusf object to provide field setters and getters
 */
struct cstatusf_wrapper : public cflags_wrapper<statusf>
{
    cstatusf_wrapper(const statusf& f) : cflags_wrapper<statusf>{f} {}
    size_t get_xfer_bytes() const { return static_cast<size_t>(cflags & statusf::xfer_bytes); }
};

/**
 * @brief Thin wrapper around a volatile statusf object to provide field setters and getters
 */
struct vstatusf_wrapper : public vflags_wrapper<statusf>
{
    vstatusf_wrapper(volatile statusf& f) : vflags_wrapper<statusf>{f} {}
    size_t get_xfer_bytes() const volatile { return static_cast<size_t>(vflags & statusf::xfer_bytes); }
};

/**
 * @brief Thin wrapper around a const volatile statusf object to provide field setters and getters
 */
struct cvstatusf_wrapper : public cvflags_wrapper<statusf>
{
    cvstatusf_wrapper(const volatile statusf& f) : cvflags_wrapper<statusf>{f} {}
    size_t get_xfer_bytes() const volatile { return static_cast<size_t>(cvflags & statusf::xfer_bytes); }
};

#endif
