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

dma_buffer_list::dma_buffer_list(bool ordered_acquire_release)
    : limit_refs_(ordered_acquire_release)
{
}

void dma_buffer_list::initialize(size_t count)
{
    buffers_.reserve(count);
}

void dma_buffer_list::add(const dma_buffer& buf)
{
    buffers_.push_back(buf);
    if (!available_++) next_ = buffers_.begin();
}

bool dma_buffer_list::empty() const
{
    return (limit_refs_ && !available_);
}

const dma_buffer &dma_buffer_list::peek_next() const
{
    return *next_;
}

dma_buffer& dma_buffer_list::acquire()
{
    if (limit_refs_) available_--;
    dma_buffer &buf = *next_;
    if (++next_ == buffers_.end()) next_ = buffers_.begin();
    return buf;
}

void dma_buffer_list::release(dma_buffer& ptr)
{
    (void)ptr;
    if (limit_refs_) available_++;
}

uaxidma::uaxidma(const std::string& udmabuf_name, size_t udmabuf_size, const std::string& axidma_uio_name, 
                 dma_mode mode, transfer_direction direction, size_t buffer_size)

    : axidma{udmabuf_name, udmabuf_size, axidma_uio_name, static_cast<axi_dma::dma_mode>(mode),
             static_cast<axi_dma::transfer_direction>(direction), buffer_size},
      mode(mode),
      direction(direction),
      buffers{(direction == transfer_direction::mem_to_dev)}
{
}

bool uaxidma::initialize()
{
    if (!axidma.initialize() || !axidma.start())
    {
        return false;
    }

    buffers.initialize(axidma.sg_desc_chain.length());

    for (auto& desc : axidma.sg_desc_chain)
    {
        buffers.add({axidma.get_virt_buffer_pointer(desc), axidma.get_buffer_size(), desc});
    }

    return true;
}

std::pair<uaxidma::acquisition_result, dma_buffer*> uaxidma::get_buffer(int timeout)
{
    if (buffers.empty())
    {
        errno = EAGAIN;
        return {acquisition_result::error, nullptr};
    }

    axidma.clean_interrupt();

    if (!axidma.is_buffer_complete(buffers.peek_next().descriptor_))
    {
        auto poll_ret = axidma.poll_interrupt(timeout);
        if (poll_ret != axi_dma::acquisition_result::success)
        {
            return {static_cast<acquisition_result>(poll_ret), nullptr};
        }
    }

    dma_buffer& acquired = buffers.acquire();

    // Prepare buffer to check for completion again next time
    axidma.clear_complete_flag(acquired.descriptor_);

    if (direction == transfer_direction::dev_to_mem)
    {
        acquired.set_payload(axidma.get_buffer_len(acquired.descriptor_));
    }

    return {acquisition_result::success, &acquired};
}

bool uaxidma::submit_buffer(dma_buffer &buf)
{
    if (axidma.transfer_buffer(buf.descriptor_, buf.length_))
    {
        return false;
    }

    buffers.release(buf);

    return true;
}
