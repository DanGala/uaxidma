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
    if (len <= capacity_)
    {
        length_ = len;
        return true;
    }

    return false;
}

uaxidma::uaxidma(const std::string& udmabuf_name, size_t udmabuf_size, size_t udmabuf_offset,
                 const std::string& axidma_uio_name, dma_mode mode, transfer_direction direction, size_t buffer_size)

    : axidma(udmabuf_name, udmabuf_size, udmabuf_offset, axidma_uio_name, static_cast<axi_dma::dma_mode>(mode),
             static_cast<axi_dma::transfer_direction>(direction), buffer_size),
      mode(mode),
      direction(direction)
{
}

bool uaxidma::initialize()
{
    if (!axidma.initialize() || !axidma.start())
    {
        return false;
    }

    buffers.resize(axidma.get_buffer_count());

    auto buf_iter = axidma.get_chain_iterator();
    for (auto& buf : buffers)
    {
        buf.capacity_ = axidma.get_buffer_size();
        buf.pdescriptor_ = &(*buf_iter);
        buf.data_ = axidma.get_virt_buffer_pointer(buf.pdescriptor_);
        buf_iter++;
    }

    next_buffer = buffers.begin();
    free_buffers = axidma.get_buffer_count();

    return true;
}

std::pair<uaxidma::acquisition_result, dma_buffer*> uaxidma::get_buffer(int timeout)
{
    if (!free_buffers)
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

    if (!axidma.is_buffer_complete(next_buffer->pdescriptor_))
    {
        auto poll_ret = axidma.poll_interrupt(timeout);
        if (poll_ret != axi_dma::acquisition_result::success)
        {
            return {static_cast<acquisition_result>(poll_ret), nullptr};
        }
    }

    dma_buffer& acquired = *next_buffer;

    // Prepare buffer to check for completion again next time
    axidma.clear_complete_flag(acquired.pdescriptor_);

    // Setup buffer with transfer details from DMA based on the buffer id
    if (direction == transfer_direction::dev_to_mem)
    {
        acquired.set_payload(axidma.get_buffer_len(acquired.pdescriptor_));
    }
    else
    {
        acquired.set_payload(0);
    }

    // loop around container
    if (++next_buffer == buffers.end()) next_buffer = buffers.begin(); 
    if (direction == transfer_direction::mem_to_dev)
    {
        free_buffers--;
    }

    return {acquisition_result::success, &acquired};
}

bool uaxidma::submit_buffer(const dma_buffer *buf)
{
    if (axidma.transfer_buffer(buf->pdescriptor_, buf->length_))
    {
        return false;
    }

    // Update buffer pool stats
    free_buffers++;

    return true;
}
