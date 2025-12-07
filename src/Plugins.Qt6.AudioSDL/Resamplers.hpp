#ifndef AUDIOSDL_RESAMPLERS_HPP_INCLUDED
#define AUDIOSDL_RESAMPLERS_HPP_INCLUDED

#include <span>

namespace AudioSDL
{

class IResampler_Old
{
  public:
    virtual ~IResampler_Old() {}

    virtual void prepare(uint32_t src_rate, uint32_t dst_rate, size_t dst_size) = 0;

    virtual size_t required_input(size_t out_frames) = 0;

    virtual size_t resample(std::span<uint16_t> src, std::span<uint16_t> dst) = 0;
};

class IResampler
{
  public:
    virtual ~IResampler() {}

    /**
     * @brief Prepares the resampler for use.
     * 
     * @param src_rate the input sample rate
     * @param dst_rate the output sample rate
     * @param swap_channels if true, swaps the two input channels.
     */
    virtual void prepare(uint32_t src_rate, uint32_t dst_rate, bool swap_channels) = 0;

    /**
     * @brief If possible, allocates input buffers large enough to hold the specified number of frames.
     * @param in_frames the number of input frames
     */
    virtual void reserve_src(size_t in_frames) {}

    /**
     * @brief If possible, allocates output buffers large enough to hold the specified number of frames.
     * @param out_frames the number of output frames
     */
    virtual void reserve_dst(size_t out_frames) {}

    /**
     * @brief Pushes the provided frames to the resampler's queue.
     * 
     * @param samples the frames to push.
     * @return true if the push succeeded. It may fail due to limits on the resampler's buffer sizes, for example. 
     */
    virtual bool push_samples(std::span<const int16_t> samples) = 0;

    /**
     * @brief Returns, in output samples, the amount of audio currently queued at the resampler.
     */
    virtual uint64_t buffer_len_out() = 0;

    /**
     * @brief Tries to pull a chunk of output frames from the resampler.
     * 
     * @param samples A span to write the frames to.
     * @return true if there was enough data to fill the span.
     * @return false if there was too little data to fill the span.
     */
    virtual bool pull_samples(std::span<int16_t> samples) = 0;
};
} // namespace AudioSDL

#endif