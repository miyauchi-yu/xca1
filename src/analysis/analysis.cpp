#include "analysis.hpp"

Analysis::Analysis(std::string filePath, int maxPasswordLen) {
    this->filePath = filePath;
    this->maxPasswordLen = maxPasswordLen;
}

Analysis::~Analysis()
{
}

std::string Analysis::run() {
    const int maxDigits = maxPasswordLen;
    const zip_uint64_t fileIndex = 0;

    size_t maxThreads = std::thread::hardware_concurrency();
    if (maxThreads == 0) {
        maxThreads = 4;
    }

    std::vector<std::thread> threads;

    for (char c : charset) {
        threads.emplace_back(&Analysis::worker, this, c, maxDigits);

        if (threads.size() >= maxThreads) {
            for (auto& t : threads) {
                t.join();
            }
            threads.clear();

            if (found.load()) break;
        }
    }

    for (auto& t : threads) {
        t.join();
    }

    analyzedPassword = result;
    return result.empty() ? "?" : result;
}

void Analysis::worker(char firstChar, int maxDigits) {
    std::string attempt;
    attempt.push_back(firstChar);

    for (int len = 1; len <= maxDigits && !found.load(); len++) {
        if (len == 1) {
            if (testPassword(0, attempt)) {
                std::lock_guard<std::mutex> lock(resultMutex);
                result = attempt;
                found.store(true);
                return;
            }
        } else {
            search(0, attempt, len - 1);
        }
    }
}

bool Analysis::search(
        zip_uint64_t fileIndex,
        std::string& attempt,
        int remainingDigits
        ) {
    if (found.load()) return true;

    if (remainingDigits == 0) {
        if (testPassword(fileIndex, attempt)) {
            std::lock_guard<std::mutex> lock(resultMutex);
            result = attempt;
            found.store(true);
            return true;
        }
        return false;
    }

    for (char c : charset) {
        if (found.load()) return true;
        attempt.push_back(c);
        if (search(fileIndex, attempt, remainingDigits - 1)) {
            return true;
        }
        attempt.pop_back();
    }

    return false;
}

bool Analysis::testPassword(
        zip_uint64_t fileIndex,
        const std::string& password
        ) {
    int err = 0;
    zip_t* za = zip_open(filePath.c_str(), ZIP_RDONLY, &err);
    if (!za) return false;

    zip_file_t* zf =
        zip_fopen_index_encrypted(za, fileIndex, 0, password.c_str());

    if (!zf) {
        zip_close(za);
        return false;
    }

    char buf[4096];
    while (true) {
        zip_int64_t n = zip_fread(zf, buf, sizeof(buf));
        if (n < 0) {
            zip_fclose(zf);
            zip_close(za);
            return false;
        }
        if (n == 0) break;
    }

    zip_fclose(zf);
    zip_close(za);
    return true;
}
