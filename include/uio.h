#ifndef _UIO_H
#define _UIO_H

#include <cstdint>
#include <string>

class uio_device
{
public:
    uio_device(const std::string &name);
    ~uio_device();
    uint8_t *map();
    bool unmap();

    int fd;
private:
    bool find_by_name(const std::string &name);

    int number_;
    uint8_t *virt_addr_;
};

#endif /* _UIO_H */
