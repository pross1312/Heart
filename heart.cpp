#include <unistd.h>

#include <cmath>
#include <ctime>
#include <raylib.h>
#include <cstdint>
#include <raymath.h>

#include "ffmpeg.h"

constexpr size_t POINT_COUNT = 90;
constexpr float RADIUS = 1.0f;
constexpr float SCALE = 12.0f;
constexpr size_t PARTICLE_COUNT = 120;
constexpr float DEFAULT_RADIUS = 110.0f;
constexpr float ANIMATE_TIME = 1.2f;
constexpr float PARTICLE_DISTANCE_SCALE_WHEN_GROWING = 2.5f;
constexpr float CENTER_DISTANCE_SCALE_WHEN_GROWING = 1.2f;
constexpr float HEART_SCALE_SMALLEST = 1.0f;
constexpr float HEART_SCALE_BIGGEST = 1.5f;
constexpr float RENDERING_TIME = 5.0f; //seconds
constexpr const char* OUTPUT_PATH = "output.mp4";
constexpr int FPS = 60;
constexpr int RENDER_FPS = 30;
constexpr int RENDER_WIDTH = 1200;
constexpr int RENDER_HEIGHT = 900;
constexpr uint32_t HEART_COLOR = 0xff0000ff; 
int ffmpeg_pipe[2] {};

struct ControlPoint {
    Vector2 position;
    float scale;
    Vector2 particles[PARTICLE_COUNT];
};

float heart_scale = 1.0f;
ControlPoint heart_points[POINT_COUNT];

Vector2 heart_equation(float t) {
    return Vector2 {
        .x = float(16.0f*pow(sinf(t), 3)),
        .y = -float(13*cosf(t)-5*cosf(2*t)-2*cosf(3*t)-cos(4*t)),
    };
}

void generate_random_particle(ControlPoint *point) {
    for (size_t i = 0; i < PARTICLE_COUNT; i++) {
        Vector2 particle_direction = {
            .x = rand()*1.0f/RAND_MAX - 0.5f,
            .y = rand()*1.0f/RAND_MAX - 0.5f,
        };
        float radius = rand()*1.0f/RAND_MAX;
        point->particles[i] = particle_direction * radius;
    }
}

float ease_in_out(float t, float minVal, float maxVal) {
    float tNormalized = t / ANIMATE_TIME;

    float range = maxVal - minVal;
    if (tNormalized < 0.5) {
        return minVal + pow(2 * tNormalized, 3.3f) * range;
    } else {
        float x = (tNormalized - 0.5f) * 2;
        return maxVal - pow(x, 1.3f) * range;
    }
}

void update(float ease, float width, float height) {
    ClearBackground(BLACK);

    Vector2 half_screen_size = {
        .x = width/2.0f,
        .y = height/2.0f
    };
    for (size_t i = 0; i < POINT_COUNT; i++) {
        Vector2 center = half_screen_size + heart_points[i].position * pow(ease, CENTER_DISTANCE_SCALE_WHEN_GROWING);
        DrawCircleV(center, RADIUS, GetColor(HEART_COLOR));
        for (size_t j = 0; j < PARTICLE_COUNT; j++) {
            Vector2 particle_position = center + heart_points[i].particles[j] * DEFAULT_RADIUS * pow(ease, PARTICLE_DISTANCE_SCALE_WHEN_GROWING);
            DrawCircleV(particle_position, RADIUS, GetColor(HEART_COLOR));
        }
    }
}

int main() {
    srand(time(0));

    int factor = 110;
    InitWindow(12*factor, 9*factor, "Beating Heart");

    SetTargetFPS(FPS);

    for (size_t i = 0; i < POINT_COUNT; i++) {
        Vector2 center = heart_equation(i*(2.0f*M_PI)/POINT_COUNT);
        heart_points[i].position = center*SCALE;
        generate_random_particle(&heart_points[i]);
    }

    FFMPEG *ffmpeg = nullptr;
    float rendering_timer = RENDERING_TIME;
    RenderTexture2D texture2D = LoadRenderTexture(RENDER_WIDTH, RENDER_HEIGHT);
    TraceLog(LOG_INFO, "Format: %d", texture2D.texture.format);

    float time = 0;
    while (!WindowShouldClose()) {
        if (ffmpeg != nullptr) {
            time += 1.0f/RENDER_FPS;
        } else {
            time += GetFrameTime();
        }
        if (time > ANIMATE_TIME) time -= ANIMATE_TIME;

        if (ffmpeg == nullptr && IsKeyPressed(KEY_SPACE)) {
            ffmpeg = ffmpeg_create(RENDER_FPS, RENDER_WIDTH, RENDER_HEIGHT, OUTPUT_PATH);
            if (ffmpeg != nullptr) {
                rendering_timer = RENDERING_TIME;
            }
        }

        if (ffmpeg != nullptr) {
            if (rendering_timer < 0) {
                ffmpeg_stop(ffmpeg, false);
                ffmpeg = nullptr;
            } else {
                rendering_timer -= 1.0f/RENDER_FPS;
            }
        }

        float ease = ease_in_out(time, HEART_SCALE_SMALLEST, HEART_SCALE_BIGGEST);

        BeginDrawing();
        if (ffmpeg != nullptr) {
            BeginTextureMode(texture2D);
            update(ease, RENDER_WIDTH, RENDER_HEIGHT);
            EndTextureMode();
            EndDrawing();
        } else {
            DrawText("Press space to start render video", 0, 0, 20, WHITE);
            update(ease, GetScreenWidth(), GetScreenHeight());
        }
        EndDrawing();

        if (ffmpeg != nullptr) {
            Image image = LoadImageFromTexture(texture2D.texture);
            if (!ffmpeg_send_frame_flipped(ffmpeg, image.data, image.width, image.height)) {
                ffmpeg_stop(ffmpeg, false);
                ffmpeg = nullptr;
            }
            UnloadImage(image);
        }
    }
    CloseWindow();
    return 0;
}
