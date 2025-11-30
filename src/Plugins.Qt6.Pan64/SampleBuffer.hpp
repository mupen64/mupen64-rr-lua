#ifndef PAN64_SAMPLE_BUFFER_HPP_INCLUDED
#define PAN64_SAMPLE_BUFFER_HPP_INCLUDED

#include <atomic>
#include <memory>
#include <portaudio.h>
#include <vector>

#include "Config.hpp"
#include "Resampler.hpp"

namespace Pan64
{


class SampleBuffer final
{
  public:
    SampleBuffer(Config config);

    void set_dac_rate(uint32_t dac_rate);

    void push_samples(const void* data, size_t len);

    /**
     * @brief Audio callback for PortAudio.
     *
     * @param ibuf The input buffer (for audio capture).
     * @param obuf The output buffer (for audio playback).
     * @param n_frames The number of frames (sets of samples for multiple buffers) in the buffer.
     * @param time_info Timing information from PortAudio.
     * @param status_flags Status flags from PortAudio.
     * @return A status code to pass back to PortAudio.
     */
    int pa_pull(const void *ibuf, void *obuf, unsigned long n_frames, const PaStreamCallbackTimeInfo *time_info,
                PaStreamCallbackFlags status_flags);

  private:
    // std::vector<uint8_t> m_in_buffer;
    // std::unique_ptr<Resampler> m_resampler;
    uint64_t m_sample_count;

    uint32_t m_sample_rate;
    std::atomic<uint32_t> m_next_sample_rate;

    bool m_audio_sync;
};
} // namespace Pan64

#endif