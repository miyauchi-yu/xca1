#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <zip.h>
#include "../logger/logger.hpp"

class Analysis {
    public:
        Analysis(std::string filePath, int maxPasswordLen);
        ~Analysis();
        std::atomic<uint64_t> triedCount{0};
        uint64_t totalCount{0};
        std::atomic<bool> isRunning{false};
        std::atomic<bool> found{false};
        std::string run();
    private:
        std::string filePath;
        int maxPasswordLen;
        std::string analyzedPassword;
        const std::string charset =
                "0123456789"
                "abcdefghijklmnopqrstuvwxyz"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "!@#$%^&*()-_=+[]{};:'\",.<>/?\\|";
        std::string result;
        std::mutex resultMutex;
        bool search(zip_uint64_t fileIndex, std::string& attempt, int remainingDigits);
        bool testPassword(zip_uint64_t fileIndex, const std::string& password);
        void worker(char firstChar, int maxDigits);
        uint64_t pow_u64(uint64_t base, int exp);
};
