#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "../util/exceptions.h"

namespace io {

using FileDescriptor = int32_t;

class File {
    FileDescriptor fd_;
    std::string fileName_;

public:
    File(std::string&& fileName) : fileName_(std::forward<std::string>(fileName)) {
        fd_ = open(fileName_.c_str(), O_RDWR | O_CREAT);
        if (fd_ == -1) {
            throw error::IoError("File could not be opened");
        }
    }
    File(const std::string& fileName) : File(std::string(fileName)) {
    }
    ~File() {
        close(fd_);
    }

    _GLIBCXX_NODISCARD size_t GetSize() {

        // TODO: Adapt for Windows
        struct stat64 file_stat;
        if (fstat64(fd_, &file_stat) != 0) {
            throw error::BadArgument("Failed to get file" + fileName_ + "size.");
        }

        return file_stat.st_size;
    }
};

}  // namespace io