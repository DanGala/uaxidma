#include "sg_descriptor.h"

sg_descriptor_chain::iterator::iterator(sg_descriptor *ptr)
    : p_(ptr)
{
}

sg_descriptor_chain::iterator::iterator(const iterator& it)
    : p_(it.p_)
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

sg_descriptor_chain::const_iterator::const_iterator(const sg_descriptor *ptr)
    : cp_(ptr)
{
}

sg_descriptor_chain::const_iterator::const_iterator(const const_iterator& it)
    : cp_(it.cp_)
{
}

const sg_descriptor* sg_descriptor_chain::const_iterator::operator-> () const
{
    return cp_;
}

const sg_descriptor& sg_descriptor_chain::const_iterator::operator* () const
{
    return *cp_;
}

const sg_descriptor& sg_descriptor_chain::const_iterator::operator [] (int n) const
{
    return *(cp_ + n);
}

sg_descriptor_chain::const_iterator& sg_descriptor_chain::const_iterator::operator++ ()
{
    ++cp_;
    return *this;
}

sg_descriptor_chain::const_iterator sg_descriptor_chain::const_iterator::operator++ (int)
{
    const_iterator tmp = *this;
    ++cp_;
    return tmp;
}

sg_descriptor_chain::const_iterator& sg_descriptor_chain::const_iterator::operator-- ()
{
    --cp_;
    return *this;
}

sg_descriptor_chain::const_iterator sg_descriptor_chain::const_iterator::operator-- (int)
{
    const_iterator tmp = *this;
    --cp_;
    return tmp;
}

sg_descriptor_chain::const_iterator& sg_descriptor_chain::const_iterator::operator+= (int n)
{
    cp_ += n;
    return *this;
}

sg_descriptor_chain::const_iterator& sg_descriptor_chain::const_iterator::operator-= (int n)
{
    cp_ -= n;
    return *this;
}

bool sg_descriptor_chain::const_iterator::operator== (const const_iterator& other)
{
    return (cp_ == other.cp_);
}

bool sg_descriptor_chain::const_iterator::operator!= (const const_iterator& other)
{
    return (cp_ != other.cp_);
}

bool sg_descriptor_chain::const_iterator::operator== (const const_iterator& other) const
{
    return (cp_ == other.cp_);
}

bool sg_descriptor_chain::const_iterator::operator!= (const const_iterator& other) const
{
    return (cp_ != other.cp_);
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

std::ptrdiff_t operator- (sg_descriptor_chain::const_iterator lhs, sg_descriptor_chain::const_iterator rhs)
{
    return lhs.cp_ - rhs.cp_; 
}

sg_descriptor_chain::const_iterator operator+ (sg_descriptor_chain::const_iterator lhs, int rhs)
{
    return lhs += rhs; 
}

sg_descriptor_chain::const_iterator operator- (sg_descriptor_chain::const_iterator lhs, int rhs)
{
    return lhs -= rhs; 
}

sg_descriptor_chain::const_iterator operator+ (int lhs, sg_descriptor_chain::const_iterator rhs)
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

std::size_t sg_descriptor_chain::length() const
{
    return distance(begin(), end());
}

sg_descriptor_chain::iterator sg_descriptor_chain::begin()
{
    return iterator(head_);
}

sg_descriptor_chain::const_iterator sg_descriptor_chain::const_begin() const
{
    return const_iterator(head_);
}

const sg_descriptor_chain::iterator sg_descriptor_chain::begin() const
{
    return iterator(head_);
}

sg_descriptor_chain::iterator sg_descriptor_chain::end()
{
    return iterator(head_ + size_);
}

sg_descriptor_chain::const_iterator sg_descriptor_chain::const_end() const
{
    return const_iterator(head_ + size_);
}

const sg_descriptor_chain::iterator sg_descriptor_chain::end() const
{
    return iterator(head_ + size_);
}

sg_descriptor_chain::iterator sg_descriptor_chain::next(iterator& it)
{
    return (it == end()) ? begin() : iterator(it)++;
}

sg_descriptor_chain::const_iterator sg_descriptor_chain::next(const_iterator& it)
{
    return (it == const_end()) ? const_begin() : const_iterator(it)++;
}

const sg_descriptor_chain::iterator sg_descriptor_chain::next(const iterator& it) const
{
    return (it == end()) ? begin() : iterator(it)++;
}

const sg_descriptor_chain::const_iterator sg_descriptor_chain::next(const const_iterator& it) const
{
    return (it == const_end()) ? const_begin() : const_iterator(it)++;
}

std::size_t sg_descriptor_chain::distance(const iterator& first, const iterator& last)
{
    std::size_t hops = 0;
    for (auto it = iterator(first); it != last; it++)
    {
        hops++; 
    }
    return hops;
}

std::size_t sg_descriptor_chain::distance(const const_iterator& first, const const_iterator& last)
{
    std::size_t hops = 0;
    for (auto it = const_iterator(first); it != last; it++)
    {
        hops++; 
    }
    return hops;
}
