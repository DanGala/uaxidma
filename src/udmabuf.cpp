/**
 * @file udmabuf.c
 * @brief A small module to find u-dma-buf buffers by name and mmap them
 * @version 1.0
 */

#include "udmabuf.h"

#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <inttypes.h>
#include <cstdio>
#include <sstream>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

/**
 * @brief Initialize the udmabuf instance and map the underlying buffer into user space memory
 * @param name Absolute path to the udmabuf device node in the /dev directory
 */
u_dma_buf::u_dma_buf(const std::string& name, size_t size)
{
    phys_addr = static_cast<uintptr_t>(read_property("/sys/class/u-dma-buf/" + name + "/phys_addr", "%" SCNxPTR));
    if (phys_addr == 0)
    {
        abort();
    }

    size_t max_size = read_property("/sys/class/u-dma-buf/" + name + "/size", "%" SCNd64);
    if ((max_size == 0) || (size > max_size))
    {
        abort();
    }

    size = (size != 0) ? size : max_size;

    map("/dev/" + name);
}

/**
 * @brief Read the udmabuf node's property file from the system and return its contents as an
 * integer
 * @param file_path Absolute path to the udmabuf device node's property in the /sys/class directory
 * @note file_path must be a nul-terminated string
 * @return unsigned integer property value, 0 on errors
 */
uint64_t u_dma_buf::read_property(const std::string& file_path, const std::string& format)
{
    std::ifstream t(file_path);
    std::stringstream buffer;
    buffer << t.rdbuf();

    uint64_t value;
    if (sscanf(buffer.str().c_str(), format.c_str(), &value) != 1)
    {
        return 0;
    }

    return value;
}

/**
 * @brief Maps a memory region assigned to a udmabuf node into user space memory
 * @param udmabuf Pointer to udmabuf instance
 */
void u_dma_buf::map(const std::string& file)
{
    int fd = open(file.c_str(), O_RDWR);
    if (fd < 0)
    {
        abort();
    }

    virt_addr
        = static_cast<uint8_t *>(mmap(nullptr, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0));
    if (virt_addr == MAP_FAILED)
    {
        abort();
    }

    close(fd);
}
