/**
 * @file uio.c
 * @brief Utils to search and open UIO devices
 * @version 1.0
 */

#include "uio.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @brief Finds and sets the uio device number by device name
 * @param name Name of the UIO device as in the /sys/class/uio/uioN/name file
 * @return false on errors
 */
bool uio_device::find_by_name(const std::string &name)
{
    DIR *dir = opendir("/sys/class/uio");
    if (!dir)
    {
        return false;
    }

    int n;
    dirent *entry;
    while ((entry = readdir(dir)))
    {
        if (sscanf(entry->d_name, "uio%d", &n) != 1)
        {
            continue;
        }

        std::stringstream file_name;
        file_name << "/sys/class/uio/uio" << n << "/name";

        std::ifstream uio_name {file_name.str()};
        if (!uio_name.is_open())
        {
            break;
        }

        std::stringstream buf;
        buf << uio_name.rdbuf();

        if (buf.str().compare(name) == 0)
        {
            uio_name.close();
            closedir(dir);
            number_ = n;
            return true;
        }
    }

    closedir(dir);

    errno = ENOENT;
    return false;
}

uio_device::uio_device(const std::string &name) :
    fd(-1), number_(-1), virt_addr_(nullptr)
{
    if (find_by_name(name))
    {
        std::string uio_path {"/dev/uio" + number_};
        fd = open(uio_path.c_str(), O_RDWR);
    }

    if (fd < 0)
    {
        std::cerr << strerror(errno) << std::endl;
        abort();
    }
}

uio_device::~uio_device()
{
    close(fd);
    unmap();
}

uint8_t *uio_device::map()
{
    virt_addr_ = static_cast<uint8_t *>(mmap(nullptr, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (virt_addr_ == MAP_FAILED)
    {
        return nullptr;
    }

    return virt_addr_;
}

bool uio_device::unmap()
{
    if (munmap(virt_addr_, sysconf(_SC_PAGESIZE)) < 0)
    {
        return false;
    }

    virt_addr_ = nullptr;

    return true;
}
