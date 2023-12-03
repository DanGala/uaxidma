#include "sg_descriptor.h"

sg_descriptor_handle::sg_descriptor_handle(sg_descriptor &desc)
    : d(desc)
{
}

bool sg_descriptor_handle::completed() const
{
    cstatusf_wrapper status{d.status};
    bool complete = status.flags.check(statusf::complete);
    if (complete)
    {
#ifdef __ARM_ARCH
        // Avoid speculatively doing any work before the status is actually read
        asm volatile("dmb sy");
#endif
    }
    return complete;
}

void sg_descriptor_handle::clear_complete_flag()
{
    statusf_wrapper status{d.status};
    status.flags.clear(statusf::complete);
#ifdef __ARM_ARCH
    // Avoid speculatively doing any work before the status is actually updated
    asm volatile("dmb st");
#endif
}

/**
 * @brief Get the amount of data transferred by the buffer described by a descriptor
 * @return length in bytes
 */
size_t sg_descriptor_handle::get_buffer_len() const
{
    cstatusf_wrapper status{d.status};
    const size_t len = status.get_xfer_bytes();
#ifdef __ARM_ARCH
    // Avoid speculatively doing any work before the status is actually read
    asm volatile("dmb sy");
#endif
    return len;
}

sg_descriptor_chain::iterator::iterator(sg_descriptor *ptr)
    : p_(ptr)
{
}

sg_descriptor_chain::iterator::iterator(const iterator& it)
    : p_(it.p_)
{
}

sg_descriptor_chain::iterator::iterator(sg_descriptor &ref)
    : p_(&ref)
{
}

sg_descriptor* sg_descriptor_chain::iterator::operator-> () const
{
    return p_;
}

sg_descriptor& sg_descriptor_chain::iterator::operator* () const
{
    return *p_;
}

sg_descriptor& sg_descriptor_chain::iterator::operator [] (int n) const
{
    return *(p_ + n);
}

sg_descriptor_chain::iterator& sg_descriptor_chain::iterator::operator++ ()
{
    ++p_;
    return *this;
}

sg_descriptor_chain::iterator sg_descriptor_chain::iterator::operator++ (int)
{
    iterator tmp = *this;
    ++p_;
    return tmp;
}

sg_descriptor_chain::iterator& sg_descriptor_chain::iterator::operator-- ()
{
    --p_;
    return *this;
}

sg_descriptor_chain::iterator sg_descriptor_chain::iterator::operator-- (int)
{
    iterator tmp = *this;
    --p_;
    return tmp;
}

sg_descriptor_chain::iterator& sg_descriptor_chain::iterator::operator+= (int n)
{
    p_ += n;
    return *this;
}

sg_descriptor_chain::iterator& sg_descriptor_chain::iterator::operator-= (int n)
{
    p_ -= n;
    return *this;
}

bool sg_descriptor_chain::iterator::operator== (const iterator& other)
{
    return (p_ == other.p_);
}

bool sg_descriptor_chain::iterator::operator!= (const iterator& other)
{
    return (p_ != other.p_);
}

bool sg_descriptor_chain::iterator::operator== (const iterator& other) const
{
    return (p_ == other.p_); 
}

bool sg_descriptor_chain::iterator::operator!= (const iterator& other) const
{
    return (p_ != other.p_); 
}

std::ptrdiff_t operator- (sg_descriptor_chain::iterator lhs, sg_descriptor_chain::iterator rhs)
{
    return lhs.p_ - rhs.p_; 
}

sg_descriptor_chain::iterator operator+ (sg_descriptor_chain::iterator lhs, int rhs)
{
    return lhs += rhs; 
}

sg_descriptor_chain::iterator operator- (sg_descriptor_chain::iterator lhs, int rhs)
{
    return lhs -= rhs; 
}

sg_descriptor_chain::iterator operator+ (int lhs, sg_descriptor_chain::iterator rhs)
{
    return rhs += lhs; 
}

sg_descriptor_chain::sg_descriptor_chain(sg_descriptor *ptr, std::size_t sz)
    : head_(ptr), size_(sz * sizeof(sg_descriptor))
{
}

sg_descriptor& sg_descriptor_chain::operator[](std::size_t idx)
{
    return head_[idx];
}

const sg_descriptor& sg_descriptor_chain::operator[](std::size_t idx) const
{
    return head_[idx];
}

std::size_t sg_descriptor_chain::size() const
{
    return size_;
}

sg_descriptor_chain::iterator sg_descriptor_chain::begin()
{
    return iterator(head_);
}

const sg_descriptor_chain::iterator sg_descriptor_chain::begin() const
{
    return iterator(head_);
}

sg_descriptor_chain::iterator sg_descriptor_chain::end()
{
    return iterator(head_ + size_);
}

const sg_descriptor_chain::iterator sg_descriptor_chain::end() const
{
    return iterator(head_ + size_);
}

sg_descriptor_chain::iterator sg_descriptor_chain::next(iterator& it)
{
    return (it == end()) ? begin() : iterator(it)++;
}

const sg_descriptor_chain::iterator sg_descriptor_chain::next(const iterator& it) const
{
    return (it == end()) ? begin() : iterator(it)++;
}

std::size_t sg_descriptor_chain::offset(const iterator& some) const
{
    std::size_t hops = 0;
    for (auto it = begin(); it != some; it++)
    {
        hops++; 
    }
    return hops;
}
