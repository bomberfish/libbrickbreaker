#ifndef BRICKBREAKER_RENDERER_H
#define BRICKBREAKER_RENDERER_H

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdint>
#include <memory>

namespace libbrickbreaker {
class Game;
}

class AudioEngine;

struct android_app;

class Renderer {
public:
    explicit Renderer(android_app *pApp);
    ~Renderer();

    void handleInput();
    void render();

private:
    void initRenderer();
    void updateRenderArea();

    void initializeGame();
    void loadAssets();
    void dispatchPointerEvent(int type, float x, float y);
    std::int32_t screenToBoardX(float x) const;
    std::int32_t screenToBoardY(float y) const;

    void initializePipeline();
    void updateFrameTime();
    void updateGame();

    void drawScene();
    void drawRect(float x, float y, float w, float h, float r, float g, float b, float a) const;
    void drawTexturedRect(float x,
                          float y,
                          float w,
                          float h,
                          GLuint texture,
                          float u0,
                          float v0,
                          float u1,
                          float v1,
                          float alpha = 1.0f) const;

    struct Texture {
        GLuint id{0};
        std::int32_t width{0};
        std::int32_t height{0};

        bool loaded() const { return id != 0 && width > 0 && height > 0; }
    };

    Texture loadTexture(const char* assetPath) const;

    android_app *app_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    std::unique_ptr<libbrickbreaker::Game> game_;
    std::unique_ptr<AudioEngine> audio_;
    bool touchActive_;
    std::int32_t touchPointerId_;
    float touchStartX_;
    float touchStartY_;
    bool touchMoved_;
    std::int64_t lastFrameTimeMs_;

    std::int32_t elapsedMs_{16};
    std::int32_t ballFrameTick_{0};
    std::int32_t pillFrameTick_{0};

    GLuint program_{0};
    GLuint vbo_{0};
    GLuint vao_{0};
    GLint colorLocation_{-1};
    GLint useTextureLocation_{-1};
    GLint textureLocation_{-1};

    Texture bgTexture_{};
    Texture bricksTexture_{};
    Texture paddlesTexture_{};
    Texture paddleLongTexture_{};
    Texture pillsTexture_{};
    Texture laserTexture_{};
    Texture bombTexture_{};
    Texture ballsTexture_{};
};

#endif
