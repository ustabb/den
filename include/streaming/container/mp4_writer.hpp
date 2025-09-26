// include/streaming/container/mp4_writer.hpp
#pragma once

#include "media_container.hpp"
#include <fstream>

namespace streaming {
namespace container {

class MP4Writer : public MediaContainer {
private:
    // MP4 Specific Box Types
    struct MP4Boxes {
        static constexpr uint32_t FTYP = 0x66747970; // 'ftyp'
        static constexpr uint32_t MOOV = 0x6D6F6F76; // 'moov'
        static constexpr uint32_t MOOF = 0x6D6F6F66; // 'moof'
        static constexpr uint32_t MDAT = 0x6D646174; // 'mdat'
        static constexpr uint32_t TRAK = 0x7472616B; // 'trak'
        static constexpr uint32_t MVHD = 0x6D766864; // 'mvhd'
        static constexpr uint32_t TKHD = 0x746B6864; // 'tkhd'
        static constexpr uint32_t MDIA = 0x6D646961; // 'mdia'
        static constexpr uint32_t MINF = 0x6D696E66; // 'minf'
        static constexpr uint32_t STBL = 0x7374626C; // 'stbl'
        static constexpr uint32_t STSD = 0x73747364; // 'stsd'
        static constexpr uint32_t STTS = 0x73747473; // 'stts'
        static constexpr uint32_t STSC = 0x73747363; // 'stsc'
        static constexpr uint32_t STSZ = 0x7374737A; // 'stsz'
        static constexpr uint32_t STCO = 0x7374636F; // 'stco'
        static constexpr uint32_t STSS = 0x73747373; // 'stss'
    };

public:
    MP4Writer();
    ~MP4Writer() override;
    
    bool open(const std::string& filename, bool for_writing) override;
    void close() override;
    
    bool write_sample(uint32_t track_id, const uint8_t* data, 
                     uint32_t size, uint64_t timestamp, 
                     bool is_sync_sample) override;
    
    bool create_fragment() override;
    bool finalize_fragment() override;

    // MP4-specific features
    void set_brand(const std::string& major_brand, uint32_t version = 0);
    void add_compatible_brand(const std::string& brand);
    void enable_fast_start(bool enable);

private:
    bool write_header() override;
    bool write_track_headers() override;
    bool write_index() override;
    
    // MP4 box writing functions
    bool write_ftyp_box();
    bool write_moov_box();
    bool write_mvhd_box();
    bool write_trak_box(uint32_t track_id);
    bool write_moof_box(uint32_t sequence_number);
    bool write_mdat_box(const std::vector<uint8_t>& sample_data);
    
    // Sample management
    bool update_stbl_boxes(uint32_t track_id);
    bool write_sample_table(uint32_t track_id);

private:
    std::ofstream file_;
    std::string filename_;
    
    // MP4 specific state
    std::string major_brand_ = "isom";
    uint32_t minor_version_ = 0;
    std::vector<std::string> compatible_brands_;
    
    // Fragment state
    uint32_t fragment_sequence_ = 1;
    uint64_t fragment_start_offset_ = 0;
    std::vector<uint8_t> current_fragment_data_;
    
    // Fast start optimization
    bool fast_start_ = true;
    uint64_t moov_position_ = 0;
    uint64_t mdat_position_ = 0;
};

} // namespace container
} // namespace streaming