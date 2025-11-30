#include "SampleBuffer.hpp"
#include <cmath>

namespace Pan64
{
SampleBuffer::SampleBuffer(Config config) {
  m_sample_count = 0;
}

void SampleBuffer::set_dac_rate(uint32_t dac_rate)
{
}

void SampleBuffer::push_samples(const void *data, size_t len)
{
}

int SampleBuffer::pa_pull(const void *ibuf, void *obuf, unsigned long n_frames,
                          const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags status_flags)
{
  float* optr = (float*) obuf;

  for (uint64_t i = 0; i < n_frames; i++) {
    uint64_t t_samples = m_sample_count + i;

    double sin_left = sin(2.0 * M_PI * 440.0 * t_samples / 48000.0);
    double sin_right = sin(2.0 * M_PI * 660.0 * t_samples / 48000.0);

    *optr++ = sin_left;
    *optr++ = sin_right;
  }
  m_sample_count += n_frames;
  return paContinue;
}
} // namespace Pan64