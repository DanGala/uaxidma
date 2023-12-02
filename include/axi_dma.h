#ifndef _AXI_DMA_H
#define _AXI_DMA_H

#include "register_flags.h"
#include "sg_descriptor.h"
#include "udmabuf.h"
#include "uio.h"
#include <cstddef>
#include <cstdint>
#include <sys/poll.h>

class axi_dma
{
public:

    enum class transfer_direction
    {
        mm2s = 0,
        s2mm = 1
    };

    enum class dma_mode
    {
        normal = 0,
        cyclic = 1
    };

    enum class acquisition_result
    {
        success = 1,
        error = -1,
        timeout = 0
    };

    explicit axi_dma(const std::string& udmabuf_name, size_t udmabuf_size, size_t udmabuf_offset, const std::string& uio_device_name,
            dma_mode mode, transfer_direction direction, size_t buffer_size);
    axi_dma() = delete;
    ~axi_dma();
    bool initialize();
    bool start();
    void clean_interrupt();
    acquisition_result poll_interrupt(int timeout);
    bool transfer_buffer(sg_descriptor &desc, size_t len);
    bool is_buffer_complete(const sg_descriptor &desc) const;
    void clear_complete_flag(sg_descriptor &desc);
    size_t get_buffer_size() const;
    size_t get_buffer_len(const sg_descriptor &desc) const;
    uint8_t *get_virt_buffer_pointer(const sg_descriptor &desc) const;

    sg_descriptor_chain sg_desc_chain;   //!< Scatter/Gather descriptor chain

private:

    /**
     * @brief DMA control register flags
     */
    enum class dmacontrolf : uint32_t
    {
        rs = 1u << 0,
        reset = 1u << 2,
        keyhole = 1u << 3,
        cyclic_bd_en = 1u << 4,
        ioc_irq_en = 1u << 12,
        dly_irq_en = 1u << 13,
        err_irq_en = 1u << 14,
        all_irq_en = (ioc_irq_en | dly_irq_en | err_irq_en),
        irq_thresh = 0xffu << 16,
        irq_delay = 0xffu << 24,
        all = 0xffffffff
    };

    /**
     * @brief Thin wrapper around a dmacontrolf object to provide field setters and getters
     */
    struct dmacontrolf_wrapper : public flags_wrapper<dmacontrolf>
    {
        dmacontrolf_wrapper(dmacontrolf& f) : flags_wrapper<dmacontrolf>{f} {}
        void set_irq_threshold(uint32_t thresh) { set_flags(dmacontrolf::irq_thresh & thresh); }
    };

    /**
     * @brief Thin wrapper around a volatile dmacontrolf object to provide field setters and getters
     */
    struct vdmacontrolf_wrapper : public vflags_wrapper<dmacontrolf>
    {
        vdmacontrolf_wrapper(volatile dmacontrolf& f) : vflags_wrapper<dmacontrolf>{f} {}
        void set_irq_threshold(uint32_t thresh) volatile { set_flags(dmacontrolf::irq_thresh & thresh); }
    };

    /**
     * @brief DMA status register flags
     */
    enum class dmastatusf : uint32_t
    {
        halted = 1u << 0,
        idle = 1u << 1,
        sg_incld = 1u << 3,
        dma_int_err = 1u << 4,
        dma_slv_err = 1u << 5,
        dma_dec_err = 1u << 6,
        dma_errors = (dma_int_err | dma_slv_err | dma_dec_err),
        sg_int_err = 1u << 8,
        sg_slv_err = 1u << 9,
        sg_dec_err = 1u << 10,
        sg_errors = (sg_int_err | sg_slv_err | sg_dec_err),
        all_errors = (dma_errors | sg_errors),
        ioc_irq = 1u << 12,
        dly_irq = 1u << 13,
        err_irq = 1u << 14,
        all_irqs = (ioc_irq | dly_irq | err_irq),
        all = 0xffffffff
    };

    /**
     * @brief Thin wrapper around a dmastatusf object to provide field setters and getters
     */
    struct dmastatusf_wrapper : public flags_wrapper<dmastatusf>
    {
        dmastatusf_wrapper(dmastatusf& f) : flags_wrapper<dmastatusf>{f} {}
    };

    /**
     * @brief Thin wrapper around a volatile dmastatusf object to provide field setters and getters
     */
    struct vdmastatusf_wrapper : public vflags_wrapper<dmastatusf>
    {
        vdmastatusf_wrapper(volatile dmastatusf& f) : vflags_wrapper<dmastatusf>{f} {}
    };

    /**
     * @brief Scatter/Gather Mode Register Address Map
     */
    struct sg_registers
    {
        dmacontrolf control;        //!< DMA Control register
        dmastatusf status;          //!< DMA Status register
        uint32_t current_desc_low;  //!< Current Descriptor Pointer. Lower 32 bits of the address.
        uint32_t current_desc_high; //!< Current Descriptor Pointer. Higher 32 bits of the address.
        uint32_t tail_desc_low;     //!< Tail Descriptor Pointer. Lower 32 bits of the address;
        uint32_t tail_desc_high;    //!< Tail Descriptor Pointer. Higher 32 bits of the address;
    };

    /**
     * @brief Full Scatter/Gather AXI DMA Mermory Map
     */
    struct memory_map
    {
        sg_registers mm2s; //!< MM2S registers
        uint32_t sg_ctl;          //!< Scatter/Gather User and Cache
        sg_registers s2mm; //!< S2MM registers
    };

    u_dma_buf udmabuf;                   //!< Associated u-dma-buf buffer
    uio_device uio;                      //!< AXI DMA UIO device
    dma_mode mode;                       //!< Operational Mode
    transfer_direction direction;        //!< Channel direction
    size_t buffer_size;                  //!< Scatter/Gather buffer size
    uint8_t *buffers;                    //!< Scatter/Gather buffers
    volatile memory_map *registers_base; //!< Memory mapped AXI DMA registers
    pollfd fds;                          //!< Used for polling the UIO device interrupt file descriptor

    bool stop();
    bool reset();
    bool mask_interrupt();
    bool unmask_interrupt();
    void create_desc_ring(std::size_t buffer_count);
    bool start_normal();
    bool start_cyclic();
};

#endif //#ifndef _AXI_DMA_H