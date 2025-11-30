#ifndef PAN64_RESAMPLER_HPP_INCLUDED
#define PAN64_RESAMPLER_HPP_INCLUDED

#include <memory>
#include <span>
#include <samplerate.h>
#include <vector>

namespace Pan64
{

/**
 * @brief Custom deleter that calls a global function.
 *
 * @tparam T The type of the pointer.
 * @tparam Del The deletion function.
 */
template <class T, auto Del>
    requires requires(T *ptr) { Del(ptr); }
struct DeleteFunc
{
    void operator()(T *ptr) { Del(ptr); }
};

/**
 * @brief Smart pointer for SRC_STATE.
 */
using SrcStatePtr = std::unique_ptr<SRC_STATE, DeleteFunc<SRC_STATE, src_delete>>;

class Resampler
{
  public:
    virtual ~Resampler() {}

    virtual void reserve_input(size_t size) {}
    virtual void reserve_output(size_t size) {}

    /**
     * @brief Resamples enough audio to fill the output buffer, assuming in_data contains enough data.
     * @note All sample data is assumed to be 16-bit interleaved stereo.
     *
     * @param in_data A span containing the input sample data.
     * @param in_rate The input sample rate, in Hz.
     * @param out_data A span containing the output sample data.
     * @param out_rate The output sample rate, in Hz.
     * @return The number of input frames consumed.
     */
    virtual size_t resample(std::span<uint8_t> in_data, uint32_t in_rate, std::span<uint8_t> out_data,
                            uint32_t out_rate) = 0;
};

class SrcResampler : public Resampler
{
  public:
    SrcResampler(int resampler_type = SRC_SINC_MEDIUM_QUALITY);

    virtual ~SrcResampler() {}

    virtual void reserve_input(size_t size) {
      m_ibuf.reserve(size);
    }
    virtual void reserve_output(size_t size) {
      m_obuf.reserve(size);
    }

    virtual size_t resample(std::span<uint8_t> in_data, uint32_t in_rate, std::span<uint8_t> out_data,
                            uint32_t out_rate);

  private:
    SrcStatePtr m_state;
    std::vector<float> m_ibuf;
    std::vector<float> m_obuf;
};
} // namespace Pan64

#endif