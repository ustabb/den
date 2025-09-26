// include/streaming/server/stream_manager.hpp
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace streaming {
namespace server {

class MediaStream {
public:
    struct StreamConfig {
        std::string name;
        std::string source_url;
        uint32_t max_viewers = 100;
        uint32_t max_bitrate = 5000000; // 5 Mbps
        bool record_enabled = false;
        std::string record_path;
        uint32_t segment_duration = 2000; // 2 seconds for HLS
    };

    struct StreamStatistics {
        uint32_t current_viewers;
        uint64_t total_bytes_sent;
        uint32_t current_bitrate;
        uint32_t packet_loss_rate;
        uint64_t uptime;
    };

    MediaStream(const StreamConfig& config);
    ~MediaStream();
    
    bool initialize();
    void add_viewer(const std::string& session_id);
    void remove_viewer(const std::string& session_id);
    void push_media_data(const uint8_t* data, size_t size, uint64_t timestamp, 
                        bool is_video, bool is_keyframe);
    
    // Stream processing
    void transcode_stream();
    void generate_hls_segments();
    void generate_dash_manifest();
    void record_stream();

    // Quality adaptation
    void adapt_bitrate(uint32_t target_bitrate);
    void apply_quality_adjustment(float network_condition);

    StreamStatistics get_statistics() const;

private:
    void process_video_data(const uint8_t* data, size_t size, uint64_t timestamp, bool is_keyframe);
    void process_audio_data(const uint8_t* data, size_t size, uint64_t timestamp);
    void distribute_to_viewers(const std::vector<uint8_t>& packet);

    StreamConfig config_;
    StreamStatistics stats_;
    std::vector<std::string> viewers_;
    
    // Media processors
    std::unique_ptr<video::VideoProcessor> video_processor_;
    std::unique_ptr<audio::AudioProcessor> audio_processor_;
    
    // Recording
    std::unique_ptr<container::MP4Writer> recorder_;
    uint64_t recording_start_time_;
};

class StreamManager {
public:
    StreamManager();
    ~StreamManager();
    
    bool create_stream(const std::string& name, const std::string& source_url = "");
    bool delete_stream(const std::string& name);
    std::shared_ptr<MediaStream> get_stream(const std::string& name);
    
    void push_stream_data(const std::string& stream_name, const uint8_t* data, 
                         size_t size, uint64_t timestamp, bool is_video, bool is_keyframe);
    
    // Stream discovery
    std::vector<std::string> get_active_streams() const;
    bool stream_exists(const std::string& name) const;

private:
    std::unordered_map<std::string, std::shared_ptr<MediaStream>> streams_;
    mutable std::mutex streams_mutex_;
};

} // namespace server
} // namespace streaming