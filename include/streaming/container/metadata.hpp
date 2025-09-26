// include/streaming/container/metadata.hpp
#pragma once

#include <string>
#include <map>
#include <vector>

namespace streaming {
namespace container {

class MetadataManager {
public:
    struct MetadataEntry {
        std::string key;
        std::string value;
        std::string language; // ISO 639-2 language code
        uint64_t timestamp;   // When metadata applies
        uint64_t duration;    // How long it's valid
    };

    MetadataManager();
    
    void add_metadata(const std::string& key, const std::string& value, 
                     const std::string& language = "und", 
                     uint64_t timestamp = 0, uint64_t duration = 0);
    
    bool remove_metadata(const std::string& key, uint64_t timestamp = 0);
    std::vector<MetadataEntry> get_metadata_at_time(uint64_t timestamp) const;
    
    // Serialization
    std::vector<uint8_t> serialize_metadata() const;
    bool deserialize_metadata(const uint8_t* data, size_t size);

    // Standard metadata fields
    void set_title(const std::string& title);
    void set_artist(const std::string& artist);
    void set_album(const std::string& album);
    void set_year(uint32_t year);
    void set_genre(const std::string& genre);
    void set_duration(uint64_t duration);
    void set_video_resolution(uint32_t width, uint32_t height);

private:
    std::vector<MetadataEntry> metadata_;
    mutable std::mutex metadata_mutex_;
};

} // namespace container
} // namespace streaming