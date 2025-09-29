#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <mutex>

class VODManager {
public:
    VODManager(const std::string& storagePath);
    void saveSegment(const std::vector<uint8_t>& encodedFrame);
    void generatePlaylist();

private:
    std::string storagePath_;
    int segmentIndex_;
    std::mutex fileMutex_;
    std::vector<std::string> segments_;
};
