#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace AV
{

inline void free_object(AVFormatContext *ptr)
{
    avformat_free_context(ptr);
}
inline void free_object(AVCodecContext *ptr)
{
    avcodec_free_context(&ptr);
}
inline void free_object(AVFrame *ptr)
{
    av_frame_free(&ptr);
}
inline void free_object(AVPacket *ptr)
{
    av_packet_free(&ptr);
}
inline void free_object(SwsContext *ptr)
{
    sws_free_context(&ptr);
}
inline void free_object(SwrContext *ptr)
{
    swr_free(&ptr);
}

/**
 * @brief Simple wrapping pointer class for FFmpeg objects.
 *
 * @tparam T
 */
template <class T> class FFPointer
{
  public:
    FFPointer(nullptr_t = nullptr) {}

    FFPointer(const FFPointer &) = delete;
    FFPointer &operator=(const FFPointer &) = delete;

    FFPointer(FFPointer &&rhs) : m_ptr(rhs.m_ptr) { rhs.m_ptr = nullptr; }
    FFPointer &operator=(FFPointer &&rhs)
    {
        m_ptr = rhs.m_ptr;
        rhs.m_ptr = nullptr;
        return *this;
    }

    ~FFPointer()
    {
        if (m_ptr != nullptr) free_object(m_ptr);
    }

    operator T *() { return m_ptr; }
    operator const T *() const { return m_ptr; }

    T *operator->() { return m_ptr; }
    const T *operator->() const { return m_ptr; }

    /**
     * @brief Tries to change the pointer from null to a non-null value.
     *
     * @param pointer The pointer to set.
     * @return Whether the operation succeeded.
     */
    bool try_set(T *pointer)
    {
        release();
        if (pointer == nullptr) return false;

        m_ptr = pointer;
        return true;
    }

    /**
     * @brief Calls a function taking a double-pointer as its first argument to set or reset this pointer.
     * @note This is intended to facilitate functions like `avformat_alloc_output_context2`.
     * @return The function's return value.
     */
    template <class F, class... Args>
        requires(std::is_invocable_r_v<int, F, T **, Args...>)
    int set_via(F fn, Args &&...args)
    {
        int err = fn(&m_ptr, std::forward<Args>(args)...);
        return err;
    }

    /**
     * @brief Releases the currently held object.
     * @return true if an object was freed, false if the pointer was already null.
     */
    bool release()
    {
        bool result = m_ptr != nullptr;
        if (result) free_object(m_ptr);
        m_ptr = nullptr;
        return result;
    }

  private:
    T *m_ptr;
};
using PFormatContext = FFPointer<AVFormatContext>;
using PCodecContext = FFPointer<AVCodecContext>;
using PFrame = FFPointer<AVFrame>;
using PPacket = FFPointer<AVPacket>;
using PSwsContext = FFPointer<SwsContext>;
using PSwrContext = FFPointer<SwrContext>;

inline AVFrame *alloc_video_frame(int width, int height, AVPixelFormat fmt)
{
    AVFrame *result = av_frame_alloc();
    if (result == nullptr) return nullptr;

    result->width = width;
    result->height = height;
    result->format = fmt;
    if (av_frame_get_buffer(result, 0) < 0)
    {
        av_frame_free(&result);
        return nullptr;
    }

    return result;
}

inline AVFrame *alloc_audio_frame(int nb_samples, const AVChannelLayout& ch_layout, AVSampleFormat fmt)
{
    AVFrame *result = av_frame_alloc();
    if (result == nullptr) return nullptr;
    result->nb_samples = nb_samples;
    result->ch_layout = ch_layout;
    result->format = fmt;
    if (av_frame_get_buffer(result, 0) < 0)
    {
        av_frame_free(&result);
        return nullptr;
    }

    return result;
}

inline AVPixelFormat pick_best_pixel_fmt(AVCodecContext* codec_ctx, AVPixelFormat target) {
    const AVPixelFormat* pix_fmts;
    int size;
    
    if (avcodec_get_supported_config(codec_ctx, codec_ctx->codec, AV_CODEC_CONFIG_PIX_FORMAT, 0, (const void**) &pix_fmts, &size) < 0) {
        return AV_PIX_FMT_NONE;
    }

    AVPixelFormat best = pix_fmts[0];
    for (int i = 1; i < size; i++) {
        if (pix_fmts[i] == target) return target;
        best = av_find_best_pix_fmt_of_2(best, pix_fmts[i], target, 0, NULL);
    }

    return best;
}

inline AVSampleFormat pick_best_sample_fmt(AVCodecContext* codec_ctx, AVSampleFormat target) {
    const AVSampleFormat* sample_fmts;
    int size;
    
    if (avcodec_get_supported_config(codec_ctx, codec_ctx->codec, AV_CODEC_CONFIG_PIX_FORMAT, 0, (const void**) &sample_fmts, &size) < 0) {
        return AV_SAMPLE_FMT_NONE;
    }

    for (int i = 0; i < size; i++) {
        if (sample_fmts[i] == target) return target;
    }
    return sample_fmts[0];
}

} // namespace AV