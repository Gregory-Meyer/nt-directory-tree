#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <variant>
#include <utility>
#include <unordered_map>
#include <cstdint>
#include <queue>
#include <optional>

#include <Windows.h>

class DirectoryEntry;

using zstring = char*;
using czstring = const char*;

using wzstring = wchar_t*;
using cwzstring = const wchar_t;

std::optional<std::string> error_string(std::uint32_t code) {
    if (code == 0) {
        return { std::string{ } };
    }

    std::optional<std::string> maybe_description;
    zstring description = nullptr;
    const std::uint32_t size = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<zstring>(&description),
        0,
        nullptr
    );

    if (!description) {
        return maybe_description;
    }

    maybe_description.emplace(description, size);

    return maybe_description;
}

class File {
public:
    File(cwzstring name) : name_{ name_ }, size_{ get_size() } { }

    std::wstring_view name() const noexcept {
        return { name_ };
    }

    std::int64_t size() const noexcept {
        return size_;
    }

private:
    std::int64_t get_size() {
        name_ = L"\\?" + name_;

        const HANDLE handle = CreateFileW(
            name_.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (handle == INVALID_HANDLE_VALUE) {
            const auto error = GetLastError();
            const auto description = error_string(error);

            throw std::runtime_error{ description };
        }

        WIN32_FILE_ATTRIBUTE_DATA data;

        if (!GetFileAttributesExW(name_.c_str(), GetFileExInfoStandard,
                                  &data)) {
            throw std::runtime_error("error getting file attributes");
        }

        std::uint64_t size = 0;

        size |= data.nFileSizeHigh;
        size <<= 32;
        size |= data.nFileSizeLow;

        return static_cast<std::int64_t>(size);
    }

    std::wstring name_;
    std::int64_t size_;
};

class Folder {
    using EntryMapT = std::unordered_map<
        std::wstring_view, std::reference_wrapper<DirectoryEntry>
    >;

    using EntryVecT = std::vector<DirectoryEntry>;

    std::wstring name;
    EntryVecT children;
    EntryMapT children_names;
    std::uint64_t size;
};

class DirectoryEntry {
public:
    DirectoryEntry& parent() noexcept {
        return *parent_ptr_;
    }

    const DirectoryEntry& parent() const noexcept{
        return *parent_ptr_;
    }
    
    std::wstring_view name() const noexcept {
        return std::visit(
            [](auto &&entry) {
                return std::forward<decltype(entry)>(entry).name();
            },
            data_
        );
    }

    std::uint64_t size() const noexcept {
        return std::visit(
            [](auto &&entry) {
                return std::forward<decltype(entry)>(entry).size;
            },
            data_
        );
    }
    
private:
    std::variant<File, Folder> data_;
    DirectoryEntry *parent_ptr_;
};

int main(int argc, const char *const argv[]) {
    const std::vector<std::string_view> args{ argv, argv + argc };
}