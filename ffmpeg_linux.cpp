#include "ffmpeg.h"
#include <raylib.h>
#include <unistd.h>
#include <cassert>
#include <errno.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sys/wait.h>
#include <cstdint>

#define READ_END 0
#define WRITE_END 1

struct FFMPEG {
    pid_t pid;
    int write_pipe;
};

FFMPEG* ffmpeg_create(int fps, int width, int heigth, const char *output_path) {
    int pipes[2];
    if (pipe(pipes) == -1) {
        TraceLog(LOG_ERROR, "FFMPEG: Could not create pipes, error: %s", strerror(errno));
        return nullptr;
    }

    pid_t pid = fork();
    if (pid == -1) {       // error
        TraceLog(LOG_ERROR, "FFMPEG: Could not fork, error: %s", strerror(errno));
        return nullptr;
    } else if (pid == 0) { // child
        if (dup2(pipes[READ_END], STDIN_FILENO) == -1) {
            TraceLog(LOG_ERROR, "FFMPEG CHILD: Could not reopen read end of pipe as stdin, error: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        close(pipes[WRITE_END]);

        char resolution[64] {}, framerate[64] {};
        sprintf(resolution, "%dx%d", width, heigth);
        sprintf(framerate, "%d", fps);

        int ret = execlp("ffmpeg",
            "ffmpeg",
            "-loglevel", "verbose",
            "-y",

            "-f", "rawvideo",
            "-pix_fmt", "rgba",
            "-video_size", resolution,
            "-r", framerate,
            "-i", "-",

            "-c:v", "libx264",
            "-vb", "10000k",
            "-pix_fmt", "yuv420p",
            output_path,

            NULL
        );
        if (ret == -1) {
            TraceLog(LOG_ERROR, "[ERROR] Error: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        assert(false && "Unreachable");
    }

    if (close(pipes[READ_END]) == -1) {
        TraceLog(LOG_WARNING, "FFMPEG: could not close read end of the pipe on the parent's end: %s", strerror(errno));
    }

    FFMPEG *result = new FFMPEG;
    result->pid = pid;
    result->write_pipe = pipes[WRITE_END];

    return result;
}

bool ffmpeg_stop(FFMPEG *ffmpeg, bool cancel) {
    int pipe = ffmpeg->write_pipe;
    pid_t pid = ffmpeg->pid;

    delete ffmpeg;

    if (close(pipe) < 0) {
        TraceLog(LOG_WARNING, "FFMPEG: could not close write end of the pipe on the parent's end: %s", strerror(errno));
    }

    if (cancel) kill(pid, SIGKILL);

    for (;;) {
        int wstatus = 0;
        if (waitpid(pid, &wstatus, 0) < 0) {
            TraceLog(LOG_ERROR, "FFMPEG: could not wait for ffmpeg child process to finish: %s", strerror(errno));
            return false;
        }

        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) {
                TraceLog(LOG_ERROR, "FFMPEG: ffmpeg exited with code %d", exit_status);
                return false;
            }

            return true;
        }

        if (WIFSIGNALED(wstatus)) {
            TraceLog(LOG_ERROR, "FFMPEG: ffmpeg got terminated by %s", strsignal(WTERMSIG(wstatus)));
            return false;
        }
    }

    assert(0 && "unreachable");
}

bool ffmpeg_send_frame_flipped(FFMPEG *ffmpeg, void *data, size_t width, size_t height) {
    for (size_t y = height; y > 0; --y) {
        // TODO: write() may not necessarily write the entire row. We may want to repeat the call.
        if (write(ffmpeg->write_pipe, (uint32_t*)data + (y - 1)*width, sizeof(uint32_t)*width) < 0) {
            TraceLog(LOG_ERROR, "FFMPEG: failed to write into ffmpeg pipe: %s", strerror(errno));
            return false;
        }
    }
    return true;
}
