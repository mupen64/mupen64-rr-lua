#pragma once

int rsmp_resample(short** dst, int dst_freq, const short* src, int src_freq, int src_bitrate, int src_len);

int rsmp_get_resample_len(int dst_freq, int src_freq, int src_bitrate, int src_len);
