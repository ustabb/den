#pragma once
#include <string>
#include <vector>
#include "flv_parser.hpp"

class HlsSegmenter {
public:
    HlsSegmenter(const std::string& outputDir);
    void addTag(const FlvTag& tag);
    void writePlaylist();

private:
    std::string outputDir;
    int segmentIndex;
    std::vector<std::string> segments;
};
