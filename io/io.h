#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include "../util/error.h"

namespace io {

using FileDescriptor = int32_t;
using Offset = off_t;

class File {
    FileDescriptor fd_;
    std::string fileName_;

public:
    File(std::string&& fileName) : fileName_(std::forward<std::string>(fileName)) {
        fd_ = open(fileName_.c_str(), O_RDWR | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
        if (fd_ == -1) {
            throw error::IoError("File could not be opened");
        }
    }
    File(const std::string& fileName) : File(std::string(fileName)) {
    }
    ~File() {
        close(fd_);
    }

    _GLIBCXX_NODISCARD size_t GetSize() const {

        // TODO: Adapt for Windows
        struct stat64 file_stat;
        if (fstat64(fd_, &file_stat) != 0) {
            throw error::BadArgument("Failed to get file " + fileName_ + "size.");
        }

        return file_stat.st_size;
    }

    Offset Write(const void* data, size_t count, Offset offset = 0) {
        Offset new_offset = lseek(fd_, offset, SEEK_SET);
        if (new_offset == offset - 1) {
            throw error::BadArgument("Wrong arguments");
        }
        if (write(fd_, data, count) == -1) {
            throw error::IoError("Failed to write to file " + fileName_);
        }
        return new_offset;
    }

    void Read(void* data, size_t count, Offset offset = 0) const {
        Offset new_offset = lseek(fd_, offset, SEEK_SET);
        if (new_offset == offset - 1) {
            throw error::BadArgument("Wrong arguments");
        }
        auto result = read(fd_, data, count);
        if (result == -1) {
            throw error::IoError("Failed to read from file " + fileName_);
        }
        if (result == 0) {
            throw error::BadArgument("Reached EOF");
        }
    }
};

}  // namespace io