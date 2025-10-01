#include "hls_segmenter.hpp"
#include <fstream>
#include <sstream>

HlsSegmenter::HlsSegmenter(const std::string& dir) 
    : outputDir(dir), segmentIndex(0) {}

void HlsSegmenter::addTag(const FlvTag& tag) {
    // TODO: convert FLV video/audio data to MPEG-TS packets
    // for now, just simulate by writing raw data
    std::ostringstream filename;
    filename << outputDir << "/segment_" << segmentIndex++ << ".ts";
    std::ofstream out(filename.str(), std::ios::binary);
    out.write((char*)tag.data.data(), tag.data.size());
    out.close();
    segments.push_back(filename.str());
}

void HlsSegmenter::writePlaylist() {
    std::ofstream playlist(outputDir + "/index.m3u8");
    playlist << "#EXTM3U\n#EXT-X-VERSION:3\n";
    for (const auto& seg : segments) {
        playlist << "#EXTINF:5,\n" << seg << "\n";
    }
    playlist.close();
}
