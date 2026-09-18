// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_KERNEL_H
#define KERNELMGR_KERNEL_H


#include <compare>
#include <string>
#include <tuple>
#include <vector>
#include <functional>

#include "Toolchain.h"

class Kernel {
public:
    enum Status {
        NotInstalled = 0,
        ModulesInstalled = 1<<0,
        ImageInstalled = 1<<1,
        InitramsInstalled = 1<<2
    };

    friend Status operator|(const Status a, const Status b) {
        return static_cast<Status>(static_cast<int>(a) | static_cast<int>(b));
    }

    friend Status operator&(const Status a, const Status b) {
        return static_cast<Status>(static_cast<int>(a) & static_cast<int>(b));
    }

    struct Version {
        static constexpr int NO_OPTIONS = 0;
        static constexpr int INCLUDE_ZERO_REVISION = 1 << 0;
        static constexpr int INCLUDE_SUFFIX = 1 << 1;
        static constexpr int DEFAULT = INCLUDE_ZERO_REVISION | INCLUDE_SUFFIX;

        std::string suffix{};
        ushort major;
        ushort minor;
        ushort revision;

        [[nodiscard]] std::string get_string(int options = DEFAULT) const;

        friend std::strong_ordering operator<=>(const Version &a, const Version &b) {
            return std::tie(a.major, a.minor, a.revision) <=> std::tie(b.major, b.minor, b.revision);
        }

        friend bool operator==(const Version &a, const Version &b) {
            return (a <=> b) == std::strong_ordering::equal;
        }
    };

private:
    Version _version;

public:
    explicit Kernel(const std::string &suffixedVersion);

    static Kernel get_current();
    static Kernel get_latest(const std::function<void (const Version &)> &callback = nullptr);
    static std::vector<Kernel> list_extracted();
    static std::vector<Kernel> list_installed();

    [[nodiscard]] Version get_version() const;
    static Version get_version(const std::string &suffixedVersion);

    // Returns a zero version rather than throwing: callers ask about directories already on disk.
    static Version get_version(const std::string &version, const std::string &suffix) noexcept;

    [[nodiscard]] Status get_status() const;
    [[nodiscard]] std::string get_source_directory() const;

    [[nodiscard]] std::string get_image() const;
    [[nodiscard]] std::string get_initramfs() const;
    [[nodiscard]] bool is_signed() const;

    [[nodiscard]] bool is_configured() const;
    [[nodiscard]] bool is_built() const;
    [[nodiscard]] bool is_patched() const;

    static std::string get_source_directory(const Version &version);
    static bool is_configured(const Version &version);
    static bool is_built(const Version &version);
    static std::string find_config(const Version &version);
    static std::string find_patch(const Version &version);

    static Toolchain built_with(const Version &version);

    // Patching twice fails, so a patched tree carries a mark naming its definition.
    static bool is_patched(const Version &version);
    static std::string patched_with(const Version &version);
    static bool mark_patched(const Version &version, const std::string &definition);
    static bool clear_patched(const Version &version);
};


#endif //KERNELMGR_KERNEL_H
