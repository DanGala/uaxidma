#ifndef _UAXIDMA_H
#define _UAXIDMA_H

#include "axi_dma.h"
#include "udmabuf.h"
#include <cstdint>
#include <memory>
#include <vector>

class dma_buffer
{
friend class uaxidma;
public:
    dma_buffer(uint8_t *data, size_t max_len, sg_descriptor& desc)
        : data_(data), length_(0), capacity_(max_len), desc_handle_{desc} {}
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
    uint8_t *data_;
    size_t length_;
    size_t capacity_;
    sg_descriptor_handle desc_handle_;
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
     * starting from the udmabuf buffer base address.
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
    uaxidma(const std::string& udmabuf_name, size_t udmabuf_size, const std::string& axidma_uio_name,
            dma_mode mode, transfer_direction direction, size_t buffer_size);

    bool initialize();

    /**
     * @brief Acquires the next buffer from the list
     * In mem_to_dev transfers, the user must first call this function, then write the
     * contents into the buffer and set the data length, and finally call submit_buffer().
     * In dev_to_mem transfers, the user must first call this function, then process the
     * data received, and finally call mark_reusable().
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
     * @brief Returns buffer ownership to the DMA library
     * @note To be used only when direction has been set to dev_to_mem
     */
    void mark_reusable(dma_buffer &buf);

    /**
     * @brief Submits a buffer for transmission to the device end-point
     * @note To be used only when direction has been set to mem_to_dev
     */
    void submit_buffer(dma_buffer &buf);

private:

    class dma_buffer_ring
    {
    public:
        /**
         * @brief Construct an empty list of dma_buffer pointers
         */
        dma_buffer_ring() = default;
        /**
         * @brief Construct an empty list of dma_buffer pointers with reference limits enabled
         */
        dma_buffer_ring(bool limit_refs);
        /**
         * @brief Sets the maximum number of buffers in the list
         */
        void initialize(size_t capacity);
        /**
         * @brief Inserts a new element at the end of the list
         */
        void add(const dma_buffer& ptr);
        /**
         * @brief Returns true if the number of available buffers is zero, false otherwise
         */
        bool empty() const;
        /**
         * @brief Provides a read-only view of the next available buffer from the list
         */
        const dma_buffer &peek_next() const;
        /**
         * @brief Obtains the next available buffer. The buffer returned won't be available again until released.
         */
        dma_buffer& acquire();
        /**
         * @brief Releases a buffer, making it available for future use
         */
        void release(dma_buffer& ptr);
    private:
        std::vector<dma_buffer> buffers_;
        std::vector<dma_buffer>::iterator next_;
        std::size_t available_;
        bool limit_refs_;
    };

    axi_dma axidma;
    dma_mode mode;
    transfer_direction direction;
    dma_buffer_ring buffers;
};

#endif // #ifndef _DMA_H
