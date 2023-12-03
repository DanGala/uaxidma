#include "uaxidma.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t *dma_buffer::data()
{
    return data_;
}

size_t dma_buffer::length()
{
    return length_;
}

bool dma_buffer::set_payload(size_t len)
{
    if (len > capacity_)
    {
        return false;
    }

    length_ = len;
    return true;
}

uaxidma::uaxidma(const std::string& udmabuf_name, size_t udmabuf_size, const std::string& axidma_uio_name, 
                 dma_mode mode, transfer_direction direction, size_t buffer_size)

    : axidma{udmabuf_name, udmabuf_size, axidma_uio_name, static_cast<axi_dma::dma_mode>(mode),
             static_cast<axi_dma::transfer_direction>(direction), buffer_size},
      mode(mode),
      direction(direction),
      buffers{(mode == dma_mode::normal)} // in cyclic mode, the hardware won't wait for the user anyway
{
}

bool uaxidma::initialize()
{
    if (axidma.initialize() && axidma.start())
    {
        buffers.initialize(axidma.sg_desc_chain.size());

        for (auto& desc : axidma.sg_desc_chain)
        {
            buffers.add({axidma.get_virt_buffer_pointer(desc), axidma.get_buffer_size(), desc});
        }

        return true;
    }

    return false;
}

std::pair<uaxidma::acquisition_result, dma_buffer*> uaxidma::get_buffer(int timeout)
{
    if (buffers.empty())
    {
        errno = EAGAIN;
        return {acquisition_result::error, nullptr};
    }

    axidma.clean_interrupt();

    if (!buffers.peek_next().desc_handle_.completed())
    {
        auto poll_ret = axidma.poll_interrupt(timeout);
        if (poll_ret != axi_dma::acquisition_result::success)
        {
            return {static_cast<acquisition_result>(poll_ret), nullptr};
        }
    }

    dma_buffer& acquired = buffers.acquire();

    if (direction == transfer_direction::dev_to_mem)
    {
        acquired.set_payload(acquired.desc_handle_.get_buffer_len());
    }

    return {acquisition_result::success, &acquired};
}

void uaxidma::mark_reusable(dma_buffer &buf)
{
    // Prepare buffer to check for completion again next time
    buf.desc_handle_.clear_complete_flag();
    buffers.release(buf);
}

void uaxidma::submit_buffer(dma_buffer &buf)
{
    axidma.transfer_buffer(buf.desc_handle_.d, buf.length_);
    buffers.release(buf);
}

uaxidma::dma_buffer_ring::dma_buffer_ring(bool limit_refs)
    : limit_refs_(limit_refs)
{
}

void uaxidma::dma_buffer_ring::initialize(size_t count)
{
    buffers_.reserve(count);
}

void uaxidma::dma_buffer_ring::add(const dma_buffer& buf)
{
    buffers_.push_back(buf);
    if (!available_++) next_ = buffers_.begin();
}

bool uaxidma::dma_buffer_ring::empty() const
{
    return (limit_refs_ && !available_);
}

const dma_buffer &uaxidma::dma_buffer_ring::peek_next() const
{
    return *next_;
}

dma_buffer& uaxidma::dma_buffer_ring::acquire()
{
    if (limit_refs_) available_--;
    dma_buffer &buf = *next_;
    if (++next_ == buffers_.end()) next_ = buffers_.begin();
    return buf;
}

void uaxidma::dma_buffer_ring::release(dma_buffer& ptr)
{
    (void)ptr;
    if (limit_refs_) available_++;
}
