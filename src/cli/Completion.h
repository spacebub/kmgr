// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_COMPLETION_H
#define KERNELMGR_COMPLETION_H


#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "core/Kernel.h"
#include "core/KernelArchive.h"

namespace Completion {

static void put(const std::vector<std::string> &values) {
    for (const std::string &value : values) {
        std::cout << value << "\n";
    }
}

inline std::vector<std::string> flags() {
    return {
        "-p", "--path",
        "-P", "--patch",
        "-k", "--kernel",
        "-K", "--oldkernel",
        "-s", "--suffix",
        "-S", "--oldsuffix",
        "-c", "--config",
        "-L", "--list",
        "-u", "--pull",
        "-i", "--install",
        "-l", "--clean",
        "-d", "--delete",
        "-w", "--download",
        "-r", "--resume",
        "-x", "--nopatch",
        "--revert",
        "-g", "--clang",
        "--gcc",
        "--custom",
        "-n", "--sign",
        "-a", "--autoupdate",
        "-f", "--force",
        "-v", "--version",
        "-h", "--help",
        "--completion"
    };
}

inline std::vector<std::string> kernels() {
    std::set<std::string> names;

    try {
        for (const KernelArchive &archive : KernelArchive::list_archives()) {
            names.insert(archive.get_version().get_string());
        }
    } catch (const std::exception &) {
        // Completion stays quiet on errors.
    }

    try {
        for (const Kernel &kernel : Kernel::list_extracted()) {
            names.insert(kernel.get_version().get_string(Kernel::Version::INCLUDE_ZERO_REVISION));
        }
    } catch (const std::exception &) {
    }

    return { names.begin(), names.end() };
}

inline std::vector<std::string> installed() {
    std::set<std::string> names;

    try {
        for (const Kernel &kernel : Kernel::list_installed()) {
            names.insert(kernel.get_version().get_string());
        }
    } catch (const std::exception &) {
    }

    return { names.begin(), names.end() };
}

inline std::vector<std::string> suffixes() {
    std::set<std::string> found;

    try {
        for (const Kernel &kernel : Kernel::list_extracted()) {
            if (const std::string suffix = kernel.get_version().suffix; !suffix.empty()) {
                found.insert(suffix);
            }
        }
    } catch (const std::exception &) {
    }

    return { found.begin(), found.end() };
}

inline int candidates(const std::string &what) {
    if (what == "flags") {
        put(flags());
    } else if (what == "kernels") {
        put(kernels());
    } else if (what == "installed") {
        put(installed());
    } else if (what == "suffixes") {
        put(suffixes());
    } else if (what == "clean") {
        put({ "all", "archive", "logs" });
    } else if (what == "shells") {
        put({ "bash", "zsh", "fish" });
    } else {
        std::cerr << "Nothing to complete for \"" << what << "\"\n";

        return 1;
    }

    return 0;
}

inline constexpr auto BASH = R"COMPLETION(# kmgr(1) completion for bash. Install with:
#   kmgr --completion bash > ~/.local/share/bash-completion/completions/kmgr

_kmgr() {
    local current previous
    current=${COMP_WORDS[COMP_CWORD]}
    previous=${COMP_WORDS[COMP_CWORD-1]}

    case "$previous" in
        -k|--kernel)
            mapfile -t COMPREPLY < <(compgen -W "$(kmgr --complete-for kernels)" -- "$current")
            return ;;
        -K|--oldkernel)
            mapfile -t COMPREPLY < <(compgen -W "$(kmgr --complete-for installed)" -- "$current")
            return ;;
        -s|--suffix|-S|--oldsuffix)
            mapfile -t COMPREPLY < <(compgen -W "$(kmgr --complete-for suffixes)" -- "$current")
            return ;;
        -p|--path)
            mapfile -t COMPREPLY < <(compgen -d -- "$current")
            return ;;
        -P|--patch|-c|--config)
            mapfile -t COMPREPLY < <(compgen -f -- "$current")
            return ;;
        --completion)
            mapfile -t COMPREPLY < <(compgen -W "$(kmgr --complete-for shells)" -- "$current")
            return ;;
    esac

    # --clean takes a list, so only the word after the last comma is being typed.
    if [[ "$current" == --clean=* ]]; then
        local list=${current#--clean=}
        local done=""
        local word=$list

        if [[ "$list" == *,* ]]; then
            done=${list%,*},
            word=${list##*,}
        fi

        local left=""

        for candidate in $(kmgr --complete-for clean); do
            # A word already in the list has nothing left to ask for.
            case ",$list," in
                *",$candidate,"*) ;;
                *) left="$left $candidate" ;;
            esac
        done

        mapfile -t COMPREPLY < <(compgen -P "--clean=$done" -W "$left" -- "$word")
        return
    fi

    mapfile -t COMPREPLY < <(compgen -W "$(kmgr --complete-for flags)" -- "$current")
}

complete -F _kmgr kmgr
)COMPLETION";

inline constexpr auto FISH = R"COMPLETION(# kmgr(1) completion for fish. Install with:
#   kmgr --completion fish > ~/.config/fish/completions/kmgr.fish

complete -c kmgr -f

complete -c kmgr -s p -l path      -r -F -d 'Base directory to work in'
complete -c kmgr -s P -l patch     -r -F -d 'Patch script to apply'
complete -c kmgr -s c -l config    -r -F -d 'Configuration to build with'

complete -c kmgr -s k -l kernel    -x -a '(kmgr --complete-for kernels)'   -d 'Kernel version to work on'
complete -c kmgr -s K -l oldkernel -x -a '(kmgr --complete-for installed)' -d 'Kernel version to replace'
complete -c kmgr -s s -l suffix    -x -a '(kmgr --complete-for suffixes)'  -d 'Suffix to build under'
complete -c kmgr -s S -l oldsuffix -x -a '(kmgr --complete-for suffixes)'  -d 'Suffix of the kernel being replaced'
complete -c kmgr      -l completion -x -a '(kmgr --complete-for shells)'   -d 'Print the completions for a shell'

# --clean takes a list, so each word is offered onto whatever is already there.
complete -c kmgr -s l -l clean -x -a '(__kmgr_clean)' -d 'What to remove'

function __kmgr_clean
    # fish hands over the whole word, the option it belongs to and all, and
    # puts that part back itself, so only what follows it is ours to build on.
    set -l value (string replace -r '^(--clean=|-l)' '' -- (commandline -ct))
    set -l prefix ""

    if string match -q '*,*' -- $value
        set prefix (string replace -r ',[^,]*$' ',' -- $value)
    end

    set -l chosen (string split ',' -- $value)

    for word in (kmgr --complete-for clean)
        # A word already in the list has nothing left to ask for.
        if not contains -- $word $chosen
            echo $prefix$word
        end
    end
end

complete -c kmgr -s L -l list       -d 'Show what is on disk'
complete -c kmgr -s u -l pull       -d 'Update the repositories in the base directory'
complete -c kmgr -s i -l install    -d 'Install what was built'
complete -c kmgr -s d -l delete     -d 'Remove the build directory only'
complete -c kmgr -s w -l download   -d 'Download and prepare the directory'
complete -c kmgr -s r -l resume     -d 'Skip to building an existing directory'
complete -c kmgr -s x -l nopatch    -d 'Skip patching'
complete -c kmgr      -l revert     -d 'Take applied patches back out'
complete -c kmgr -s g -l clang      -d 'Build with LLVM'
complete -c kmgr      -l gcc        -d 'Build with GCC'
complete -c kmgr      -l custom     -d 'Build with the configured custom flags'
complete -c kmgr -s n -l sign       -d 'Sign the image with sbctl'
complete -c kmgr -s a -l autoupdate -d 'Build and install the newest release'
complete -c kmgr -s f -l force      -d 'Carry on after a step fails'
complete -c kmgr -s v -l version    -d 'Show the version'
complete -c kmgr -s h -l help       -d 'Show the help'
)COMPLETION";

inline constexpr auto ZSH = R"COMPLETION(#compdef kmgr
# kmgr(1) completion for zsh. Install with:
#   kmgr --completion zsh > "${fpath[1]}/_kmgr"

_kmgr() {
    local current previous
    current=${words[CURRENT]}
    previous=${words[CURRENT-1]}

    case "$previous" in
        -k|--kernel)
            compadd -- ${(f)"$(kmgr --complete-for kernels)"}
            return ;;
        -K|--oldkernel)
            compadd -- ${(f)"$(kmgr --complete-for installed)"}
            return ;;
        -s|--suffix|-S|--oldsuffix)
            compadd -- ${(f)"$(kmgr --complete-for suffixes)"}
            return ;;
        -p|--path)
            _files -/
            return ;;
        -P|--patch|-c|--config)
            _files
            return ;;
        --completion)
            compadd -- ${(f)"$(kmgr --complete-for shells)"}
            return ;;
    esac

    # --clean takes a list, so only the word after the last comma is being typed.
    if [[ "$current" == --clean=* || "$current" == -l?* ]]; then
        local head list rest word candidate
        local -a offer

        if [[ "$current" == --clean=* ]]; then
            head="--clean="
            list="${current#--clean=}"
        else
            head="-l"
            list="${current#-l}"
        fi

        rest=""
        word="$list"

        if [[ "$list" == *,* ]]; then
            rest="${list%,*},"
            word="${list##*,}"
        fi

        for candidate in ${(f)"$(kmgr --complete-for clean)"}; do
            # A word already in the list has nothing left to ask for.
            if [[ ",$list," != *",$candidate,"* ]]; then
                offer+=("$head$rest$candidate")
            fi
        done

        compadd -- $offer
        return
    fi

    compadd -- ${(f)"$(kmgr --complete-for flags)"}
}

_kmgr "$@"
)COMPLETION";

inline int script(const std::string &shell) {
    if (shell == "bash") {
        std::cout << BASH;
    } else if (shell == "fish") {
        std::cout << FISH;
    } else if (shell == "zsh") {
        std::cout << ZSH;
    } else {
        std::cerr << "No completions for \"" << shell << "\", expected bash, zsh or fish\n";

        return 1;
    }

    return 0;
}
}


#endif //KERNELMGR_COMPLETION_H
