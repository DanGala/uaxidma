#include "uaxidma.h"
#include <iostream>
#include <cstring>

static constexpr uint8_t secret[] = {4, 8, 15, 16, 23, 42};

using acq_result = uaxidma::acquisition_result;
using mode = uaxidma::dma_mode;
using dir = uaxidma::transfer_direction;

int main()
{
    uaxidma dma { "udmabuf1", 0, "axidma_tx", mode::normal, dir::mem_to_dev, 256UL << 10 };

    dma.initialize();

    const auto [res, buf_ptr] = dma.get_buffer(1000);

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
        std::memcpy(buf_ptr->data(), secret, sizeof(secret));
        buf_ptr->set_payload(sizeof(secret));
        dma.submit_buffer(*buf_ptr);
    }

    return 0;
}
