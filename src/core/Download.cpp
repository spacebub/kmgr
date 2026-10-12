// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <memory>
#include <mutex>
#include <stdexcept>

#include "Download.h"

namespace {
constexpr long CONNECT_TIMEOUT = 20;
constexpr long STALL_SECONDS = 60;
constexpr long STALL_BYTES = 1024;
constexpr long PAGE_TIMEOUT = 60;
constexpr size_t PAGE_LIMIT = 8 << 20;

std::once_flag s_started;

using Handle = std::unique_ptr<CURL, void (*)(CURL *)>;

Handle prepare(const std::string &url) {
    std::call_once(s_started, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });

    Handle curl(curl_easy_init(), curl_easy_cleanup);

    if (!curl) {
        throw std::runtime_error("Could not set up a transfer for " + url);
    }

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl.get(), CURLOPT_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(curl.get(), CURLOPT_REDIR_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
    curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "kernelmgr/" KERNELMGR_VERSION);

    return curl;
}

std::string describe(CURL *curl, const CURLcode result) {
    if (result == CURLE_HTTP_RETURNED_ERROR) {
        long status = 0;

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

        return "HTTP " + std::to_string(status);
    }

    return curl_easy_strerror(result);
}

size_t collect(const char *data, const size_t size, const size_t count, void *target) {
    auto *page = static_cast<std::string *>(target);
    const size_t bytes = size * count;

    if (page->size() + bytes > PAGE_LIMIT) {
        return 0;
    }

    page->append(data, bytes);

    return bytes;
}

size_t store(const char *data, const size_t size, const size_t count, void *target) {
    return fwrite(data, size, count, static_cast<FILE *>(target));
}

int report(void *target, const curl_off_t total, const curl_off_t now, curl_off_t, curl_off_t) {
    auto *counter = static_cast<Progress *>(target);

    if (!counter) {
        return 0;
    }

    counter->set_total(total);
    counter->set_completed(now);

    return counter->is_cancel_requested() ? 1 : 0;
}
}

Download::Download(std::string url) : _url(std::move(url)) {
}

std::string Download::get_page() const {
    const Handle curl = prepare(_url);
    std::string page;

    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, collect);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &page);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, PAGE_TIMEOUT);

    if (const CURLcode result = curl_easy_perform(curl.get()); result != CURLE_OK) {
        throw std::runtime_error(_url + ": " + describe(curl.get(), result));
    }

    return page;
}

void Download::perform(const std::string &destination, Progress *counter) const {
    const Handle curl = prepare(_url);
    FILE *file = fopen(destination.c_str(), "wb");

    if (!file) {
        throw std::runtime_error("Could not open " + destination + " for writing: " + strerror(errno));
    }

    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, store);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, counter);
    curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, report);
    curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_LIMIT, STALL_BYTES);
    curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_TIME, STALL_SECONDS);

    const CURLcode result = curl_easy_perform(curl.get());
    const bool written = fflush(file) == 0 && !ferror(file);
    const int writeError = errno;
    const bool closed = fclose(file) == 0;

    if (result != CURLE_OK) {
        throw std::runtime_error(_url + ": " + describe(curl.get(), result));
    }

    if (!written || !closed) {
        throw std::runtime_error("Could not write " + destination + ": " + strerror(written ? errno : writeError));
    }
}
