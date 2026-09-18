// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Entry point for the console application
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_PARSER_H
#define KERNELMGR_PARSER_H


#include <getopt.h>
#include <iostream>
#include <string>

struct Arguments {
    int flags;
    std::string path;
    std::string patch;
    std::string kernel;
    std::string oldkernel;
    std::string suffix;
    std::string oldsuffix;
    std::string config;
    std::string clean;
    std::string completion;    // The shell to print completions for.
    std::string completeFor;   // The option whose values the shell wants.
};

static void print_short_usage() {
    std::cout <<
        BLUE "Usage:" RESET "\n"
        "  " GREEN "kmgr" RESET " --option[-shortoption] " CYAN "value " YELLOW "--flag" RESET "\n"
        "\n"
        "For more info try kmgr --help\n";
}

static void print_usage() {
    std::cout <<
        "Downloads specified kernel, builds and installs it. If a previous kernel is provided,\n"
        "cleans the boot directory, /var/lib and dkms modules as well as anything left in the build\n"
        "directory.\n"
        "\n"
        BLUE "Usage" RESET ":\n"
        "  " GREEN "kmgr" RESET " -option " CYAN "value " YELLOW "--flag" RESET "\n"
        "\n"
        "  The default base path is \"" RED "$HOME" RESET "/.local/share/kernelmanager\". The default archive format is \"tar.xz\".\n"
        "These can be changed from the frontend, or in $XDG_CONFIG_HOME/kernelmanager/config.json.\n"
        "\n"
        "  The expected file name for the configuration file is either as majorversion.config (ex. for 6.1.4: 6.1.config)\n"
        "or, for custom configs, suffixedversion.config (ex. for 6.1.4-suffix: 6.1-suffix.config).\n"
        "If no configuration file is present, the configuration will have to manually be done during building.\n"
        "\n"
        "  The expected file name for patch scripts is *-patch.txt. If no patch file is present patching will be skipped\n"
        "(same as --nopatch). The patch file itself follows the format:\n"
        "patchdir (absolute path, no ~/)\n"
        "patch name (just the name, without a number or extension)\n"
        "  " BLUE "Ex:" RESET " \n"
        "  If the directory is \"" RED "$HOME" RESET "/.local/share/kernelmanager/patches\", the required patches are\n"
        "0001-first.patch and folder/0002-second.patch,\n"
        "and you want to skip the numbering:\n"
        "    home/user/.local/share/kernelmanager/patches\n"
        "    *first.patch\n"
        "    folder/*second.patch\n"
        "  You can also provide the full names:\n"
        "    home/user/.local/share/kernelmanager/patches\n"
        "    0001-first.patch\n"
        "    folder/0002-second.patch\n"
        "\n"
        BLUE "Examples" RESET ":\n"
        "  To download and install kernel 6.1.4 as 6.1.4-custom-suffix, remove 6.1.3-custom-suffix,\n"
        "  build with LLVM and sign the image with sbctl:\n"
        "    " GREEN "kmgr" RESET " -K " CYAN "6.1.3" RESET " -k " CYAN "6.1.4" RESET " -s " CYAN "custom-suffix " YELLOW "--clang --sign" RESET "\n"
        "\n"
        "  To download and install version 6.1.4 without removing anything:\n"
        "    " GREEN "kmgr" RESET " -k " CYAN "6.1.4 " YELLOW "--install" RESET "\n"
        "\n"
        "  To remove all traces of kernel 6.1.4 including:\n"
        "    " GREEN "kmgr" RESET " -k " CYAN "6.1.4 " YELLOW "--clean" RESET "\n"
        "\n"
        "  To download and patch kernel without building or installing:\n"
        "    " GREEN "kmgr" RESET " -k " CYAN "6.1.4 " YELLOW "--download" RESET "\n"
        "\n"
        "  To download kernel without building, installing or patching:\n"
        "    " GREEN "kmgr" RESET " -k " CYAN "6.1.4 " YELLOW "--download --nopatch" RESET "\n"
        "\n"
        "  To build and install kernel with clang after one has been extracted with --download:\n"
        "    " GREEN "kmgr" RESET " -k " CYAN "6.1.4 " YELLOW "--resume --install --clang" RESET " (Note: If a kernel has been installed, since the\n"
        "                                              build directory is not cleaned, it can be reinstalled\n"
        "                                              (rerun make install, create/run mkinitcpio profile,\n"
        "                                              run grub-mkconfig) by running the same command (with\n"
        "                                      1        or without --clang).\n"
        "                                              If DKMS modules have been installed, they will not be\n"
        "                                              reinstalled.)\n"
        "\n"
        "  To attempt an automatic update:\n"
        "    " GREEN "kmgr " YELLOW "--autoupdate" RESET "\n"
        "\n"
        BLUE "Options" RESET ":\n"
        "    -p |--path /path/to/build   Specify a path to the build directory.\n"
        "    -k |--kernel 6.1.4          Version number of the kernel to build.\n"
        "    -K |--oldkernel 6.1.4       Version number of old kernel to replace.\n"
        "    -s |--suffix tkg            Kernel suffix to be appended at the end of the version number.\n"
        "    -S |--oldsuffix tkg         Kernel suffix of the kernel to be replaced.\n"
        "    -c |--config backup.config  Specify name of a custom config file located in the build directory.\n"
        "    -P |--patch patchall.sh     Specify name of custom patch script located in the build directory.\n"
        "\n"
        BLUE "Flags" RESET ":\n"
        "    --pull                      Update any git repository present in the base path.\n"
        "\n"
        "    --install                   This will install relevant files to the modules and boot\n"
        "                                folders, will refresh grub, run relevant dkms commands and\n"
        "                                create a mkinitpcio profile. Enabled by default if old kernel\n"
        "                                version is supplied.\n"
        "\n"
        "    --clean=all/archive/logs    Clean all traces of given kernel. This will clean the\n"
        "                                boot and modules folders and will remove the build folder.\n"
        "                                Accepts a comma separated list of the following, so\n"
        "                                --clean=logs,archive asks for both:\n"
        "                                  all     - Deletes everything: dkms modules, kernel modules,\n"
        "                                        images, signatures, boot entries, build folder but\n"
        "                                        keeps the archive. Does not work without a kernel\n"
        "                                        version.\n"
        "                                  archive - Deletes the associated kernel's archive. If\n"
        "                                        no kernel version is specified, deletes all kernel\n"
        "                                        archives.\n"
        "                                  logs    - Deletes the logs written for the given kernel,\n"
        "                                        under every suffix it was run with. If no kernel\n"
        "                                        version is specified, deletes every log there is,\n"
        "                                        maintenance runs included.\n"
        "\n"
        "\n"
        "    --download                  Download the kernel and prepare the directory. This will\n"
        "                                also apply patches assuming the nopatch flag is omitted.\n"
        "                                If a kernel folder with the same version already exists\n"
        "                                it will be replaced.\n"
        "\n"
        "    --resume                    If a corresponding build directory exists, skips to build.\n"
        "                                Does not assume --install, if needed it should be supplied\n"
        "                                separately.\n"
        "\n"
        "    --nopatch                   Skip the patching process defined in the patch script.\n"
        "\n"
        "    --revert                    Take the patches back out of a build directory that was\n"
        "                                patched, newest first, and drop the mark that said so.\n"
        "                                What goes out is what the mark says went in, so a patch\n"
        "                                file that has changed since does not confuse it. A request\n"
        "                                on its own: nothing is downloaded, built or installed\n"
        "                                alongside it.\n"
        "\n"
        "    --clang                     Build kernel with LLVM, whatever the configured toolchain\n"
        "                                is.\n"
        "\n"
        "    --gcc                       Build kernel with GCC, whatever the configured toolchain\n"
        "                                is.\n"
        "\n"
        "    --custom                    Build kernel with the flags named in config.json under\n"
        "                                customFlags, handed to make as they are written and in\n"
        "                                place of anything the other two would have added.\n"
        "                                Without any of these three the toolchain named in\n"
        "                                config.json is used, which is gcc unless it was changed.\n"
        "\n"
        "    --sign                      Assuming sbctl is installed and configured and secure boot\n"
        "                                has already been set up, signs the new image.\n"
        "\n"
        "    --autoupdate                Tries to query the latest available kernel, compares it to\n"
        "                                the currently used version, gets the current suffix, gets\n"
        "                                the compiler used and uses the information to configure,\n"
        "                                build and install the newer kernel. Overwrites every other\n"
        "                                flag/option.\n"
        "\n"
        "    --force                     Will force execution of subsequent steps after error.\n"
        "                                Not recommended since most steps rely on the successful\n"
        "                                completion of previous ones.\n"
        "\n"
        BLUE "Other" RESET ":\n"
        "    --help                      Show this text and exit.\n"
        "    -L |--list                  Show what is on disk: the archives that have been\n"
        "                                downloaded, the directories they were unpacked into and\n"
        "                                how far each has been taken, and the kernels installed on\n"
        "                                this machine.\n"
        "\n"
        "    --completion bash/zsh/fish  Print the completion definitions for a shell. The flags\n"
        "                                and, where there is something to offer, the values behind\n"
        "                                them are completed too. To install them:\n"
        "                                  bash: kmgr --completion bash >\n"
        "                                          ~/.local/share/bash-completion/completions/kmgr\n"
        "                                  fish: kmgr --completion fish >\n"
        "                                          ~/.config/fish/completions/kmgr.fish\n"
        "                                  zsh:  kmgr --completion zsh > \"${fpath[1]}/_kmgr\"\n"
        "\n"
        "    --version                   Show script version.\n";
}

namespace Parser {
    enum Flags {
        NONE = 0,
        VERSION = 1 << 0,
        PULL = 1 << 1,
        INSTALL = 1 << 2,
        CLEAN = 1 << 3,
        DELETE = 1 << 4,
        DOWNLOAD = 1 << 5,
        RESUME = 1 << 6,
        NOPATCH = 1 << 7,
        CLANG = 1 << 8,
        SIGN = 1 << 9,
        AUTOUPDATE = 1 << 10,
        FORCE = 1 << 11,
        HELP = 1 << 12,
        LIST = 1 << 13,
        REVERT = 1 << 14,
        GCC = 1 << 15,
        CUSTOM = 1 << 16,
    };

    // Long only, so numbered past any letter.
    enum LongOnly {
        COMPLETION = 256,
        COMPLETE_FOR = 257
    };

    inline bool parse(const int argc, char *argv[], Arguments *arguments)
    {
        if (argc < 2) {
            print_short_usage();

            return false;
        }

        arguments->flags = NONE;

        const option options[] = {
            { .name = "path"      , .has_arg = required_argument, .flag = nullptr, .val = 'p' },
            { .name = "kernel"    , .has_arg = required_argument, .flag = nullptr, .val = 'k' },
            { .name = "oldkernel" , .has_arg = required_argument, .flag = nullptr, .val = 'K' },
            { .name = "suffix"    , .has_arg = required_argument, .flag = nullptr, .val = 's' },
            { .name = "oldsuffix" , .has_arg = required_argument, .flag = nullptr, .val = 'S' },
            { .name = "config"    , .has_arg = required_argument, .flag = nullptr, .val = 'c' },
            { .name = "patch"     , .has_arg = required_argument, .flag = nullptr, .val = 'P' },
            { .name = "version"   , .has_arg = no_argument, .flag = nullptr, .val = 'v'},
            { .name = "list"      , .has_arg = no_argument, .flag = nullptr, .val = 'L'},
            { .name = "completion", .has_arg = required_argument, .flag = nullptr, .val = COMPLETION },
            { .name = "complete-for", .has_arg = required_argument, .flag = nullptr, .val = COMPLETE_FOR },
            { .name = "pull"      , .has_arg = no_argument, .flag = nullptr, .val = 'u'},
            { .name = "install"   , .has_arg = no_argument, .flag = nullptr, .val = 'i'},
            { .name = "clean"     , .has_arg = optional_argument, .flag = nullptr, .val = 'l'},
            { .name = "delete"    , .has_arg = no_argument, .flag = nullptr, .val = 'd'},
            { .name = "download"  , .has_arg = no_argument, .flag = nullptr, .val = 'w'},
            { .name = "resume"    , .has_arg = no_argument, .flag = nullptr, .val = 'r'},
            { .name = "nopatch"   , .has_arg = no_argument, .flag = nullptr, .val = 'x'},
            // Long only, since -r is already resume.
            { .name = "revert"    , .has_arg = no_argument, .flag = nullptr, .val = 'R'},
            { .name = "clang"     , .has_arg = no_argument, .flag = nullptr, .val = 'g'},
            // Long only, since -g is already clang.
            { .name = "gcc"       , .has_arg = no_argument, .flag = nullptr, .val = 'G'},
            { .name = "custom"    , .has_arg = no_argument, .flag = nullptr, .val = 'C'},
            { .name = "sign"      , .has_arg = no_argument, .flag = nullptr, .val = 'n'},
            { .name = "autoupdate", .has_arg = no_argument, .flag = nullptr, .val = 'a'},
            { .name = "force"     , .has_arg = no_argument, .flag = nullptr, .val = 'f'},
            { .name = "help"      , .has_arg = no_argument, .flag = nullptr, .val = 'h'},
            { .name = nullptr     , .has_arg = no_argument, .flag = nullptr, .val = 0   }
        };

        int opt;

        while ((opt = getopt_long(argc, argv, "p:P:k:K:s:S:c:vLuil::dwrxgnafh", options, nullptr)) != -1) {
            switch (opt) {
                case 'p':
                    arguments->path = optarg;
                    break;
                case 'P':
                    arguments->patch = optarg;
                    break;
                case 'k':
                    arguments->kernel = optarg;
                    break;
                case 'K':
                    arguments->oldkernel = optarg;
                    break;
                case 's':
                    arguments->suffix = optarg;
                    break;
                case 'S':
                    arguments->oldsuffix = optarg;
                    break;
                case 'c':
                    arguments->config = optarg;
                    break;
                case 'v':
                    arguments->flags |= VERSION;
                    break;
                case 'L':
                    arguments->flags |= LIST;
                    break;
                case COMPLETION:
                    arguments->completion = optarg;
                    break;
                case COMPLETE_FOR:
                    arguments->completeFor = optarg;
                    break;
                case 'u':
                    arguments->flags |= PULL;
                    break;
                case 'i':
                    arguments->flags |= INSTALL;
                    break;
                case 'l':
                    arguments->flags |= CLEAN;
                    arguments->clean = optarg ? optarg : "all";
                    break;
                case 'd':
                    arguments->flags |= DELETE;
                    break;
                case 'w':
                    arguments->flags |= DOWNLOAD;
                    break;
                case 'r':
                    arguments->flags |= RESUME;
                    break;
                case 'x':
                    arguments->flags |= NOPATCH;
                    break;
                case 'R':
                    arguments->flags |= REVERT;
                    break;
                case 'g':
                    arguments->flags |= CLANG;
                    break;
                case 'G':
                    arguments->flags |= GCC;
                    break;
                case 'C':
                    arguments->flags |= CUSTOM;
                    break;
                case 'n':
                    arguments->flags |= SIGN;
                    break;
                case 'a':
                    arguments->flags |= AUTOUPDATE;
                    break;
                case 'f':
                    arguments->flags |= FORCE;
                    break;
                case 'h':
                    print_usage();
                    return false;
                case '?':
                default:
                    print_short_usage();
                    return false;
            }
        }

        return true;
    }
}


#endif //KERNELMGR_PARSER_H
