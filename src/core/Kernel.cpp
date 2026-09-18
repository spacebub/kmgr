// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <sys/utsname.h>
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <set>
#include "yyjson.h"

#include "Kernel.h"
#include "Download.h"
#include "Configuration.h"

constexpr auto RELEASES_ENDPOINT = "https://kernel.org/releases.json";

Kernel::Kernel(const std::string &suffixedVersion) {
    _version = get_version(suffixedVersion);
}

Kernel Kernel::get_current() {
    utsname osInfo{};
    uname(&osInfo);

    return Kernel(osInfo.release);
}

Kernel Kernel::get_latest(const std::function<void (const Version &)> &callback) {
    const std::string releasesJson = Download(RELEASES_ENDPOINT).get_page();

    yyjson_doc *doc = yyjson_read(releasesJson.c_str(), releasesJson.size(), YYJSON_READ_NOFLAG);
    yyjson_val *root = yyjson_doc_get_root(doc);

    yyjson_val *val = yyjson_obj_get(root, "latest_stable");

    Kernel kernel(yyjson_get_str(yyjson_obj_get(val, "version")));

    yyjson_doc_free(doc);

    if (callback) {
        callback(kernel.get_version());
    }

    return kernel;
}

std::vector<Kernel> Kernel::list_extracted() {
    std::vector<Kernel> kernels;

    // A missing base directory is an empty list, not an error.
    std::error_code error;

    for (std::filesystem::directory_iterator it(Configuration::get()->baseDirectory, error), end;
            !error && it != end; it.increment(error)) {
        const std::string filename = it->path().filename();

        if (!it->is_directory() || !filename.starts_with("linux-")) {
            continue;
        }

        try {
            kernels.emplace_back(filename.substr(6));
        } catch (const std::exception &) {
            // Not a version, not a kernel.
        }
    }

    std::ranges::sort(kernels, [](const Kernel &a, const Kernel &b) {
        const std::strong_ordering order = a.get_version() <=> b.get_version();

        return order == std::strong_ordering::equal
            ? a.get_version().suffix < b.get_version().suffix
            : order == std::strong_ordering::greater;
    });

    return kernels;
}

// Either the modules or the boot image counts as installed. The sources are often gone.
std::vector<Kernel> Kernel::list_installed() {
    std::set<std::string> names;

    for (const std::string &directory : { std::string("/usr/lib/modules"), std::string("/lib/modules") }) {
        if (!std::filesystem::is_directory(directory)) {
            continue;
        }

        for (const auto &entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_directory()) {
                names.insert(entry.path().filename());
            }
        }
    }

    if (const std::string boot = Configuration::get()->bootDirectory; std::filesystem::is_directory(boot)) {
        for (const auto &entry : std::filesystem::directory_iterator(boot)) {
            const std::string filename = entry.path().filename();

            if (filename.starts_with("vmlinuz-")) {
                names.insert(filename.substr(8));
            }
        }
    }

    std::vector<Kernel> kernels;

    for (const std::string &name : names) {
        try {
            Kernel kernel(name);

            if (kernel.get_status() != NotInstalled) {
                kernels.push_back(kernel);
            }
        } catch (const std::exception &) {
            // Not a version, not a kernel.
        }
    }

    std::ranges::sort(kernels, [](const Kernel &a, const Kernel &b) {
        const std::strong_ordering order = a.get_version() <=> b.get_version();

        return order == std::strong_ordering::equal
            ? a.get_version().suffix < b.get_version().suffix
            : order == std::strong_ordering::greater;
    });

    return kernels;
}

Kernel::Version Kernel::get_version() const {
    return _version;
}

Kernel::Version Kernel::get_version(const std::string &suffixedVersion) {
    const ulong separatorIndex = suffixedVersion.find('-');
    Version version{};

    if (separatorIndex != std::string::npos) {
        version.suffix = suffixedVersion.substr(separatorIndex + 1);
    }

    ushort versionNumbers[3]{};
    size_t last = 0, counter = 0;
    for (size_t current = 0; current < suffixedVersion.length(); ++current) {
        if (counter == 2) {
            break;
        }

        if (suffixedVersion[current] == '.') {
            std::string sss = suffixedVersion.substr(last + (last != 0), current - last - (last != 0));
            versionNumbers[counter] = std::stoi(sss);
            last = current;
            counter++;
        }
    }

    versionNumbers[counter] = std::stoi(separatorIndex == std::string::npos
            ? suffixedVersion.substr(last + 1)
            : suffixedVersion.substr(last + 1, suffixedVersion.length() - version.suffix.length() - last - 2));

    version.major = versionNumbers[0];
    version.minor = versionNumbers[1];
    version.revision = versionNumbers[2];

    return version;
}

Kernel::Version Kernel::get_version(const std::string &version, const std::string &suffix) noexcept {
    Version resolved{};

    try {
        resolved = get_version(version);
    } catch (const std::exception &) {
        return {};
    }

    resolved.suffix = suffix;

    return resolved;
}

Kernel::Status Kernel::get_status() const {
    Status status = NotInstalled;
    const std::string version = get_version().get_string();

    if (std::filesystem::exists("/usr/lib/modules/" + version)
            || std::filesystem::exists("/lib/modules/" + version)) {
        status = status | ModulesInstalled;
    }
    if (!get_image().empty()) {
        status = status | ImageInstalled;
    }
    if (!get_initramfs().empty()) {
        status = status | InitramsInstalled;
    }

    return status;
}

static std::string package_of(const std::string &version) {
    for (const std::filesystem::path &directory : { std::filesystem::path("/usr/lib/modules"), std::filesystem::path("/lib/modules") }) {
        const std::filesystem::path marker = directory / version / "pkgbase";

        if (!std::filesystem::exists(marker)) {
            continue;
        }

        std::ifstream file(marker);
        std::string name;

        if (std::getline(file, name); !name.empty()) {
            return name;
        }
    }

    return {};
}

std::string Kernel::get_image() const {
    const std::string boot = Configuration::get()->bootDirectory;
    const std::string version = get_version().get_string();

    if (const std::string own = boot + "/vmlinuz-" + version; std::filesystem::exists(own)) {
        return own;
    }

    if (const std::string package = package_of(version); !package.empty()) {
        if (const std::string packaged = boot + "/vmlinuz-" + package; std::filesystem::exists(packaged)) {
            return packaged;
        }
    }

    return {};
}

std::string Kernel::get_initramfs() const {
    const std::string boot = Configuration::get()->bootDirectory;
    const std::string version = get_version().get_string();

    if (const std::string own = boot + "/initramfs-" + version + ".img"; std::filesystem::exists(own)) {
        return own;
    }

    if (const std::string package = package_of(version); !package.empty()) {
        if (const std::string packaged = boot + "/initramfs-" + package + ".img";
                std::filesystem::exists(packaged)) {
            return packaged;
        }
    }

    return {};
}

// Reads the PE certificate table, since sbctl only answers as root.
bool Kernel::is_signed() const {
    const std::string image = get_image();

    if (image.empty()) {
        return false;
    }

    std::ifstream file(image, std::ios::binary);
    char header[1024]{};

    if (!file.read(header, sizeof(header))) {
        return false;
    }

    const auto read32 = [&header](const size_t offset) {
        return static_cast<uint32_t>(static_cast<unsigned char>(header[offset]))
            | static_cast<uint32_t>(static_cast<unsigned char>(header[offset + 1])) << 8
            | static_cast<uint32_t>(static_cast<unsigned char>(header[offset + 2])) << 16
            | static_cast<uint32_t>(static_cast<unsigned char>(header[offset + 3])) << 24;
    };

    if (header[0] != 'M' || header[1] != 'Z') {
        return false;
    }

    const uint32_t pe = read32(0x3c);

    if (pe + 24 + 128 + 8 > sizeof(header) || std::string(&header[pe], 4) != std::string("PE\0\0", 4)) {
        return false;
    }

    const uint16_t magic = static_cast<uint16_t>(static_cast<unsigned char>(header[pe + 24]))
        | static_cast<uint16_t>(static_cast<unsigned char>(header[pe + 25])) << 8;

    // The certificate table is the fifth data directory of the optional header.
    const size_t directory = pe + 24 + (magic == 0x20b ? 112 : 96) + 4 * 8;

    return directory + 8 <= sizeof(header) && read32(directory + 4) > 0;
}

std::string Kernel::get_source_directory() const {
    return get_source_directory(_version);
}

std::string Kernel::get_source_directory(const Version &version) {
    return Configuration::get()->baseDirectory + "/linux-" + version.get_string();
}

bool Kernel::is_configured() const { return is_configured(_version); }
bool Kernel::is_built() const { return is_built(_version); }
bool Kernel::is_patched() const { return is_patched(_version); }

bool Kernel::is_configured(const Version &version) {
    const std::string source = get_source_directory(version);

    return std::filesystem::is_directory(source)
        && std::filesystem::exists(source + "/.config");
}

bool Kernel::is_built(const Version &version) {
    const std::string source = get_source_directory(version);

    return std::filesystem::is_directory(source)
        && (std::filesystem::exists(source + "/arch/x86/boot/bzImage")
            || std::filesystem::exists(source + "/vmlinux"));
}

namespace {
    Toolchain toolchain_of(const std::string &line) {
        if (line.find("clang") != std::string::npos || line.find("Clang") != std::string::npos) {
            return Toolchain::Llvm;
        }

        return line.find("gcc") != std::string::npos || line.find("GCC") != std::string::npos
            ? Toolchain::Gcc
            : Toolchain::Unknown;
    }
}

Toolchain Kernel::built_with(const Version &version) {
    const std::string source = get_source_directory(version);

    if (!std::filesystem::is_directory(source)) {
        return Toolchain::Unknown;
    }

    // compile.h names the compiler that actually ran, so it wins over .config.
    if (std::ifstream compiled(source + "/include/generated/compile.h"); compiled.is_open()) {
        std::string line;

        while (std::getline(compiled, line)) {
            if (line.find("LINUX_COMPILER") == std::string::npos) {
                continue;
            }

            if (const Toolchain found = toolchain_of(line); found != Toolchain::Unknown) {
                return found;
            }
        }
    }

    // Only the first lines are read: the compiler is recorded there and the file is large.
    std::ifstream config(source + "/.config");
    std::string line;

    for (int read = 0; read < 64 && std::getline(config, line); ++read) {
        if (line == "CONFIG_CC_IS_CLANG=y") {
            return Toolchain::Llvm;
        }

        if (line == "CONFIG_CC_IS_GCC=y") {
            return Toolchain::Gcc;
        }
    }

    return Toolchain::Unknown;
}

namespace {
    std::string patch_marker(const Kernel::Version &version) {
        return Kernel::get_source_directory(version) + "/.kernelmgr-patched";
    }
}

bool Kernel::is_patched(const Version &version) {
    return std::filesystem::exists(patch_marker(version));
}

std::string Kernel::patched_with(const Version &version) {
    std::ifstream file(patch_marker(version));
    std::string definition;

    std::getline(file, definition);

    return definition;
}

bool Kernel::mark_patched(const Version &version, const std::string &definition) {
    std::ofstream file(patch_marker(version), std::ios::trunc);

    if (!file.is_open()) {
        return false;
    }

    file << definition << "\n";

    return file.good();
}

bool Kernel::clear_patched(const Version &version) {
    std::error_code error;

    std::filesystem::remove(patch_marker(version), error);

    return !error;
}

std::string Kernel::find_config(const Version &version) {
    const Settings *config = Configuration::get();
    const std::string family = std::format("{}.{}", version.major, version.minor);
    std::vector<std::string> candidates;

    if (!version.suffix.empty()) {
        candidates.push_back(std::format("{}/{}-{}.config", config->baseDirectory, family, version.suffix));
    }

    candidates.push_back(std::format("{}/{}.config", config->baseDirectory, family));

    utsname osInfo{};
    uname(&osInfo);
    candidates.push_back(std::format("{}/config-{}", config->bootDirectory, osInfo.release));

    for (const std::string &candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    if (std::filesystem::exists("/proc/config.gz")) {
        return "/proc/config.gz";
    }

    return {};
}

std::string Kernel::find_patch(const Version &version) {
    const std::string base = Configuration::get()->baseDirectory;
    const std::string family = std::format("{}.{}", version.major, version.minor);
    std::vector<std::string> candidates;

    if (!version.suffix.empty()) {
        candidates.push_back(std::format("{}/{}-{}-patch.txt", base, family, version.suffix));
    }

    candidates.push_back(std::format("{}/{}-patch.txt", base, family));

    for (const std::string &candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

std::string Kernel::Version::get_string(const int options) const {
    std::string string = std::format("{}.{}", major, minor);

    if (revision > 0 || options & INCLUDE_ZERO_REVISION) {
        string += std::format(".{}", revision);
    }

    if (options & INCLUDE_SUFFIX && !suffix.empty()) {
        string += "-" + suffix;
    }

    return string;
}
