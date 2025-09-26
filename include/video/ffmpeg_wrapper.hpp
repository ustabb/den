// include/streaming/video/ffmpeg_wrapper.hpp
#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "../engine/types.hpp"
#include <memory>
#include <string>

namespace streaming {
namespace video {

class FFmpegWrapper {
public:
    FFmpegWrapper();
    ~FFmpegWrapper();
    
    // Initialization
    bool initialize(const StreamConfig& config);
    void cleanup();
    
    // Video processing
    bool open_input(const std::string& filename);
    bool open_output(const std::string& url, int width, int height);
    
    // Frame processing
    AVFrame* read_frame();
    bool encode_frame(AVFrame* frame);
    bool send_packet(AVPacket* packet);
    
    // Streaming optimizations
    void set_streaming_optimizations();
    void enable_low_latency_mode(bool enable);
    void set_bitrate(int bitrate);
    
    // Getters
    AVCodecContext* get_video_codec_ctx() const { return video_codec_ctx_; }
    AVFormatContext* get_format_ctx() const { return fmt_ctx_; }
    int get_video_stream_index() const { return video_stream_index_; }

private:
    bool init_codec(const StreamConfig& config);
    bool init_hardware_acceleration();
    void setup_streaming_parameters();
    
private:
    AVFormatContext* fmt_ctx_ = nullptr;
    AVCodecContext* video_codec_ctx_ = nullptr;
    AVCodec* video_codec_ = nullptr;
    SwsContext* sws_ctx_ = nullptr;
    
    int video_stream_index_ = -1;
    StreamConfig config_;
    bool low_latency_mode_ = true;
    
    // Hardware acceleration
    AVBufferRef* hw_device_ctx_ = nullptr;
    AVPixelFormat hw_pix_fmt_ = AV_PIX_FMT_NONE;
};

} // namespace video
} // namespace streaming