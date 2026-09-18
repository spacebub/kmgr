// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <curl/curl.h>
#include <filesystem>
#include <cstring>
#include "Download.h"

namespace {
    struct Memory {
        char *memory;
        size_t size;
    };

    struct ProgressTX {
        CURL *curl;
        Progress *counter;
    };
}

static size_t write_callback(const void *ptr, size_t size, size_t nmemb, void *userdata);
static size_t progress_callback(void *clientp,
                                curl_off_t dltotal,
                                curl_off_t dlnow,
                                curl_off_t ultotal,
                                curl_off_t ulnow);

Download::Download(const std::string &url) {
    _url = url;
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

Download::~Download() {
    curl_global_cleanup();
}

std::string Download::get_page() const {
    CURL *curl = curl_easy_init();

    if (!curl) {
        throw std::runtime_error("Could not initialize CURL instance!");
    }

    Memory chunk{};
    chunk.memory = static_cast<char *>(malloc(1));
    chunk.size = 0;

    if (!chunk.memory) {
        throw std::runtime_error("Could not allocate chunk memory!");
    }

    curl_easy_setopt(curl, CURLOPT_URL, _url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void *>(&chunk));

    if(const CURLcode result = curl_easy_perform(curl); result != CURLE_OK) {
        throw std::runtime_error(curl_easy_strerror(result));
    }

    std::string page(chunk.memory);

    curl_easy_cleanup(curl);
    free(chunk.memory);

    return page;
}

void Download::perform(const std::string &destination,
                       Progress *counter) const {
    CURL *curl = curl_easy_init();

    if (!curl) {
        throw std::runtime_error("Could not initialize CURL instance!");
    }

    FILE *file = fopen(destination.c_str(), "w");

    if (!file) {
        throw std::runtime_error("Could not open file for writing!");
    }

    curl_easy_setopt(curl, CURLOPT_URL, _url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

    ProgressTX tx {
        .curl = curl,
        .counter = counter,
    };

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &tx);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);

    const CURLcode result = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    fclose(file);

    if (result != CURLE_OK) {
        throw std::runtime_error(curl_easy_strerror(result));
    }
}

static size_t write_callback(const void *ptr,
                             const size_t size,
                             const size_t nmemb,
                             void *userdata) {
    const size_t realsize = size * nmemb;
    const auto mem = static_cast<Memory *>(userdata);

    const auto newmem = static_cast<char *>(realloc(mem->memory, mem->size + realsize * 2));

    if(!ptr) {
        throw std::runtime_error("Ran out of memory while downloading page!");
    }

    mem->memory = newmem;
    memcpy(&mem->memory[mem->size], ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = '\0';

    return realsize;
}

static size_t progress_callback(void *clientp,
                                const curl_off_t dltotal,
                                const curl_off_t dlnow,
                                curl_off_t,
                                curl_off_t) {
    const auto progress = static_cast<ProgressTX *>(clientp);

    if (!progress->counter) {
        return 0;
    }

    progress->counter->set_total(dltotal);
    progress->counter->set_completed(dlnow);

    return progress->counter->is_cancel_requested() ? 1 : 0;
}
