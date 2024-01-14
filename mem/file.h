#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <type_traits>
#include <vector>

#include "../util/error.h"

namespace mem {

using FileDescriptor = int32_t;
using Offset = off_t;

class File {
    FileDescriptor fd_;
    std::string fileName_;

    Offset Seek(Offset offset) const {
        Offset new_offset = lseek(fd_, offset, SEEK_SET);
        if (new_offset == offset - 1) {
            throw error::BadArgument("Wrong arguments");
        }
        return new_offset;
    }

public:
    explicit File(std::string&& fileName) : fileName_(std::forward<std::string>(fileName)) {
        fd_ = open(fileName_.c_str(), O_RDWR | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
        if (fd_ == -1) {
            throw error::IoError("File could not be opened");
        }
    }
    explicit File(const std::string& fileName) : File(std::string(fileName)) {
    }
    ~File() {
        close(fd_);
    }

    [[nodiscard]] size_t GetSize() const {

        // TODO: Adapt for Windows
        struct stat64 file_stat;
        if (fstat64(fd_, &file_stat) != 0) {
            throw error::BadArgument("Failed to get file " + fileName_ + "size.");
        }

        return file_stat.st_size;
    }

    template <typename T>
    Offset Write(const T& data, Offset offset = 0, size_t struct_offset = 0,
                 size_t count = sizeof(T)) {
        count = std::min(count, sizeof(T) - struct_offset);
        auto new_offset = Seek(offset);
        if (write(fd_, reinterpret_cast<const char*>(&data) + struct_offset, count) == -1) {
            throw error::IoError("Failed to write to file " + fileName_);
        }
        return new_offset;
    }

    Offset Write(std::string str, Offset offset = 0, size_t from = 0,
                 size_t count = std::string::npos) {
        count = std::min(count, str.size() - from);
        auto new_offset = Seek(offset);
        if (write(fd_, str.data() + from, count) == -1) {
            throw error::IoError("Failed to write to file " + fileName_);
        }
        return new_offset;
    }

    template <typename T>
    Offset Write(const std::vector<T>& vec, Offset offset = 0, size_t from = 0,
                 size_t count = SIZE_MAX) {
        count = std::min(count, vec.size() - from);
        auto new_offset = Seek(offset);
        if (write(fd_, vec.data(), count * sizeof(T)) == -1) {
            throw error::IoError("Failed to write to file " + fileName_);
        }
        return new_offset;
    }

    template <typename T>
    requires std::is_default_constructible_v<T>
    [[nodiscard]] T Read(Offset offset = 0, size_t struct_offset = 0,
                         size_t count = sizeof(T)) const {
        count = std::min(count, sizeof(T) - struct_offset);
        auto new_offset = Seek(offset);
        T data{};
        auto result = read(fd_, reinterpret_cast<char*>(&data) + struct_offset, count);
        if (result == -1) {
            throw error::IoError("Failed to read from file " + fileName_);
        }
        if (result == 0) {
            throw error::BadArgument("Reached EOF");
        }
        return data;
    }

    [[nodiscard]] std::string ReadString(Offset offset = 0, size_t count = 0) const {
        auto new_offset = Seek(offset);
        std::string str;
        str.resize(count);
        auto result = read(fd_, str.data(), count);
        if (result == -1) {
            throw error::IoError("Failed to read from file " + fileName_);
        }
        if (result == 0) {
            throw error::BadArgument("Reached EOF");
        }
        return str;
    }

    template <typename T>
    requires std::is_default_constructible_v<T>
    [[nodiscard]] std::vector<T> ReadVector(Offset offset = 0, size_t count = 0) const {
        auto new_offset = Seek(offset);
        std::vector<T> vec(count);
        auto result = read(fd_, vec.data(), count * sizeof(T));
        if (result == -1) {
            throw error::IoError("Failed to read from file " + fileName_);
        }
        if (result == 0) {
            throw error::BadArgument("Reached EOF");
        }
        return vec;
    }
};

}  // namespace mem