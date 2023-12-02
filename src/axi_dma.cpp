#include "axi_dma.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static inline constexpr uint32_t lower_32_bits(uintmax_t x) { return x; }
static inline constexpr uint32_t upper_32_bits(uintmax_t x) { return (x >> 32); }
static inline constexpr uintmax_t real_address(uintmax_t upper, uintmax_t lower) { return ((upper << 32) | lower); }

/**
 * @brief Mask AXI DMA interrupt file descriptor
 * @return false on errors
 */
bool axi_dma::mask_interrupt()
{
    static constexpr int32_t mask = 0;
    return (write(uio.fd, &mask, sizeof(mask)) != sizeof(mask));
}

/**
 * @brief Unmask AXI DMA interrupt file descriptor
 * @return false on errors
 */
bool axi_dma::unmask_interrupt()
{
    static constexpr int32_t unmask = 1;
    return (write(uio.fd, &unmask, sizeof(unmask)) != sizeof(unmask));
}

/**
 * @brief Create a chain of Scatter/Gather descriptors and intialize their structures
 */
void axi_dma::create_desc_ring(std::size_t buffer_count)
{
    sg_desc_chain = {reinterpret_cast<sg_descriptor *>(udmabuf.virt_addr), buffer_count};
    
    const uintptr_t desc_phys_base_addr = udmabuf.phys_addr;
    uintptr_t next_desc = desc_phys_base_addr + sizeof(sg_descriptor);
    uintptr_t buf_addr = desc_phys_base_addr + sg_desc_chain.size();

    for (auto& d : sg_desc_chain)
    {
#if (__WORDSIZE == 64)
        d.next_desc_msb = upper_32_bits(next_desc);
        d.buf_addr_msb = upper_32_bits(buf_addr);
#endif // #if (__WORDSIZE == 64)

        d.next_desc = lower_32_bits(next_desc);
        d.buf_addr = lower_32_bits(buf_addr);

        controlf_wrapper control{d.control};
        statusf_wrapper status{d.status};
        
        control.set_buf_len(buffer_size);
        status.clear_flags(statusf::all);

        if (direction == transfer_direction::mm2s)
        {
            control.set_flags(controlf::sof | controlf::eof); // Each AXI packet sent occupies exactly 1 DMA buffer
            status.set_flags(statusf::complete); // Stall until the PS is ready to transmit
        }

        next_desc += sizeof(sg_descriptor);
        buf_addr += buffer_size;
    }

    // Create a ring by pointing the last descriptor back to the first
    auto& last_desc_ptr = --(sg_desc_chain.end());
#if (__WORDSIZE == 64)
    last_desc_ptr->next_desc_msb = upper_32_bits(desc_phys_base_addr);
#endif // #if (__WORDSIZE == 64)
    last_desc_ptr->next_desc = lower_32_bits(desc_phys_base_addr);
}

/**
 * @brief C'tor
 */
axi_dma::axi_dma(const std::string& udmabuf_name, size_t udmabuf_size, size_t udmabuf_offset,
                 const std::string& uio_device_name, dma_mode mode, transfer_direction direction,
                 size_t buffer_size)

    : udmabuf(udmabuf_name, udmabuf_size, udmabuf_offset),
      uio(uio_device_name),
      mode(mode),
      direction(direction),
#ifdef USE_DATA_REALIGNMENT_ENGINE
      buffer_size(buffer_size)
#else
    // Ensure buffer address is bus width aligned (AXI-4 bus = 64 bit (8 byte))
      buffer_size((buffer_size % 8UL) ? (buffer_size + 8UL - buffer_size % 8UL) : buffer_size)
#endif
{
}

/**
 * @brief Initialize the AXI DMA instance and map its registers table into user space memory
 * @return false on errors
 */
bool axi_dma::initialize()
{
    if (buffer_size > sg_max_buf_len)
    {
        abort();
    }

    // Initialize poll structure to monitor interrupts
    fds.fd = uio.fd;
    fds.events = POLLIN;

    // Map AXI DMA peripheral to virtual address space
    registers_base = reinterpret_cast<volatile memory_map *>(uio.map());
    if (!registers_base)
    {
        return false;
    }

    volatile sg_registers& registers = (direction == transfer_direction::mm2s)
                                        ? registers_base->mm2s : registers_base->s2mm;

    // Ensure that the Scatter Gather Engine is included and the AXI DMA is configured for Scatter Gather mode
    vdmastatusf_wrapper status{registers.status};
    if (status.check_flags(dmastatusf::sg_incld))
    {
        return false;
    }

    // Create the descriptor chain
    // Buffer descriptors are located at the base of the u-dma-buf device's physical/virtual memory,
    // while their associated data buffers are located immediately after the last descriptor
    std::size_t buffer_count = (udmabuf.size / (buffer_size + sizeof(sg_descriptor)));
    if (buffer_count == 0)
    {
        // Application logic error: can't fit a single BD/buffer pair in the udmabuf
        abort();
    }

    create_desc_ring(buffer_count);

    buffers = udmabuf.virt_addr + sg_desc_chain.size();

    return true;
}

/**
 * @brief Deinitialize an AXI DMA instance
 */
axi_dma::~axi_dma()
{
    reset();
    uio.unmap();
}

/**
 * @brief Prepares the AXI DMA to start in Scatter/Gather mode with IOC interrupt enabled
 * @return false on errors
 */
bool axi_dma::start_normal()
{
    // Just in case, let's start from a known state
    if (!reset())
    {
        return false;
    }

    volatile sg_registers& registers = (direction == transfer_direction::mm2s)
                                        ? registers_base->mm2s : registers_base->s2mm;

    // Prepare control word for starting the DMA channel and generating one interrupt for each BD
    // completed Note that non-cyclic operation will be configured: the DMA will stall when all
    // buffer descriptors are complete
    vdmacontrolf_wrapper control{registers.control};
    control.set_flags(dmacontrolf::ioc_irq_en | dmacontrolf::err_irq_en);
    control.set_irq_threshold(1u);

    // Set current descriptor pointer to the first descriptor
    const uintptr_t first_desc = udmabuf.phys_addr;

#if (__WORDSIZE == 64)
    registers.current_desc_high, upper_32_bits(first_desc);
#endif // #if (__WORDSIZE == 64)

    registers.current_desc_low, lower_32_bits(first_desc);

    // Start AXI DMA but don't set the tail descriptor yet
    control.set_flags(dmacontrolf::rs);

    return true;
}

/**
 * @brief Starts the AXI DMA in cyclic mode with IOC interrupt enabled
 * @return false on errors
 */
bool axi_dma::start_cyclic()
{
    // Just in case, let's start from a known state
    if (!reset())
    {
        return false;
    }

    volatile sg_registers& registers = registers_base->s2mm;

    // Prepare control word for starting the cyclic DMA channel and generating one interrupt for each BD completed
    vdmacontrolf_wrapper control{registers.control};
    control.set_flags(dmacontrolf::cyclic_bd_en | dmacontrolf::ioc_irq_en | dmacontrolf::err_irq_en);
    control.set_irq_threshold(1u);

    // Set current descriptor pointer to the first descriptor
    const uintptr_t first_desc = udmabuf.phys_addr;

#if (__WORDSIZE == 64)
    registers.current_desc_high = upper_32_bits(first_desc);
#endif // #if (__WORDSIZE == 64)

    registers.current_desc_low = lower_32_bits(first_desc);

    // Start DMA channel
    control.set_flags(dmacontrolf::rs);

    // Set tail descriptor pointer:
    // According to the programming sequence described in the IP Product Guide, the actual value
    // written to it does not serve any purpose, and is only used to trigger the DMA to start
    // fetching the BDs. Recommendation is to program it with some value which is not part of the BD
    // chain.
#if (__WORDSIZE == 64)
    registers.tail_desc_high = 0U;
#endif // #if (__WORDSIZE == 64)

    // Interrupts shall be unmasked right before calling poll()
    if (!mask_interrupt())
    {
        return false;
    }

#ifdef __ARM_ARCH
    asm volatile("dmb st");
#endif

    registers.tail_desc_low = 0xFFFFFFFFU;

    return true;
}

/**
 * @brief Starts AXI DMA operation
 * @return false on errors
 */
bool axi_dma::start()
{
    switch (mode)
    {
        case dma_mode::cyclic:
            return start_cyclic();
        case dma_mode::normal:
            return start_normal();
        default:
            abort();
    }
}

/**
 * @brief Stops the ongoing AXI DMA operation
 * @return false on errors
 */
bool axi_dma::stop()
{
    volatile sg_registers& registers = (direction == transfer_direction::mm2s)
                                        ? registers_base->mm2s : registers_base->s2mm;

    // Reset the RS bit in the control register and wait a bit for the DMA channel to be halted
    vdmacontrolf_wrapper control{registers.control};
    control.clear_flags(dmacontrolf::rs);

    unsigned int spin_count = 128U;
    vdmastatusf_wrapper status{registers.status};
    while (!status.check_flags(dmastatusf::halted))
    {
        if (--spin_count == 0)
        {
            return false;
        }
    }

    // Memory barrier to ensure control register is updated before following operations depending on
    // AXI DMA being stopped are executed
#ifdef __ARM_ARCH
    asm volatile("dmb sy");
#endif

    return true;
}

/**
 * @brief Triggers an AXI DMA core soft reset and blocks until it's completed
 * @return false on errors
 */
bool axi_dma::reset()
{
    // Set the Reset bit in the control register to soft-reset the DMA engine
    // It doesn't really matter whether the Reset bit is set on S2MM_DMACR or MM2S_DMACR,
    // as the whole AXI DMA will be reset.
    vdmacontrolf_wrapper control{registers_base->mm2s.control};
    control.set_flags(dmacontrolf::reset);

    // While the reset is in progress, the Reset bit remains high
    unsigned int spin_count = 128U;
    while (control.check_flags(dmacontrolf::reset))
    {
        if (--spin_count == 0)
        {
            return false;
        }
    }

    // Memory barrier to ensure control register is updated before following operations depending on
    // AXI DMA being reset are executed
#ifdef __ARM_ARCH
    asm volatile("dmb sy");
#endif

    return true;
}

/**
 * @brief Clears AXI DMA's interrupt flags from the DMASR register
 */
void axi_dma::clean_interrupt()
{
    volatile sg_registers& registers = (direction == transfer_direction::mm2s)
                                        ? registers_base->mm2s : registers_base->s2mm;

    vdmastatusf_wrapper status{registers.status};
    status.set_flags(dmastatusf::ioc_irq | dmastatusf::err_irq);

    // Memory barrier to ensure IRQs are cleared before following operations assuming a clean slate
#ifdef __ARM_ARCH
    asm volatile("dmb st");
#endif
}

/**
 * @brief Poll the AXI DMA interrupt
 * @param timeout Allow 'timeout' millisecods for an event to occur. Set to -1 to block indefinitely
 * @return AXI_DMA_POLL_SUCCESS on success, AXI_DMA_POLL_ERROR on errors, AXI_DMA_POLL_TIMEOUT on timeout
 */
axi_dma::acquisition_result axi_dma::poll_interrupt(int timeout)
{
    unmask_interrupt();
    acquisition_result ret = static_cast<acquisition_result>(poll(&fds, 1, timeout));

    if (ret == acquisition_result::error)
    {
        switch (errno)
        {
            case EINTR:
            case EAGAIN:
                // Try again, let's pretend no time has elapsed
                return poll_interrupt(timeout);
            default:
                return ret;
        }
    }
    
    if (ret == acquisition_result::success)
    {
        // Blocking wait for a DMA interrupt - this should return immediately as we are only polling one fd
        int32_t n_interrupts;
        if (read(uio.fd, &n_interrupts, sizeof(n_interrupts))
            != sizeof(n_interrupts))
        {
            ret = acquisition_result::error;
        }
    }

    // Avoid speculatively doing any work before the interrupt returns
#ifdef __ARM_ARCH
    asm volatile("dmb sy");
#endif

    return ret;
}

/**
 * @brief Starts the AXI DMA transfer of the specified buffer descriptor
 * @param desc Buffer descriptor
 * @param len Transfer length
 * @return false on errors
 */
bool axi_dma::transfer_buffer(sg_descriptor &desc, size_t len)
{
    controlf_wrapper control{desc.control};
    control.set_flags(controlf::sof | controlf::eof);
    control.set_buf_len(len);

    statusf_wrapper status{desc.status};
    status.clear_flags(statusf::complete | statusf::dma_errors);

    // Update tail descriptor to point to the current buffer descriptor
    ptrdiff_t desc_offset = sg_descriptor_chain::distance(sg_desc_chain.begin(), sg_descriptor_chain::sg_desc_iterator{&desc});
    const uintptr_t tail_desc = udmabuf.phys_addr + sizeof(sg_descriptor) * desc_offset;

#if (__WORDSIZE == 64)
    registers_base->mm2s.tail_desc_high = upper_32_bits(tail_desc);
#endif // #if (__WORDSIZE == 64)

    // Memory barrier to ensure tail descriptor is not set before buffers and sg_desc_chain have been written in memory
#ifdef __ARM_ARCH
    asm volatile("dmb st");
#endif

    registers_base->mm2s.tail_desc_low = lower_32_bits(tail_desc);

    return true;
}

/**
 * @brief Checks if the buffer has been completed
 * @param desc Buffer descriptor
 * @return true for completed transfers, false otherwise
 */
bool axi_dma::is_buffer_complete(const sg_descriptor &desc) const
{
    cstatusf_wrapper status{desc.status};
    bool complete = status.check_flags(statusf::complete);
    if (complete)
    {
        // Avoid speculatively doing any work before the status is actually read
        // Note that for this function, acquire semantics apply when the buffer has been completed
#ifdef __ARM_ARCH
        asm volatile("dmb sy");
#endif
    }
    return complete;
}

/**
 * @brief Reset the buffer complete bit in the Scatter Gather descriptor's status register
 * @param desc Buffer descriptor
 */
void axi_dma::clear_complete_flag(sg_descriptor &desc)
{
    statusf_wrapper status{desc.status};
    status.clear_flags(statusf::complete);
#ifdef __ARM_ARCH
    asm volatile("dmb st");
#endif
}

/**
 * @brief Get the maximum amount of data that can be stored in this buffer
 * @return size in bytes
 */
size_t axi_dma::get_buffer_size()
{
    return buffer_size;
}

/**
 * @brief Get the amount of data transferred by the buffer described by a struct sg_descriptor
 * @param desc Buffer descriptor
 * @return length in bytes
 */
size_t axi_dma::get_buffer_len(const sg_descriptor &desc) const
{
    cstatusf_wrapper status{desc.status};
    const size_t len = status.get_xfer_bytes();
    // Avoid speculatively doing any work before the status is actually read
#ifdef __ARM_ARCH
    asm volatile("dmb sy");
#endif
    return len;
}

/**
 * @brief Get a pointer to the start of a buffer described by a struct sg_descriptor in virtual memory
 * @param desc Buffer descriptor
 * @return a pointer to virtual memory
 */
uint8_t *axi_dma::get_virt_buffer_pointer(const sg_descriptor &desc)
{
    return buffers + (sg_descriptor_chain::distance(sg_desc_chain.const_begin(), sg_descriptor_chain::sg_desc_const_iterator{&desc})) * buffer_size;
}
