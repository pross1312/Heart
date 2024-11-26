#ifndef FFMPEG_H
#define FFMPEG_H
#include <cstddef>

struct FFMPEG;

FFMPEG* ffmpeg_create(int fps, int width, int heigth, const char *output_path);
bool ffmpeg_send_frame_flipped(FFMPEG *ffmpeg, void *data, size_t width, size_t height);
bool ffmpeg_stop(FFMPEG *ffmpeg, bool cancel);

#endif // FFMPEG_H
