#include "LAVCEncoder.hpp"

extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
}

#include <capture/CaptureManager.hpp>

std::optional<std::wstring> LAVCEncoder::start(Params params)
{
    // TODO: configurable format
    const auto *ofmt = av_guess_format(nullptr, nullptr, "video/mp4");
    if (ofmt != nullptr) return L"Container format not found";

    m_params = std::move(params);

    auto filename = params.path.u8string();

    if (m_fmt_ctx.set_via(avformat_alloc_output_context2, ofmt, nullptr, (const char *)filename.c_str()) < 0)
        return L"Failed to setup AVFormatContext";

    // PICKING CODECS
    // ==================

    // TODO: configurable codecs
    m_video.codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (m_video.codec == nullptr) return L"Failed to find suitable video codec";
    m_audio.codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (m_audio.codec == nullptr) return L"Failed to find suitable audio codec";

    // VIDEO SETUP
    // ==================
    if (!m_video.alloc_objects(m_fmt_ctx)) return L"Failed to allocate video codec";
    // basic settings
    m_video.codec_ctx->width = (int)params.width;
    m_video.codec_ctx->height = (int)params.height;
    m_video.codec_ctx->time_base = {1, (int)params.fps};
    // global headers
    if ((ofmt->flags & AVFMT_GLOBALHEADER) != 0) m_video.codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // init the video codec
    if (!m_video.prepare_codec()) return L"Failed to prepare video codec";

    // setup source frame for conversion
    if (!m_vsrc_frame.try_set(av_frame_alloc())) return L"Failed to allocate video source frame";
    m_vsrc_frame->width = (int)params.width;
    m_vsrc_frame->height = (int)params.height;
    m_vsrc_frame->format = AV_PIX_FMT_BGR0;
    // set fixed linesize (all frames will use the same linesize)
    memset((void *)m_vsrc_frame->linesize, 0, sizeof(m_vsrc_frame->linesize));
    m_vsrc_frame->linesize[0] = m_vsrc_frame->width * 4;

    // setup and allocate buffer frame for conversion
    if (!m_vbuf_frame.try_set(
            AV::alloc_video_frame(m_video.codec_ctx->width, m_video.codec_ctx->height, m_video.codec_ctx->pix_fmt)))
        return L"Failed to allocate video buffer frame";

    // fill buffer frame with black (this is required for sync)
    if (av_image_fill_black(m_vbuf_frame->data, m_vbuf_frame->linesize, (AVPixelFormat)m_vbuf_frame->format,
                            m_video.codec_ctx->color_range, m_vbuf_frame->width, m_vbuf_frame->height) < 0)
        return L"Failed to clear video frame";

    // setup swscale context
    if (!m_sws_ctx.try_set(sws_alloc_context())) return L"Failed to alloc swscale context";
    if (sws_frame_setup(m_sws_ctx, m_vbuf_frame, m_vsrc_frame) < 0) return L"Failed to prepare swscale";

    // reset PTS
    m_video_pts = 0;

    // AUDIO SETUP
    // ==================
    const AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
    if (!m_audio.alloc_objects(m_fmt_ctx)) return L"Failed to allocate audio codec";
    // basic settings
    m_audio.codec_ctx->ch_layout = stereo;
    m_audio.codec_ctx->sample_rate = 48000;
    m_audio.codec_ctx->bit_rate = 196000;
    m_audio.codec_ctx->time_base = {1, 48000};
    // global headers
    if ((ofmt->flags & AVFMT_GLOBALHEADER) != 0) m_audio.codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // init the audio codec
    if (!m_audio.prepare_codec()) return L"Failed to prepare audio codec";

    // setup source frame for conversion (actually not used in the conversion LMAO)
    if (!m_asrc_frame.try_set(av_frame_alloc())) return L"Failed to allocate audio source frame";
    m_asrc_frame->ch_layout = stereo;
    m_asrc_frame->sample_rate = (int)params.arate;
    m_asrc_frame->format = AV_SAMPLE_FMT_S16;

    // setup and allocate buffer frame for conversion
    m_audio_frame_size = (m_audio.codec_ctx->frame_size == 0) ? 4096 : m_audio.codec_ctx->frame_size;
    if (!m_abuf_frame.try_set(AV::alloc_audio_frame(m_audio_frame_size, stereo, m_audio.codec_ctx->sample_fmt)))
        return L"Failed to allocate audio buffer frame";

    // setup swresample context
    if (!m_swr_ctx.try_set(swr_alloc())) return L"Failed to alloc swr context";
    if (swr_config_frame(m_swr_ctx, m_abuf_frame, m_asrc_frame) < 0) return L"Failed to prepare swresample";
    if (swr_init(m_swr_ctx)) return L"Failed to prepare swresample";

    // reset PTS
    m_audio_pts = 0;

    // CONTAINER SETUP
    // ==================

    // open the output file if needed
    if ((ofmt->flags & AVFMT_NOFILE) == 0)
    {
        if (avio_open(&m_fmt_ctx->pb, (const char *)filename.c_str(), AVIO_FLAG_WRITE) < 0)
            return L"Failed to open output file";
    }

    // write the file header
    if (avformat_write_header(m_fmt_ctx, nullptr) < 0) return L"Failed to write stream header";

    return std::nullopt;
}
bool LAVCEncoder::stop()
{
    // FLUSH VIDEO
    // =====================

    // push a null frame to free up any samples stuck in the codec
    if (m_video.push_frame(m_fmt_ctx, nullptr) < 0) return false;

    // FLUSH AUDIO
    // =====================

    // reallocate the audio frame to fit the number of remaining samples
    int nb_samples_left = swr_get_out_samples(m_swr_ctx, 0);
    if (nb_samples_left > 0)
    {
        // unref the frame-sized buffer we were using
        av_frame_unref(m_abuf_frame);
        // setup a buffer big enough to hold the last few samples
        m_abuf_frame->nb_samples = nb_samples_left;
        if (av_frame_get_buffer(m_abuf_frame, 0) < 0) return false;
        // extract the samples
        if (swr_convert(m_swr_ctx, m_abuf_frame->data, m_abuf_frame->nb_samples, nullptr, 0) < 0) return false;
        // encode the frame
        m_audio.push_frame(m_fmt_ctx, m_abuf_frame);
    }

    // push a null frame to free up any samples stuck in the codec
    m_audio.push_frame(m_fmt_ctx, nullptr);

    // TRAILER
    // =====================

    // write the file trailer
    return av_write_trailer(m_fmt_ctx) < 0;
}
bool LAVCEncoder::append_video(uint8_t *image)
{
    const auto sync = static_cast<CaptureManager::Sync>(g_config.synchronization_mode);

    switch (sync)
    {
    case CaptureManager::Sync::None:
    case CaptureManager::Sync::Video:
        // read/scale the frame
        scale_frame(image);
        // push the frame
        m_vbuf_frame->pts = m_video_pts++;
        if (m_video.push_frame(m_fmt_ctx, m_vbuf_frame) < 0) return false;
        break;
    case CaptureManager::Sync::Audio:
        constexpr int64_t drift_threshold = 3;
        const int64_t audio_pos = av_rescale_q(m_audio_pts, m_audio.codec_ctx->time_base, m_video.codec_ctx->time_base);
        const int64_t drift = audio_pos - m_video_pts;

        // video is ahead of audio, drop frame
        if (drift < -drift_threshold)
        {
            return true;
        }
        // we need to send this frame, read and scale it
        scale_frame(image);
        // push one frame
        m_vbuf_frame->pts = m_video_pts++;
        if (m_video.push_frame(m_fmt_ctx, m_vbuf_frame) < 0) return false;
        // if we're ahead, push a 2nd frame
        if (drift > drift_threshold)
        {
            m_vbuf_frame->pts = m_video_pts++;
            if (m_video.push_frame(m_fmt_ctx, m_vbuf_frame) < 0) return false;
        }
        break;
    }
    return true;
}
bool LAVCEncoder::append_audio(uint8_t *audio, size_t length, uint8_t)
{
    int sample_count = (int)length / 4;
    // feed resampler
    if (swr_convert(m_swr_ctx, nullptr, 0, &audio, sample_count) < 0) return false;
    // pull frames from the resampler as long as we can pull out full frames
    while (swr_get_out_samples(m_swr_ctx, 0) >= m_audio_frame_size)
    {
        if (swr_convert(m_swr_ctx, m_abuf_frame->data, m_abuf_frame->nb_samples, nullptr, 0) < 0) return false;
        // set PTS
        m_abuf_frame->pts = m_audio_pts;
        m_audio_pts += m_abuf_frame->nb_samples;
        // encode the frame
        m_audio.push_frame(m_fmt_ctx, m_abuf_frame);
    }
}
std::wstring LAVCEncoder::get_desired_extension() const
{
    // TODO: configurable container format
    return L".mp4";
}

int LAVCEncoder::EncodeStream::alloc_objects(AVFormatContext *fmt_ctx)
{
    assert(codec != nullptr);

    stream = avformat_new_stream(fmt_ctx, codec);
    if (stream == nullptr) return AVERROR(EINVAL);
    if (!codec_ctx.try_set(avcodec_alloc_context3(codec))) return AVERROR(EINVAL);
    if (!packet.try_set(av_packet_alloc())) return AVERROR(ENOMEM);
}
int LAVCEncoder::EncodeStream::prepare_codec()
{
    int err = avcodec_open2(codec_ctx, codec, nullptr);
    if (err < 0) return err;
    return avcodec_parameters_from_context(stream->codecpar, codec_ctx);
}
int LAVCEncoder::EncodeStream::push_frame(AVFormatContext *fmt_ctx, AVFrame *frame)
{
    int err = avcodec_send_frame(codec_ctx, frame);
    if (err < 0) return err;

    while ((err = avcodec_receive_packet(codec_ctx, packet)) >= 0)
    {
        // adjust packet information to the correct time base
        av_packet_rescale_ts(packet, codec_ctx->time_base, stream->time_base);
        packet->stream_index = stream->index;

        err = av_interleaved_write_frame(fmt_ctx, packet);
        if (err < 0) return err;
    }
    if (err < 0 && err != AVERROR(EAGAIN) && err != AVERROR_EOF) return err;

    return 0;
}

bool LAVCEncoder::scale_frame(uint8_t *image)
{
    // write source frame data
    memset((void *)m_vsrc_frame->data, 0, sizeof(m_vsrc_frame->data));
    m_vsrc_frame->data[0] = image;
    // scale the frame
    return sws_scale_frame(m_sws_ctx, m_vbuf_frame, m_vsrc_frame) < 0;
}