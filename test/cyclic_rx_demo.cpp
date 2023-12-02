#include "uaxidma.h"
#include <iostream>

using acq_result = uaxidma::acquisition_result;
using mode = uaxidma::dma_mode;
using dir = uaxidma::transfer_direction;

static constexpr int timeout_1ms = 1000;
static constexpr size_t _256MiB = 256UL << 10;

int main()
{
    uaxidma dma { "udmabuf0", 0, "axidma_rx", mode::cyclic, dir::dev_to_mem, _256MiB };

    dma.initialize();

    while (true)
    {
        const auto [res, buf_ptr] = dma.get_buffer(timeout_1ms);

        if (res == acq_result::error)
        {
            std::cout << "internal error!" << std::endl;
        }
        else if (res == acq_result::timeout)
        {
            std::cout << "acquisition timed-out!" << std::endl;
        }
        else
        {
            size_t len = buf_ptr->length();
            const uint8_t *data = buf_ptr->data();
            for (size_t i = 0; i < len; i++)
            {
                std::cout << i << ": " << data[i] << std::endl;
            }
            dma.mark_reusable(*buf_ptr);
        }
    }

    return 0;
}
