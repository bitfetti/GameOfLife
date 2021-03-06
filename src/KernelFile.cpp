#include "../inc/KernelFile.hpp"

#if defined(_WIN32) || defined(__CYGWIN__)
#include <direct.h>
#define GETCWD _getcwd
#else
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#define GETCWD ::getcwd
#endif

std::string getCurrentDir() {
    const   size_t  pathSize = 4096;
    char    currentDir[pathSize];

    // Check if we received the path
    if (GETCWD(currentDir, pathSize) != NULL) {
        return std::string(currentDir);
    }

    return  std::string("");
}

bool KernelFile::open(const char* fileName) {
    size_t      size;
    char*       str;

    // Open file stream
    std::fstream f(fileName, (std::fstream::in | std::fstream::binary));

    // Check if we have opened file stream
    if (f.is_open()) {
        size_t  sizeFile;
        // Find the stream size
        f.seekg(0, std::fstream::end);
        size = sizeFile = f.tellg();
        f.seekg(0, std::fstream::beg);

        str = new char[size + 1];
        if (!str) {
            f.close();
            return  NULL;
        }

        // Read file
        f.read(str, sizeFile);
        f.close();
        str[size] = '\0';

        source_  = str;

        return true;
    }

    return false;
}