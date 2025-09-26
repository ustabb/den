// src/container/mp4_writer.cpp
#include "streaming/container/mp4_writer.hpp"
#include "streaming/utils/logger.hpp"
#include <algorithm>

namespace streaming {
namespace container {

MP4Writer::MP4Writer() {
    // Default compatible brands
    compatible_brands_.push_back("isom");
    compatible_brands_.push_back("iso2");
    compatible_brands_.push_back("avc1");
    compatible_brands_.push_back("mp41");
}

MP4Writer::~MP4Writer() {
    close();
}

bool MP4Writer::open(const std::string& filename, bool for_writing) {
    if (initialized_ && file_.is_open()) {
        close();
    }
    
    filename_ = filename;
    file_.open(filename, std::ios::binary | std::ios::out);
    
    if (!file_.is_open()) {
        LOG_ERROR("Failed to open file: {}", filename);
        return false;
    }
    
    // Write initial header
    if (!write_header()) {
        LOG_ERROR("Failed to write MP4 header");
        return false;
    }
    
    initialized_ = true;
    LOG_INFO("MP4Writer opened: {}", filename);
    return true;
}

bool MP4Writer::write_header() {
    if (!write_ftyp_box()) return false;
    
    if (fast_start_) {
        // Write MOOV at the beginning for fast start
        if (!write_moov_box()) return false;
        moov_position_ = static_cast<uint64_t>(file_.tellp());
    }
    
    // Start first MDAT box
    mdat_position_ = static_cast<uint64_t>(file_.tellp());
    if (!write_mdat_box({})) return false;
    
    return true;
}

bool MP4Writer::write_ftyp_box() {
    BoxHeader header;
    header.type = MP4Boxes::FTYP;
    header.size = 8 + 4 + 4 + (compatible_brands_.size() * 4); // Base size
    
    // Write header
    file_.write(reinterpret_cast<const char*>(&header.size), sizeof(header.size));
    file_.write(reinterpret_cast<const char*>(&header.type), sizeof(header.type));
    
    // Write major brand
    uint32_t major = (major_brand_[0] << 24) | (major_brand_[1] << 16) | 
                     (major_brand_[2] << 8) | major_brand_[3];
    file_.write(reinterpret_cast<const char*>(&major), sizeof(major));
    file_.write(reinterpret_cast<const char*>(&minor_version_), sizeof(minor_version_));
    
    // Write compatible brands
    for (const auto& brand : compatible_brands_) {
        uint32_t brand_code = (brand[0] << 24) | (brand[1] << 16) | 
                              (brand[2] << 8) | brand[3];
        file_.write(reinterpret_cast<const char*>(&brand_code), sizeof(brand_code));
    }
    
    return file_.good();
}

bool MP4Writer::write_sample(uint32_t track_id, const uint8_t* data, 
                           uint32_t size, uint64_t timestamp, bool is_sync_sample) {
    if (!initialized_ || !file_.is_open()) {
        return false;
    }
    
    auto track_it = tracks_.find(track_id);
    if (track_it == tracks_.end()) {
        LOG_ERROR("Track not found: {}", track_id);
        return false;
    }
    
    TrackInfo& track = track_it->second;
    
    // Create sample info
    SampleInfo sample;
    sample.offset = current_fragment_data_.size();
    sample.size = size;
    sample.timestamp = timestamp;
    sample.duration = track.timescale / 30; // Assume 30fps for now
    sample.is_sync_sample = is_sync_sample;
    
    // Add sample to track
    track.samples.push_back(sample);
    
    // Add data to current fragment
    current_fragment_data_.insert(current_fragment_data_.end(), data, data + size);
    
    LOG_DEBUG("Written sample: track={}, size={}, ts={}, sync={}", 
              track_id, size, timestamp, is_sync_sample);
    
    return true;
}

bool MP4Writer::create_fragment() {
    if (current_fragment_data_.size() >= config_.max_fragment_size) {
        if (!finalize_fragment()) {
            return false;
        }
    }
    
    // Start new fragment
    current_fragment_data_.clear();
    fragment_sequence_++;
    
    return true;
}

bool MP4Writer::finalize_fragment() {
    if (current_fragment_data_.empty()) {
        return true; // Nothing to finalize
    }
    
    // Write MOOF box
    if (!write_moof_box(fragment_sequence_)) {
        return false;
    }
    
    // Write MDAT box with fragment data
    if (!write_mdat_box(current_fragment_data_)) {
        return false;
    }
    
    // Update track sample tables
    for (auto& [track_id, track] : tracks_) {
        if (!update_stbl_boxes(track_id)) {
            LOG_ERROR("Failed to update sample table for track: {}", track_id);
            return false;
        }
    }
    
    LOG_INFO("Finalized fragment: sequence={}, size={} bytes", 
             fragment_sequence_, current_fragment_data_.size());
    
    current_fragment_data_.clear();
    return true;
}

bool MP4Writer::write_moof_box(uint32_t sequence_number) {
    BoxHeader header;
    header.type = MP4Boxes::MOOF;
    
    // Calculate MOOF size (simplified)
    uint32_t moof_size = 8; // Header
    
    // For each track, add TRAF box
    for (const auto& [track_id, track] : tracks_) {
        moof_size += 8 + 20; // TRAF header + TFHD box
        moof_size += track.samples.size() * 16; // TRUN boxes
    }
    
    header.size = moof_size;
    
    file_.write(reinterpret_cast<const char*>(&header.size), sizeof(header.size));
    file_.write(reinterpret_cast<const char*>(&header.type), sizeof(header.type));
    
    // Write MFHD box (Movie Fragment Header)
    uint32_t mfhd_size = 16;
    uint32_t mfhd_type = 0x6D666864; // 'mfhd'
    uint32_t mfhd_flags = 0;
    
    file_.write(reinterpret_cast<const char*>(&mfhd_size), sizeof(mfhd_size));
    file_.write(reinterpret_cast<const char*>(&mfhd_type), sizeof(mfhd_type));
    file_.write(reinterpret_cast<const char*>(&mfhd_flags), sizeof(mfhd_flags));
    file_.write(reinterpret_cast<const char*>(&sequence_number), sizeof(sequence_number));
    
    // Write TRAF boxes for each track
    for (const auto& [track_id, track] : tracks_) {
        // Write TRAF box...
    }
    
    return file_.good();
}

void MP4Writer::close() {
    if (file_.is_open()) {
        // Finalize any pending fragment
        if (!current_fragment_data_.empty()) {
            finalize_fragment();
        }
        
        // Update MOOV position if using fast start
        if (fast_start_ && moov_position_ > 0) {
            // Seek back and update MOOV
            file_.seekp(static_cast<std::streamoff>(moov_position_));
            write_moov_box();
        }
        
        file_.close();
        LOG_INFO("MP4Writer closed: {}", filename_);
    }
    
    initialized_ = false;
}

} // namespace container
} // namespace streaming