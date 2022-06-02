//
// Created by Roman Titkov on 22.03.2022.
//

#include "filemap.h"

#include <utility>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;
void handle_error(const char* msg) {
    perror(msg);
    exit(255);
}

char* map_file(const char* fname, size_t& length)
{
    int fd = open(fname, O_RDONLY);
    if (fd == -1)
        handle_error("open");

    struct stat sb;
    if (fstat(fd, &sb) == -1)
        handle_error("fstat");

    length = sb.st_size;

    char* addr = static_cast<char*>(mmap(NULL, length, PROT_READ, MAP_SHARED, fd, 0u));
    if (addr == MAP_FAILED)
        handle_error("mmap");

    close(fd);
    return addr;
}
