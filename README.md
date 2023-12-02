# uaxidma (User space Scatter/Gather AXI DMA library)
A userspace library based on UIO and u-dma-buf, aimed to manage interrupt-driven Scatter/Gather data transfers between PS and PL.

# Dependencies
- [User space I/O](https://www.kernel.org/doc/html/v4.12/driver-api/uio-howto.html)
- [User space mappable DMA Buffer](https://github.com/ikwzm/udmabuf)

# Examples
This library relies on access to sysfs to locate and manage a system's UIO and u-dma-buf devices in the device tree.

## Device tree file (simplified)
```
udmabuf0 {
    compatible = "ikwzm,u-dma-buf";
    device-name = "udmabuf0";
    minor-number = <0>;
    size = <0x00100000>;
    dma-coherent;
};

udmabuf1 {
    compatible = "ikwzm,u-dma-buf";
    device-name = "udmabuf0";
    minor-number = <0>;
    size = <0x00100000>;
    sync-mode = <3>;
    sync-always;
};

axidma_rx@40000000 {
    compatible = "generic-uio";
    interrupt-parent = <&intc>;
    interrupts = <0 29 4>;
    interrupt-names = "s2mm_introut";
    reg = <0x40400000 0x10000>;
    xlnx,include-sg ;
}

axidma_tx@40010000 {
    compatible = "generic-uio";
    interrupt-parent = <&intc>;
    interrupts = <0 30 4>;
    interrupt-names = "mm2s_introut";
    reg = <0x40400000 0x10000>;
    xlnx,include-sg ;
}

```

## Transfer data from PL to userspace in cyclic DMA mode
```cpp
#include "uaxidma.h"
#include <iostream>

using acq_result = uaxidma::acquisition_result;
using mode = uaxidma::dma_mode;
using dir = uaxidma::transfer_direction;

int main()
{
    uaxidma dma { "udmabuf0", 0, "axidma_rx", mode::cyclic, dir::dev_to_mem, 256UL << 10 };

    dma.initialize();

    while (true)
    {
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
            size_t len = buf_ptr->length();
            const uint8_t *data = buf_ptr->data();
            for (size_t i = 0; i < len; i++)
            {
                std::cout << i << ": " << data[i] << std::endl;
            }
        }
    }

    return 0;
}
```

## Transfer data from userspace memory to PL
```cpp
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
```
