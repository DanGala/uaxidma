#ifndef _UDMABUF_H
#define _UDMABUF_H

#include <cstddef>
#include <cstdint>
#include <string>

class u_dma_buf
{
    public:
        explicit u_dma_buf(const std::string& name, size_t size, size_t offset);
        u_dma_buf() = delete;

        uintptr_t phys_addr;
        uint8_t *virt_addr;
        size_t size;
        
    private:
        uint64_t read_property(const std::string& file_path, const std::string& format);
        void map(const std::string& file);

        size_t offset;
};

#endif //#ifndef _UDMABUF_H