#pragma once
// Segmenter: Splits live streams into HLS segments
namespace storage {
class Segmenter {
public:
	Segmenter(const std::string& outputDir);
	void addFrame(const std::vector<uint8_t>& encodedFrame);
	void finalizeSegment();
	// Add segment file writing logic here
};
}
