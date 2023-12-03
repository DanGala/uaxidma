#ifndef _UIO_H
#define _UIO_H

#include <cstdint>
#include <string>

class uio_device
{
public:
    /**
     * @brief Construct a new UIO device
     * @param name the UIO device property in the /sys/class/uio/uioN/name file should match this name
     * @note Aborts execution if the device is not found
     */
    uio_device(const std::string &name);

    /**
     * @brief Destroy the uio device releasing associated system resources
     */
    ~uio_device();

    /**
     * @brief Creates a virtual mapping of the UIO device's memory space
     * @return virtual address where the device is mapped, nullptr if map failed
     */
    uint8_t *map();

    /**
     * @brief Deallocates the virtual mapping of the UIO device's memory
     * @return false on errors 
     */
    bool unmap();

    int fd; //!< File descriptor number associated to the device

private:
    /**
     * @brief Finds and sets the uio device number by device name
     * @param name Name of the UIO device as in the /sys/class/uio/uioN/name file
     * @return false on errors
     */
    bool find_by_name(const std::string &name);

    int number_; //!< UIO device number as found in /dev/uio<N> and /sys/class/uio/uio<N>
    uint8_t *virt_addr_; //!< Virtual mapping address
};

#endif /* _UIO_H */
