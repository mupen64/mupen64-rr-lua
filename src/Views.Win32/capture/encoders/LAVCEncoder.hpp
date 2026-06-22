#pragma once

#include "Encoder.hpp"
#include "LAVHelpers.hpp"

class LAVCEncoder : public Encoder
{
  public:
    std::optional<std::wstring> start(Params params) override;
    bool stop() override;
    bool append_video(uint8_t *image) override;
    bool append_audio(uint8_t *audio, size_t length, uint8_t bitrate) override;
    std::wstring get_desired_extension() const override;

  private:
    struct EncodeStream {
        const AVCodec* codec {};
        AVStream* stream {};

        AV::PCodecContext codec_ctx;
        AV::PPacket packet;

        int alloc_objects(AVFormatContext* fmt_ctx);
        int prepare_codec();
        int push_frame(AVFormatContext* fmt_ctx, AVFrame* frame);
    };

    // Scales the input video frame to m_vbuf_frame.
    bool scale_frame(uint8_t* image);

    Params m_params{};

    AV::PFormatContext m_fmt_ctx;
    
    EncodeStream m_video;
    AV::PFrame m_vsrc_frame;
    AV::PFrame m_vbuf_frame;
    AV::PSwsContext m_sws_ctx;
    int64_t m_video_pts {0};

    EncodeStream m_audio;
    AV::PFrame m_asrc_frame;
    AV::PFrame m_abuf_frame;
    AV::PSwrContext m_swr_ctx;
    int m_audio_frame_size;
    int64_t m_audio_pts {0};
};