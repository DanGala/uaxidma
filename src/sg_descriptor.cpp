#include "sg_descriptor.h"

sg_descriptor_chain::sg_desc_iterator::sg_desc_iterator(sg_descriptor *ptr)
    : p_(ptr)
{
}

sg_descriptor_chain::sg_desc_iterator::sg_desc_iterator(const sg_desc_iterator& it)
    : p_(it.p_)
{
}

sg_descriptor* sg_descriptor_chain::sg_desc_iterator::operator-> () const
{
    return p_; 
}

sg_descriptor& sg_descriptor_chain::sg_desc_iterator::operator* () const
{
    return *p_; 
}

sg_descriptor& sg_descriptor_chain::sg_desc_iterator::operator [] (int n) const
{
    return *(p_ + n); 
}

sg_descriptor_chain::sg_desc_iterator& sg_descriptor_chain::sg_desc_iterator::operator++ ()
{
    ++p_;
    return *this; 
}

sg_descriptor_chain::sg_desc_iterator sg_descriptor_chain::sg_desc_iterator::operator++ (int)
{
    sg_desc_iterator tmp = *this;
    ++p_;
    return tmp; 
}

sg_descriptor_chain::sg_desc_iterator& sg_descriptor_chain::sg_desc_iterator::operator-- ()
{
    --p_;
    return *this; 
}

sg_descriptor_chain::sg_desc_iterator sg_descriptor_chain::sg_desc_iterator::operator-- (int)
{
    sg_desc_iterator tmp = *this;
    --p_;
    return tmp; 
}

sg_descriptor_chain::sg_desc_iterator& sg_descriptor_chain::sg_desc_iterator::operator+= (int n)
{
    p_ += n;
    return *this; 
}

sg_descriptor_chain::sg_desc_iterator& sg_descriptor_chain::sg_desc_iterator::operator-= (int n)
{
    p_ -= n;
    return *this; 
}

bool sg_descriptor_chain::sg_desc_iterator::operator== (const sg_desc_iterator& other)
{
    return (p_ == other.p_); 
}

bool sg_descriptor_chain::sg_desc_iterator::operator!= (const sg_desc_iterator& other)
{
    return (p_ != other.p_); 
}

bool sg_descriptor_chain::sg_desc_iterator::operator== (const sg_desc_iterator& other) const
{
    return (p_ == other.p_); 
}

bool sg_descriptor_chain::sg_desc_iterator::operator!= (const sg_desc_iterator& other) const
{
    return (p_ != other.p_); 
}

std::ptrdiff_t operator- (sg_descriptor_chain::sg_desc_iterator lhs, sg_descriptor_chain::sg_desc_iterator rhs)
{
    return lhs.p_ - rhs.p_; 
}

sg_descriptor_chain::sg_desc_iterator operator+ (sg_descriptor_chain::sg_desc_iterator lhs, int rhs)
{
    return lhs += rhs; 
}

sg_descriptor_chain::sg_desc_iterator operator- (sg_descriptor_chain::sg_desc_iterator lhs, int rhs)
{
    return lhs -= rhs; 
}

sg_descriptor_chain::sg_desc_iterator operator+ (int lhs, sg_descriptor_chain::sg_desc_iterator rhs)
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

sg_descriptor_chain::sg_desc_iterator sg_descriptor_chain::begin()
{
    return sg_desc_iterator(head_);
}

const sg_descriptor_chain::sg_desc_iterator sg_descriptor_chain::begin() const
{
    return sg_desc_iterator(head_);
}

sg_descriptor_chain::sg_desc_iterator sg_descriptor_chain::end()
{
    return sg_desc_iterator(head_ + size_);
}

const sg_descriptor_chain::sg_desc_iterator sg_descriptor_chain::end() const
{
    return sg_desc_iterator(head_ + size_);
}

sg_descriptor_chain::sg_desc_iterator sg_descriptor_chain::next(sg_desc_iterator& it)
{
    return (it == end()) ? begin() : sg_desc_iterator(it)++;
}

const sg_descriptor_chain::sg_desc_iterator sg_descriptor_chain::next(const sg_desc_iterator& it) const
{
    return (it == end()) ? begin() : sg_desc_iterator(it)++;
}

