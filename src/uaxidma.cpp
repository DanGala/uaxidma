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

void dma_buffer_list::add(const std::shared_ptr<dma_buffer>& ptr)
{
    buffers_.push_back(ptr);
    if (!available_++) next_ = buffers_.begin();
}

bool dma_buffer_list::empty() const
{
    return (limit_refs_ && !available_);
}

const std::shared_ptr<dma_buffer> &dma_buffer_list::peek_next() const
{
    return *next_;
}

std::shared_ptr<dma_buffer> dma_buffer_list::acquire()
{
    if (limit_refs_) available_--;
    auto ptr = *next_;
    if (++next_ == buffers_.end()) next_ = buffers_.begin();
    return ptr;
}

void dma_buffer_list::release(std::shared_ptr<dma_buffer>& ptr)
{
    (void)ptr;
    if (limit_refs_) available_++;
}

uaxidma::uaxidma(const std::string& udmabuf_name, size_t udmabuf_size, size_t udmabuf_offset,
                 const std::string& axidma_uio_name, dma_mode mode, transfer_direction direction, size_t buffer_size)

    : axidma{udmabuf_name, udmabuf_size, udmabuf_offset, axidma_uio_name, static_cast<axi_dma::dma_mode>(mode),
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

    for (auto& desc : axidma.sg_desc_chain)
    {
        buffers.add(std::make_shared<dma_buffer>(
            axidma.get_virt_buffer_pointer(desc),
            axidma.get_buffer_size(),
            desc
        ));
    }

    return true;
}

std::pair<uaxidma::acquisition_result, std::shared_ptr<dma_buffer>> uaxidma::get_buffer(int timeout)
{
    if (buffers.empty())
    {
        // Cannot get any more buffers without calling submit_buffer()
        errno = EAGAIN;
        return {acquisition_result::error, nullptr};
    }

    /* In cyclic mode, any number of buffers may have been completed in between calls.
       If a buffer cannot be returned immediately (is in progress), wait for the
       next interrupt (notification of buffer completion).
       Stale interrupts must be cleared BEFORE checking for immediate availability,
       in order not to mask events happening in between checking for availability
       and waiting for an interrupt. */
    axidma.clean_interrupt();

    if (!axidma.is_buffer_complete(buffers.peek_next()->descriptor_))
    {
        auto poll_ret = axidma.poll_interrupt(timeout);
        if (poll_ret != axi_dma::acquisition_result::success)
        {
            return {static_cast<acquisition_result>(poll_ret), nullptr};
        }
    }

    std::shared_ptr<dma_buffer> acquired = buffers.acquire();

    // Prepare buffer to check for completion again next time
    axidma.clear_complete_flag(acquired->descriptor_);

    if (direction == transfer_direction::dev_to_mem)
    {
        acquired->set_payload(axidma.get_buffer_len(acquired->descriptor_));
    }

    return {acquisition_result::success, acquired};
}

bool uaxidma::submit_buffer(std::shared_ptr<dma_buffer> &ptr)
{
    if (axidma.transfer_buffer(ptr->descriptor_, ptr->length_))
    {
        return false;
    }

    buffers.release(ptr);

    return true;
}
