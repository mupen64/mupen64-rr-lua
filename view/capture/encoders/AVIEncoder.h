#pragma once
#include "Encoder.h"

#include <Vfw.h>
#include <Windows.h>

class AVIEncoder final : public Encoder {
public:
    bool start(Params params) override;
    bool stop() override;
    bool append_video(uint8_t* image) override;
    bool append_audio(uint8_t* audio, size_t length, uint8_t bitrate) override;

private:
    // 44100=1s sample, soundbuffer capable of holding 4s future data in circular
    // buffer
    static constexpr auto SOUND_BUF_SIZE = 44100 * 2 * 2;
    static constexpr auto MAX_AVI_SIZE = 0x7B9ACA00;
    static constexpr auto RESAMPLED_FREQ = 44100;

    bool write_sound(uint8_t* buf, int len, int min_write_size, int max_write_size, BOOL force, uint8_t bitrate);
    bool append_video_impl(uint8_t* image);
    bool save_options();
    bool load_options();

    Params m_params{};
    AVICOMPRESSOPTIONS* m_avi_options = new AVICOMPRESSOPTIONS();

    bool m_splitting = false;
    size_t m_splits = 0;

    size_t m_frame = 0;
    size_t m_sample = 0;
    double_t m_video_frame = 0;
    double_t m_audio_frame = 0;
    uint8_t m_sound_buf[SOUND_BUF_SIZE];
    uint8_t m_sound_buf_empty[SOUND_BUF_SIZE];
    int sound_buf_pos = 0;
    long last_sound = 0;

    BITMAPINFOHEADER m_info_hdr{};
    PAVIFILE m_avi_file{};

    AVISTREAMINFO m_video_stream_hdr{};
    PAVISTREAM m_video_stream{};
    PAVISTREAM m_compressed_video_stream{};

    AVISTREAMINFO m_sound_stream_hdr{};
    PAVISTREAM m_sound_stream{};
    WAVEFORMATEX m_sound_format{};

    size_t m_avi_file_size = 0;
};
