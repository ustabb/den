// include/streaming/container/webm_writer.hpp
#pragma once

#include "media_container.hpp"
#include <fstream>

namespace streaming {
namespace container {

class WebMWriter : public MediaContainer {
private:
    // EBML Element IDs
    struct EBMLIds {
        static constexpr uint32_t EBML = 0x1A45DFA3;
        static constexpr uint32_t SEGMENT = 0x18538067;
        static constexpr uint32_t INFO = 0x1549A966;
        static constexpr uint32_t TRACKS = 0x1654AE6B;
        static constexpr uint32_t CLUSTER = 0x1F43B675;
        static constexpr uint32_t SIMPLE_BLOCK = 0xA3;
    };

public:
    WebMWriter();
    ~WebMWriter() override;
    
    bool open(const std::string& filename, bool for_writing) override;
    void close() override;
    
    bool write_sample(uint32_t track_id, const uint8_t* data, 
                     uint32_t size, uint64_t timestamp, 
                     bool is_sync_sample) override;
    
    bool create_fragment() override;
    bool finalize_fragment() override;

    // WebM-specific features
    void set_doc_type(const std::string& doc_type);
    void set_codec_private_data(uint32_t track_id, const std::vector<uint8_t>& data);

private:
    bool write_header() override;
    bool write_track_headers() override;
    bool write_index() override;
    
    // EBML writing functions
    bool write_ebml_header();
    bool write_segment_header();
    bool write_info_element();
    bool write_tracks_element();
    bool write_cluster(uint64_t timestamp);
    bool write_simple_block(uint32_t track_id, const uint8_t* data, 
                           uint32_t size, uint64_t relative_time, 
                           bool keyframe);
    
    // EBML utility functions
    void write_ebml_element(uint32_t id, const std::vector<uint8_t>& data);
    void write_ebml_uint(uint32_t id, uint64_t value);
    void write_ebml_string(uint32_t id, const std::string& value);
    void write_vint(uint64_t value); // Variable-sized integer

private:
    std::ofstream file_;
    std::string filename_;
    std::string doc_type_ = "webm";
    
    // WebM state
    uint64_t segment_offset_ = 0;
    uint64_t cluster_offset_ = 0;
    uint64_t current_cluster_time_ = 0;
    uint32_t cluster_count_ = 0;
};

} // namespace container
} // namespace streaming