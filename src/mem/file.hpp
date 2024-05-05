#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "logger.hpp"

namespace mem {

using FileDescriptor = int32_t;
using Offset = off64_t;
using StructOffset = size_t;

class File {

    DECLARE_LOGGER;
    FileDescriptor fd_;
    std::string fileName_;

    auto Seek(Offset offset) const -> Offset {
        Offset new_offset = lseek64(fd_, offset, SEEK_SET);
        if (new_offset == offset - 1) {
            throw error::BadArgument("Wrong arguments");
        }
        return new_offset;
    }

public:
    using Ptr = util::Ptr<mem::File>;

    explicit File(std::string&& fileName, DEFAULT_LOGGER(logger))
        : LOGGER(logger), fileName_(std::move(fileName)) {
        fd_ = open(fileName_.c_str(), O_RDWR | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
        if (fd_ == -1) {
            throw error::IoError("File could not be opened");
        }
    }
    explicit File(const std::string& fileName, DEFAULT_LOGGER(logger))
        : File(std::string(fileName), logger) {
    }
    ~File() {
        close(fd_);
    }

    [[nodiscard]] auto GetFilename() const noexcept -> std::string_view {
        return fileName_;
    }

    [[nodiscard]] auto GetSize() const -> Offset {

        // TODO: Adapt for Windows
        struct stat64 file_stat;
        if (fstat64(fd_, &file_stat) != 0) {
            throw error::BadArgument("Failed to get file " + fileName_ + "size.");
        }

        return file_stat.st_size;
    }

    auto Truncate(Offset size) -> void {
        DEBUG("Truncating, current size: ", GetSize());
        if (ftruncate64(fd_, GetSize() - size) != 0) {
            throw error::IoError("Can't truncate file " + fileName_);
        }
        DEBUG("Truncating, current size: ", GetSize());
    }

    auto Extend(Offset size) -> void {
        DEBUG("Extending, current size: ", GetSize());
        if (ftruncate64(fd_, GetSize() + size) != 0) {
            throw error::IoError("Can't extend file " + fileName_);
        }
    }
    auto Clear() -> void {
        DEBUG("Clear");
        if (ftruncate64(fd_, 0) != 0) {
            throw error::IoError("Can't clear file " + fileName_);
        }
    }

    template <typename T>
    auto Write(const T& data, Offset offset = 0, StructOffset struct_offset = 0,
               StructOffset count = sizeof(T)) -> Offset {
        count = std::min(count, sizeof(T) - struct_offset);
        auto new_offset = Seek(offset);
        if (write(fd_, reinterpret_cast<const char*>(&data) + struct_offset, count) == -1) {
            throw error::IoError("Failed to write to file " + fileName_);
        }
        return new_offset;
    }

    auto Write(std::string str, Offset offset = 0, size_t from = 0,
               size_t count = std::string::npos) -> Offset {
        count = std::min(count, str.size() - from);
        auto new_offset = Seek(offset);
        if (write(fd_, str.data() + from, count) == -1) {
            throw error::IoError("Failed to write to file " + fileName_);
        }
        return new_offset;
    }

    template <typename T>
    auto Write(const std::vector<T>& vec, Offset offset = 0, size_t from = 0,
               StructOffset count = SIZE_MAX) -> Offset {
        count = std::min(count, vec.size() - from);
        auto new_offset = Seek(offset);
        if (write(fd_, vec.data(), count * sizeof(T)) == -1) {
            throw error::IoError("Failed to write to file " + fileName_);
        }
        return new_offset;
    }

    template <typename T>
    [[nodiscard]] auto Read(Offset offset = 0, StructOffset struct_offset = 0,
                            StructOffset count = sizeof(T)) const
        -> T requires std::is_default_constructible_v<T> {
        count = std::min(count, sizeof(T) - struct_offset);
        Seek(offset);
        T data{};
        auto result = read(fd_, reinterpret_cast<char*>(&data) + struct_offset, count);
        if (result == -1) {
            throw error::IoError("Failed to read from file " + fileName_);
        }
        if (result == 0) {
            throw error::IoError("Reached EOF");
        }
        return data;
    }

    [[nodiscard]] auto ReadString(Offset offset = 0, size_t count = 0) const -> std::string {
        Seek(offset);
        std::string str;
        str.resize(count);
        auto result = read(fd_, str.data(), count);
        if (result == -1) {
            throw error::IoError("Failed to read from file " + fileName_);
        }
        if (result == 0) {
            throw error::IoError("Reached EOF");
        }
        return str;
    }

    template <typename T>
    [[nodiscard]] auto ReadVector(Offset offset = 0, size_t count = 0) const -> std::vector<T>
    requires std::is_default_constructible_v<T> {
        Seek(offset);
        std::vector<T> vec(count);
        auto result = read(fd_, vec.data(), count * sizeof(T));
        if (result == -1) {
            throw error::IoError("Failed to read from file " + fileName_);
        }
        if (result == 0) {
            throw error::IoError("Reached EOF");
        }
        return vec;
    }
};

}  // namespace mem