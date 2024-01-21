#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "error.hpp"
#include "logger.hpp"

namespace mem {

using FileDescriptor = int32_t;
using Offset = loff_t;
using StructOffset = size_t;

class File {
    FileDescriptor fd_;
    std::string fileName_;
    std::shared_ptr<util::Logger> logger_;

    Offset Seek(Offset offset) const {
        Offset new_offset = lseek(fd_, offset, SEEK_SET);
        if (new_offset == offset - 1) {
            throw error::BadArgument("Wrong arguments");
        }
        return new_offset;
    }

public:
    explicit File(std::string&& fileName,
                  std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : fileName_(std::move(fileName)), logger_(logger) {
        fd_ = open(fileName_.c_str(), O_RDWR | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
        if (fd_ == -1) {
            throw error::IoError("File could not be opened");
        }
    }
    explicit File(const std::string& fileName,
                  std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : File(std::string(fileName), logger) {
    }
    ~File() {
        close(fd_);
    }

    [[nodiscard]] std::string GetFilename() const {
        return fileName_;
    }

    [[nodiscard]] size_t GetSize() const {

        // TODO: Adapt for Windows
        struct stat64 file_stat;
        if (fstat64(fd_, &file_stat) != 0) {
            throw error::BadArgument("Failed to get file " + fileName_ + "size.");
        }

        return file_stat.st_size;
    }

    void Truncate(Offset size) {
        logger_->Verbose("Truncating, current size: " + std::to_string(GetSize()));
        if (ftruncate64(fd_, GetSize() - size) != 0) {
            throw error::IoError("Can't truncate file " + fileName_);
        }
    }

    void Extend(Offset size) {
        logger_->Verbose("Extending, current size: " + std::to_string(GetSize()));
        if (ftruncate64(fd_, GetSize() + size) != 0) {
            throw error::IoError("Can't extend file " + fileName_);
        }
    }
    void Clear() {
        logger_->Verbose("Clear");
        if (ftruncate64(fd_, 0) != 0) {
            throw error::IoError("Can't clear file " + fileName_);
        }
    }

    template <typename T>
    Offset Write(const T& data, Offset offset = 0, StructOffset struct_offset = 0,
                 StructOffset count = sizeof(T)) {
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
                 StructOffset count = SIZE_MAX) {
        count = std::min(count, vec.size() - from);
        auto new_offset = Seek(offset);
        if (write(fd_, vec.data(), count * sizeof(T)) == -1) {
            throw error::IoError("Failed to write to file " + fileName_);
        }
        return new_offset;
    }

    template <typename T>
    [[nodiscard]] T Read(
        Offset offset = 0, StructOffset struct_offset = 0,
        StructOffset count = sizeof(T)) const requires std::is_default_constructible_v<T> {
        logger_->Error("Cuurent size " + std::to_string(GetSize()));
        count = std::min(count, sizeof(T) - struct_offset);
        auto new_offset = Seek(offset);
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

    [[nodiscard]] std::string ReadString(Offset offset = 0, size_t count = 0) const {
        auto new_offset = Seek(offset);
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
    [[nodiscard]] std::vector<T> ReadVector(
        Offset offset = 0, size_t count = 0) const requires std::is_default_constructible_v<T> {
        auto new_offset = Seek(offset);
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