// include/streaming/container/media_container.hpp
#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <map>

namespace streaming {
namespace container {

// Container Format Types
enum class ContainerFormat {
    MP4_LIKE,    // MP4 benzeri (fragmented)
    WEBM_LIKE,   // WebM benzeri (Matroska tabanlı)
    TS_LIKE,     // MPEG-TS benzeri (streaming için optimize)
    CUSTOM       // Kendi özel formatımız
};

// Track Types
enum class TrackType {
    VIDEO_TRACK,
    AUDIO_TRACK,
    SUBTITLE_TRACK,
    METADATA_TRACK
};

// Codec Types
enum class CodecType {
    H264,
    H265,
    AV1,
    VP9,
    OPUS,
    AAC,
    PCM
};

// Box/Atom Structure (MP4 style)
struct BoxHeader {
    uint32_t size;          // Box size including header
    uint32_t type;          // Box type (FourCC)
    uint64_t large_size;    // For boxes > 4GB
    uint8_t version;        // Box version
    uint32_t flags;         // Box flags
};

// Sample (Frame) Information
struct SampleInfo {
    uint64_t offset;        // File offset
    uint32_t size;          // Sample size
    uint64_t timestamp;     // Presentation timestamp
    uint64_t duration;      // Sample duration
    bool is_sync_sample;    // Keyframe flag
    uint32_t composition_offset; // B-frame offset
};

// Track Information
struct TrackInfo {
    uint32_t track_id;
    TrackType track_type;
    CodecType codec_type;
    uint32_t timescale;     // Ticks per second
    uint32_t duration;      // Track duration in timescale units
    uint32_t width;         // Video width
    uint32_t height;        // Video height
    uint32_t sample_rate;   // Audio sample rate
    uint16_t channels;      // Audio channels
    std::vector<uint8_t> codec_config; // Codec configuration
    std::vector<SampleInfo> samples;
};

// Container Configuration
struct ContainerConfig {
    ContainerFormat format = ContainerFormat::CUSTOM;
    uint32_t timescale = 1000000; // 1MHz clock
    uint64_t duration = 0;
    bool fragmented = true;      // Streaming için fragmented
    bool fast_start = true;      // MOOV önde
    uint32_t fragment_duration = 2000; // 2 second fragments
    uint32_t max_fragment_size = 4000000; // 4MB max fragment
};

class MediaContainer {
public:
    MediaContainer();
    virtual ~MediaContainer() = default;
    
    bool initialize(const ContainerConfig& config);
    virtual bool open(const std::string& filename, bool for_writing) = 0;
    virtual void close() = 0;
    
    // Track management
    uint32_t add_track(const TrackInfo& track_info);
    bool remove_track(uint32_t track_id);
    TrackInfo* get_track(uint32_t track_id);
    
    // Sample writing
    virtual bool write_sample(uint32_t track_id, const uint8_t* data, 
                             uint32_t size, uint64_t timestamp, 
                             bool is_sync_sample) = 0;
    
    // Fragmentation
    virtual bool create_fragment() = 0;
    virtual bool finalize_fragment() = 0;
    
    // Metadata
    void add_metadata(const std::string& key, const std::string& value);
    bool write_metadata();
    
    // Utility functions
    uint64_t get_duration() const { return config_.duration; }
    uint32_t get_timescale() const { return config_.timescale; }
    ContainerFormat get_format() const { return config_.format; }

protected:
    virtual bool write_header() = 0;
    virtual bool write_track_headers() = 0;
    virtual bool write_index() = 0;
    
    ContainerConfig config_;
    std::map<uint32_t, TrackInfo> tracks_;
    std::map<std::string, std::string> metadata_;
    uint32_t next_track_id_ = 1;
    bool initialized_ = false;
};

} // namespace container
} // namespace streaming