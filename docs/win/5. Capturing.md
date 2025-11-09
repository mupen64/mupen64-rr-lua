Capturing a gameplay video using Mupen64's capture functionality is recommended
over using external capture software such as OBS, as the video and audio streams
are guaranteed to be steady and clean.

This page details some tips for capturing.

# 1 - Choose a video plugin

✅ Do

Use a video plugin which either:

- implements MGE (e.g. bettergln64)

or

- implements readScreen correctly (e.g. GLideN64)

❌ Don't

Use a plugin such as Jabo's Direct3D8 or Direct64, as they are too unstable
during capture.

# 2 - Choose an encoder

There are currently two encoders: VFW and FFmpeg.

## VFW

VFW is the legacy encoder implementation.

To produce videos with reasonable quality and size, it's recommended to install
the [x264vfw codec](https://sourceforge.net/projects/x264vfw/) and select it in
the VFW codec dialog upon starting a capture.

When using x264vfw, you must enable "Zero Latency" in the codec settings
("Configure..." in the codec dialog) to avoid desyncs.

## FFmpeg

The FFmpeg encoder requires an installation of FFmpeg to work.

To install FFmpeg, download the
[latest release build](https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-full.7z)
and extract it to your `C:\` drive.

If FFmpeg is still not detected by Mupen, check that the FFmpeg path in the
Mupen settings matches the path in the filesystem.

# 3 - Choose a mode

| Criterium                                    | Plugin | Window | Screen | Hybrid |
| -------------------------------------------- | ------ | ------ | ------ | ------ |
| Performance is acceptable                    | ✅     | ✅     | ❌     | ✅     |
| Lua Capture                                  | ❌     | ❌     | ✅     | ✅     |
| Works when minimized or occluded             | ✅     | ✅     | ❌     | ✅     |
| Works with plugins without MGE or readScreen | ❌     | ✅     | ✅     | ❌     |

It's generally recommended to only use the Hybrid capture mode, unless you have
special requirements.

# Troubleshooting

## The recording is black but has audio

Check that you have selected a capture mode compatible with your use-case. It's
recommended to use the Hybrid capture mode.

If using FFmpeg, verify that your encoding parameters are valid and accepted by
FFmpeg by checking the debug output.

## Mupen crashes after a few frames have gone by

If you are encoding at large resolutions using VFW, Mupen might have run out of
memory.

Try using FFmpeg instead.

If you are encoding at large resolutions using FFmpeg, the memory pressure can
be very high during the first 30 frames of capture, leaving Mupen to operate
with barely a few megabytes of memory.

Avoid performing any action in the UI, such as opening dialogs or starting movie
playback.

## I am getting the error "Audio frequency changed during capture. The capture will be stopped."

The emulator can't be reset during a capture.

Consequently, you shouldn't do anything which resets the emulator, such as
pressing `Reset ROM` or playing back a movie which starts from console startup,
while a capture is running.
