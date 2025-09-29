#include "vod_manager.hpp"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

VODManager::VODManager(const std::string& storagePath)
    : storagePath_(storagePath), segmentIndex_(0)
{
    if (!fs::exists(storagePath_)) {
        fs::create_directories(storagePath_);
    }
}

void VODManager::saveSegment(const std::vector<uint8_t>& encodedFrame) {
    std::lock_guard<std::mutex> lock(fileMutex_);

    std::string filename = storagePath_ + "/segment_" + std::to_string(segmentIndex_) + ".ts";
    std::ofstream outFile(filename, std::ios::binary);
    outFile.write(reinterpret_cast<const char*>(encodedFrame.data()), encodedFrame.size());
    outFile.close();

    segments_.push_back(filename);
    segmentIndex_++;
    std::cout << "Segment kaydedildi: " << filename << std::endl;

    // Basit olarak her segment ayrı dosya
}

void VODManager::generatePlaylist() {
    std::lock_guard<std::mutex> lock(fileMutex_);

    std::string playlistFile = storagePath_ + "/playlist.m3u8";
    std::ofstream playlist(playlistFile);
    playlist << "#EXTM3U\n";
    playlist << "#EXT-X-VERSION:3\n";
    playlist << "#EXT-X-TARGETDURATION:10\n";
    playlist << "#EXT-X-MEDIA-SEQUENCE:0\n";

    for (const auto& segment : segments_) {
        playlist << "#EXTINF:10.0,\n";
        playlist << fs::path(segment).filename().string() << "\n";
    }

    playlist << "#EXT-X-ENDLIST\n";
    playlist.close();

    std::cout << "Playlist oluşturuldu: " << playlistFile << std::endl;
}
