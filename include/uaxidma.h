#ifndef _UAXIDMA_H
#define _UAXIDMA_H

#include <cstdint>
#include <vector>

#include "axi_dma.h"
#include "udmabuf.h"

class dma_buffer
{
friend class uaxidma;
public:
    /**
     * @brief Returns the pointer to the beginning of data
     * @return nullptr on errors
     */
    uint8_t *data();
    /**
     * @brief Returns the number of bytes of data received
     * @return data length
     */
    size_t length();
    /**
     * @brief Sets the number of bytes of data to be sent
     * @param len new data length
     * @return false if len exceeds the buffer's capacity
     */
    bool set_payload(size_t len);
private:
    unsigned int id_;
    uint8_t *data_;
    size_t length_;
    size_t capacity_;
};

class uaxidma
{
public:

    enum class dma_mode
    {
        normal = static_cast<int>(axi_dma::dma_mode::normal),
        cyclic = static_cast<int>(axi_dma::dma_mode::cyclic)
    };

    enum class transfer_direction
    {
        mem_to_dev = static_cast<int>(axi_dma::transfer_direction::mm2s),
        dev_to_mem = static_cast<int>(axi_dma::transfer_direction::s2mm)
    };

    enum class acquisition_result
    {
        success = static_cast<int>(axi_dma::acquisition_result::success),
        error = static_cast<int>(axi_dma::acquisition_result::error),
        timeout = static_cast<int>(axi_dma::acquisition_result::timeout)
    };

    /**
     * @brief Creates a DMA channel.
     * @param udmabuf_name of the udmabuf buffer to use.
     *
     * The corresponding character device shall be /dev/<name>
     * There shall be a directory /sys/class/u-dma-buf/<name>
     * 
     * @param udmabuf_size in bytes of the udmabuf buffer to use.
     * 
     * A value of 0 indicates all memory reserved to the udabuf buffer will be used.
     * A value greater than 0 indicates that <em>udmabuf_size</em> bytes of the udmabuf buffer will be used,
     * starting from <em>udmabuf_offset</em> bytes after the udmabuf buffer base address.
     *
     * @param udmabuf_offset in bytes with respect to the udmabuf buffer base address.
     * 
     * To be used in conjunction with <em>udmabuf_size</em>.
     * This setting shall be ignored when <em>udmabuf_size</em> is set to 0.
     * 
     * @param axidma_uio_name of the UIO device associated to the AXI-DMA.
     *
     * There shall be a directory /sys/class/uio/uio<num>/<name>
     * 
     * @param mode can be a value of @ref mode
     * 
     * @param direction can be a value of @ref direction
     * 
     * @param buffer_size size of each buffer in bytes
     */
    uaxidma(const std::string& udmabuf_name, size_t udmabuf_size, size_t udmabuf_offset, const std::string& axidma_uio_name,
            dma_mode mode, transfer_direction direction, size_t buffer_size);

    bool initialize();

    /**
     * @brief Obtains a free buffer
     * * To make a mem_to_dev transfer, the user must first call this function, then write the
     * contents into the buffer and set the data length, and finally call submit_buffer().
     *
     * @note If buffer submission is deferred (get_buffer() is called more than once without 
     * calling submit_buffer()), the user may get up to <em>N</em> buffers - where the value 
     * of <em>N</em> depends on the size of each buffer and the size of the u-dma-buf memory segment 
     * reserved for the DMA - before the API returns an error and sets errno to EAGAIN.
     * An attempt to submit buffers in a different order than the one in which said buffers were 
     * obtained results in undefined behaviour.
     * @note The semantics of the timeout parameter is the same as for the poll() function.
     *       It is given in milliseconds, and -1 indicates no timeout, while 0 indicates non-blocking behaviour.
     * @param timeout 
     * @return Pair of acquisition_result object representing the success of the operation and pointer to the buffer,
     *         nullptr on error
     */
    std::pair<acquisition_result, dma_buffer*> get_buffer(int timeout);

    /**
     * @brief Submits a buffer.
     * @note To be used only when direction has been set to mem_to_dev
     * @return true on success, false on error
     */
    bool submit_buffer(const dma_buffer *buf);

private:
    axi_dma axidma;
    dma_mode mode;
    transfer_direction direction;
    std::vector<dma_buffer> buffers;
    unsigned int next_buffer_id;
    unsigned int free_buffers;
};

#endif // #ifndef _DMA_H
