#include "Renderer.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <cstring>
#include <memory>
#include <vector>

#include <android/asset_manager.h>
#include <android/bitmap.h>

#include <game-activity/GameActivity.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "AudioEngine.h"
#include "AndroidOut.h"
#include "libbrickbreaker/libbrickbreaker.hpp"

namespace {

using namespace libbrickbreaker;

constexpr std::int32_t kBaseBoardWidth = Board::BASE_WIDTH;
constexpr std::int32_t kBaseBoardHeight = Board::BASE_HEIGHT;
constexpr std::int32_t kMaxFrameMs = 33;
constexpr std::int32_t kBrickFrameRows = 9;

std::int64_t nowMs() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::int64_t>(ts.tv_sec) * 1000 + static_cast<std::int64_t>(ts.tv_nsec / 1000000);
}

std::vector<std::uint8_t> readAssetBytes(AAssetManager* assetManager, const char* assetPath) {
    if (assetManager == nullptr || assetPath == nullptr) {
        return {};
    }
    AAsset* asset = AAssetManager_open(assetManager, assetPath, AASSET_MODE_BUFFER);
    if (asset == nullptr) {
        return {};
    }

    const off_t length = AAsset_getLength(asset);
    if (length <= 0) {
        AAsset_close(asset);
        return {};
    }

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    const int count = AAsset_read(asset, bytes.data(), static_cast<size_t>(length));
    AAsset_close(asset);
    if (count <= 0) {
        return {};
    }

    bytes.resize(static_cast<std::size_t>(count));
    return bytes;
}

std::int32_t brickFrame(std::int32_t value) {
    if (value >= Bricks::INDESTRUCTIBLE) {
        return 8;
    }
    if (value >= 8) {
        return 7;
    }
    if (value >= 6) {
        return 6;
    }
    if (value >= 4) {
        return 5;
    }
    if (value >= 2) {
        return 4;
    }
    if (value >= 1) {
        return 3;
    }
    return 8;
}

std::pair<float, float> uv(float px, float py, std::int32_t width, std::int32_t height) {
    return {
        std::clamp(px / static_cast<float>(std::max<std::int32_t>(1, width)), 0.0f, 1.0f),
        std::clamp(py / static_cast<float>(std::max<std::int32_t>(1, height)), 0.0f, 1.0f),
    };
}

}  // namespace

Renderer::Renderer(android_app* pApp)
    : app_(pApp),
      display_(EGL_NO_DISPLAY),
      surface_(EGL_NO_SURFACE),
      context_(EGL_NO_CONTEXT),
      width_(0),
      height_(0),
      game_(std::make_unique<Game>()),
      audio_(std::make_unique<AudioEngine>()),
      touchActive_(false),
      touchPointerId_(-1),
      touchStartX_(0.0f),
      touchStartY_(0.0f),
      touchMoved_(false),
      lastFrameTimeMs_(nowMs()) {
    initRenderer();
    initializePipeline();
    initializeGame();
    loadAssets();
}

Renderer::~Renderer() {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    for (Texture* texture : std::array<Texture*, 8>{
             &bgTexture_,
             &bricksTexture_,
             &paddlesTexture_,
             &paddleLongTexture_,
             &pillsTexture_,
             &laserTexture_,
             &bombTexture_,
             &ballsTexture_,
         }) {
        if (texture->id != 0) {
            glDeleteTextures(1, &texture->id);
            texture->id = 0;
        }
    }

    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
}

void Renderer::initRenderer() {
    constexpr EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 0,
        EGL_NONE,
    };

    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display_, nullptr, nullptr);

    EGLint numConfigs = 0;
    eglChooseConfig(display_, attribs, nullptr, 0, &numConfigs);
    std::vector<EGLConfig> configs(static_cast<std::size_t>(std::max<EGLint>(1, numConfigs)));
    eglChooseConfig(display_, attribs, configs.data(), numConfigs, &numConfigs);
    const EGLConfig config = configs.empty() ? nullptr : configs[0];

    surface_ = eglCreateWindowSurface(display_, config, app_->window, nullptr);

    constexpr EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context_ = eglCreateContext(display_, config, nullptr, contextAttribs);
    const EGLBoolean ok = eglMakeCurrent(display_, surface_, surface_, context_);
    assert(ok == EGL_TRUE);

    width_ = -1;
    height_ = -1;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::initializePipeline() {
    static const char* kVertexSrc = R"glsl(#version 300 es
layout(location=0) in vec2 in_pos;
layout(location=1) in vec2 in_uv;
out vec2 v_uv;
void main() {
    v_uv = in_uv;
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}
)glsl";

    static const char* kFragmentSrc = R"glsl(#version 300 es
precision mediump float;
uniform vec4 u_color;
uniform int u_useTexture;
uniform sampler2D u_texture;
in vec2 v_uv;
out vec4 outColor;
void main() {
    vec4 base = u_color;
    if (u_useTexture == 1) {
        base = texture(u_texture, v_uv) * u_color;
    }
    outColor = base;
}
)glsl";

    auto compile = [](GLenum type, const char* src) -> GLuint {
        const GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_FALSE) {
            GLint len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
            if (len > 1) {
                std::vector<char> log(static_cast<std::size_t>(len));
                glGetShaderInfoLog(shader, len, nullptr, log.data());
                aout << "Shader compile failed: " << log.data() << std::endl;
            }
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    };

    const GLuint vs = compile(GL_VERTEX_SHADER, kVertexSrc);
    const GLuint fs = compile(GL_FRAGMENT_SHADER, kFragmentSrc);
    assert(vs != 0 && fs != 0);

    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    assert(linked == GL_TRUE);

    colorLocation_ = glGetUniformLocation(program_, "u_color");
    useTextureLocation_ = glGetUniformLocation(program_, "u_useTexture");
    textureLocation_ = glGetUniformLocation(program_, "u_texture");

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(sizeof(float) * 2));
    glBindVertexArray(0);
}

void Renderer::initializeGame() {
    game_->setSchedulingEnabled(false);
    game_->setViewport(kBaseBoardWidth, kBaseBoardHeight);
    game_->resize();
    game_->newGame(1);
}

Renderer::Texture Renderer::loadTexture(const char* assetPath) const {
    Texture out{};
    AAssetManager* assetManager = app_->activity->assetManager;
    if (assetManager == nullptr || assetPath == nullptr) {
        return out;
    }

    AAsset* asset = AAssetManager_open(assetManager, assetPath, AASSET_MODE_BUFFER);
    if (asset == nullptr) {
        return out;
    }

    const off_t length = AAsset_getLength(asset);
    if (length <= 0) {
        AAsset_close(asset);
        return out;
    }

    std::vector<std::uint8_t> encoded(static_cast<std::size_t>(length));
    const int bytesRead = AAsset_read(asset, encoded.data(), static_cast<size_t>(length));
    AAsset_close(asset);
    if (bytesRead <= 0) {
        return out;
    }

    JNIEnv* env = app_->activity->env;
    if (env == nullptr) {
        return out;
    }

    jclass bitmapFactoryClass = env->FindClass("android/graphics/BitmapFactory");
    if (bitmapFactoryClass == nullptr) {
        return out;
    }

    jmethodID decodeMethod = env->GetStaticMethodID(
        bitmapFactoryClass,
        "decodeByteArray",
        "([BII)Landroid/graphics/Bitmap;");
    if (decodeMethod == nullptr) {
        env->DeleteLocalRef(bitmapFactoryClass);
        return out;
    }

    jbyteArray byteArray = env->NewByteArray(bytesRead);
    if (byteArray == nullptr) {
        env->DeleteLocalRef(bitmapFactoryClass);
        return out;
    }
    env->SetByteArrayRegion(
        byteArray,
        0,
        bytesRead,
        reinterpret_cast<const jbyte*>(encoded.data()));

    jobject bitmapObj = env->CallStaticObjectMethod(bitmapFactoryClass, decodeMethod, byteArray, 0, bytesRead);
    env->DeleteLocalRef(byteArray);
    env->DeleteLocalRef(bitmapFactoryClass);

    if (bitmapObj == nullptr) {
        return out;
    }

    AndroidBitmapInfo info{};
    if (AndroidBitmap_getInfo(env, bitmapObj, &info) != ANDROID_BITMAP_RESULT_SUCCESS ||
        info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        env->DeleteLocalRef(bitmapObj);
        return out;
    }

    void* pixels = nullptr;
    if (AndroidBitmap_lockPixels(env, bitmapObj, &pixels) != ANDROID_BITMAP_RESULT_SUCCESS || pixels == nullptr) {
        env->DeleteLocalRef(bitmapObj);
        return out;
    }

    const std::int32_t width = static_cast<std::int32_t>(info.width);
    const std::int32_t height = static_cast<std::int32_t>(info.height);
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(width * height * 4));
    const auto* src = static_cast<const std::uint8_t*>(pixels);
    for (std::int32_t y = 0; y < height; ++y) {
        std::memcpy(
            rgba.data() + static_cast<std::size_t>(y * width * 4),
            src + static_cast<std::size_t>(y * info.stride),
            static_cast<std::size_t>(width * 4));
    }

    AndroidBitmap_unlockPixels(env, bitmapObj);
    env->DeleteLocalRef(bitmapObj);

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    out.id = id;
    out.width = width;
    out.height = height;
    return out;
}

void Renderer::loadAssets() {
    AAssetManager* assetManager = app_->activity->assetManager;
    if (assetManager == nullptr) {
        aout << "Asset manager unavailable" << std::endl;
        return;
    }

    std::vector<std::uint8_t> levelBytes = readAssetBytes(assetManager, "levels.bin");
    if (!levelBytes.empty()) {
        Bricks::setLevelData(levelBytes.data(), static_cast<std::int32_t>(levelBytes.size()));
        game_->newGame(1);
    }

    bgTexture_ = loadTexture("ui/bg.png");
    bricksTexture_ = loadTexture("sprites/bricks.png");
    paddlesTexture_ = loadTexture("sprites/paddles.png");
    paddleLongTexture_ = loadTexture("sprites/paddlelong.png");
    pillsTexture_ = loadTexture("sprites/pills.png");
    laserTexture_ = loadTexture("sprites/laser.png");
    bombTexture_ = loadTexture("sprites/bomb.png");
    ballsTexture_ = loadTexture("sprites/balls.png");

    if (audio_ != nullptr) {
        audio_->loadFromAssets(assetManager, {
                                           "sounds/poppill.ogg",
                                           "sounds/eatpill.ogg",
                                           "sounds/laser.ogg",
                                           "sounds/bomb.ogg",
                                           "sounds/brickdestroy.ogg",
                                           "sounds/brickhit.ogg",
                                           "sounds/ceiling.ogg",
                                           "sounds/paddle.ogg",
                                       });
    }
}

void Renderer::updateRenderArea() {
    EGLint width = 0;
    EGLint height = 0;
    eglQuerySurface(display_, surface_, EGL_WIDTH, &width);
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);
    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        glViewport(0, 0, width_, height_);

        const float boardAspect = static_cast<float>(kBaseBoardWidth) / static_cast<float>(kBaseBoardHeight);
        const float surfaceAspect = width_ > 0 && height_ > 0
            ? static_cast<float>(width_) / static_cast<float>(height_)
            : boardAspect;

        std::int32_t viewportW = kBaseBoardWidth;
        std::int32_t viewportH = kBaseBoardHeight;
        if (surfaceAspect > boardAspect) {
            viewportW = static_cast<std::int32_t>(std::round(kBaseBoardHeight * surfaceAspect));
        } else {
            viewportH = static_cast<std::int32_t>(std::round(kBaseBoardWidth / std::max(0.1f, surfaceAspect)));
        }
        game_->setViewport(std::max<std::int32_t>(1, viewportW), std::max<std::int32_t>(1, viewportH));
        game_->resize();
    }
}

std::int32_t Renderer::screenToBoardX(float x) const {
    if (width_ <= 1) {
        return 0;
    }
    const float clamped = std::clamp(x, 0.0f, static_cast<float>(width_ - 1));
    const Board& board = game_->boardRef();
    const float normalized = clamped / static_cast<float>(width_ - 1);
    return static_cast<std::int32_t>(std::round(normalized * static_cast<float>(std::max(1, board.width))));
}

std::int32_t Renderer::screenToBoardY(float y) const {
    if (height_ <= 1) {
        return 0;
    }
    const float clamped = std::clamp(y, 0.0f, static_cast<float>(height_ - 1));
    const Board& board = game_->boardRef();
    const float normalized = clamped / static_cast<float>(height_ - 1);
    return static_cast<std::int32_t>(std::round(normalized * static_cast<float>(std::max(1, board.height))));
}

void Renderer::dispatchPointerEvent(int type, float x, float y) {
    PointerEvent event;
    switch (type) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            event.type = PointerType::kStart;
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            event.type = PointerType::kMove;
            break;
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
        case AMOTION_EVENT_ACTION_CANCEL:
            event.type = PointerType::kEnd;
            break;
        default:
            return;
    }

    event.x = screenToBoardX(x);
    event.y = screenToBoardY(y);
    game_->touchEvent(event);
}

void Renderer::handleInput() {
    android_input_buffer* inputBuffer = android_app_swap_input_buffers(app_);
    if (inputBuffer == nullptr) {
        return;
    }

    for (std::size_t i = 0; i < inputBuffer->motionEventsCount; ++i) {
        GameActivityMotionEvent& motionEvent = inputBuffer->motionEvents[i];
        const int actionMasked = motionEvent.action & AMOTION_EVENT_ACTION_MASK;
        const int pointerIndex = (motionEvent.action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
            >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        if (pointerIndex >= motionEvent.pointerCount) {
            continue;
        }

        const GameActivityPointerAxes* pointer = &motionEvent.pointers[pointerIndex];
        const int pointerId = pointer->id;
        const float x = GameActivityPointerAxes_getX(pointer);
        const float y = GameActivityPointerAxes_getY(pointer);

        if (actionMasked == AMOTION_EVENT_ACTION_DOWN || actionMasked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            if (!touchActive_) {
                touchActive_ = true;
                touchPointerId_ = pointerId;
                touchStartX_ = x;
                touchStartY_ = y;
                touchMoved_ = false;
                dispatchPointerEvent(actionMasked, x, y);
            }
            continue;
        }

        if (!touchActive_ || pointerId != touchPointerId_) {
            continue;
        }

        if (actionMasked == AMOTION_EVENT_ACTION_MOVE) {
            if (!touchMoved_) {
                const float dx = std::abs(x - touchStartX_);
                const float dy = std::abs(y - touchStartY_);
                touchMoved_ = (dx + dy) > 6.0f;
            }
            dispatchPointerEvent(actionMasked, x, y);
        } else if (actionMasked == AMOTION_EVENT_ACTION_UP
                   || actionMasked == AMOTION_EVENT_ACTION_POINTER_UP
                   || actionMasked == AMOTION_EVENT_ACTION_CANCEL) {
            dispatchPointerEvent(actionMasked, x, y);
            if (!touchMoved_) {
                PointerEvent tap{};
                tap.type = PointerType::kTap;
                tap.x = screenToBoardX(x);
                tap.y = screenToBoardY(y);
                game_->touchEvent(tap);
            }
            touchActive_ = false;
            touchPointerId_ = -1;
        }
    }
    android_app_clear_motion_events(inputBuffer);

    for (std::size_t i = 0; i < inputBuffer->keyEventsCount; ++i) {
        const GameActivityKeyEvent& keyEvent = inputBuffer->keyEvents[i];
        if (keyEvent.action != AKEY_EVENT_ACTION_DOWN && keyEvent.action != AKEY_EVENT_ACTION_MULTIPLE) {
            continue;
        }

        KeyEvent event{};
        switch (keyEvent.keyCode) {
            case AKEYCODE_DPAD_LEFT:
            case AKEYCODE_NUMPAD_4:
                event.keycode = Game::KEY_LEFT;
                break;
            case AKEYCODE_DPAD_RIGHT:
            case AKEYCODE_NUMPAD_6:
                event.keycode = Game::KEY_RIGHT;
                break;
            case AKEYCODE_DPAD_UP:
            case AKEYCODE_NUMPAD_8:
            case AKEYCODE_SPACE:
            case AKEYCODE_DPAD_CENTER:
            case AKEYCODE_ENTER:
                event.keycode = Game::KEY_SPACE;
                break;
            case AKEYCODE_DPAD_DOWN:
            case AKEYCODE_NUMPAD_2:
            case AKEYCODE_P:
                event.keycode = Game::KEY_P;
                break;
            case AKEYCODE_BACK:
            case AKEYCODE_ESCAPE:
                event.keycode = Game::KEY_ESCAPE;
                break;
            default:
                continue;
        }
        event.timeMs = nowMs();
        const std::int32_t repeatCount = std::max<std::int32_t>(1, keyEvent.repeatCount + 1);
        for (std::int32_t repeat = 0; repeat < repeatCount; ++repeat) {
            game_->keyDown(event);
        }
    }
    android_app_clear_key_events(inputBuffer);
}

void Renderer::updateFrameTime() {
    const std::int64_t now = nowMs();
    elapsedMs_ = static_cast<std::int32_t>(std::max<std::int64_t>(0, now - lastFrameTimeMs_));
    elapsedMs_ = std::min<std::int32_t>(elapsedMs_, kMaxFrameMs);
    lastFrameTimeMs_ = now;
}

void Renderer::updateGame() {
    if (game_->paused || game_->gameState() != Game::STATE_PLAYING) {
        return;
    }

    game_->applyInput();  // apply pending touch drag to the paddle before simulating
    game_->boardRef().update(elapsedMs_);
    game_->advanceState();

    ballFrameTick_ = (ballFrameTick_ + 1) % 24;
    pillFrameTick_ = (pillFrameTick_ + 1) % 12;

    if (audio_ != nullptr) {
        audio_->syncFromCore();
    }
}

void Renderer::drawRect(float x, float y, float w, float h, float r, float g, float b, float a) const {
    if (w <= 0.0f || h <= 0.0f) {
        return;
    }

    const GLfloat verts[] = {
        x, y, 0.0f, 0.0f,
        x + w, y, 1.0f, 0.0f,
        x + w, y + h, 1.0f, 1.0f,
        x, y + h, 0.0f, 1.0f,
    };

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glUniform1i(useTextureLocation_, 0);
    glUniform4f(colorLocation_, r, g, b, a);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

void Renderer::drawTexturedRect(float x,
                                float y,
                                float w,
                                float h,
                                GLuint texture,
                                float u0,
                                float v0,
                                float u1,
                                float v1,
                                float alpha) const {
    if (w <= 0.0f || h <= 0.0f) {
        return;
    }

    const GLfloat verts[] = {
        x, y, u0, v0,
        x + w, y, u1, v0,
        x + w, y + h, u1, v1,
        x, y + h, u0, v1,
    };

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);

    if (texture != 0) {
        glUniform1i(useTextureLocation_, 1);
        glUniform4f(colorLocation_, 1.0f, 1.0f, 1.0f, alpha);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(textureLocation_, 0);
    } else {
        glUniform1i(useTextureLocation_, 0);
    }

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

void Renderer::drawScene() {
    const Board& board = game_->boardRef();
    const float bw = static_cast<float>(std::max(1, board.width));
    const float bh = static_cast<float>(std::max(1, board.height));

    if (bgTexture_.loaded()) {
        drawTexturedRect(-1.0f, -1.0f, 2.0f, 2.0f, bgTexture_.id, 0.0f, 0.0f, 1.0f, 1.0f);
    } else {
        drawRect(-1.0f, -1.0f, 2.0f, 2.0f, 0.055f, 0.071f, 0.11f, 1.0f);
    }

    for (std::int32_t row = 0; row < Bricks::ROWS; ++row) {
        for (std::int32_t col = 0; col < Bricks::COLUMNS; ++col) {
            const std::int32_t cell = board.bricks.cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
            if (cell <= 0) {
                continue;
            }

            const float x = (static_cast<float>(col * board.tileWidth) / bw) * 2.0f - 1.0f;
            const float y = 1.0f - (static_cast<float>(row * board.tileHeight + board.bricks.amountMoved) / bh) * 2.0f;
            const float w = (static_cast<float>(std::max(1, board.tileWidth - 1)) / bw) * 2.0f;
            const float h = (static_cast<float>(std::max(1, board.tileHeight - 1)) / bh) * 2.0f;

            if (bricksTexture_.loaded()) {
                const std::int32_t frame = brickFrame(cell);
                const float frameH = static_cast<float>(bricksTexture_.height) / static_cast<float>(kBrickFrameRows);
                const auto uv0 = uv(0.0f, frame * frameH, bricksTexture_.width, bricksTexture_.height);
                const auto uv1 = uv(static_cast<float>(bricksTexture_.width), (frame + 1) * frameH, bricksTexture_.width, bricksTexture_.height);
                drawTexturedRect(x, y - h, w, h, bricksTexture_.id, uv0.first, uv0.second, uv1.first, uv1.second);
            } else {
                drawRect(x, y - h, w, h, 0.26f, 0.65f, 0.96f, 1.0f);
            }
        }
    }

    for (const Ball& ball : board.balls) {
        if (!ball.isActive()) {
            continue;
        }
        const float x = (static_cast<float>(ball.x - Ball::RADIUS) / bw) * 2.0f - 1.0f;
        const float y = 1.0f - (static_cast<float>(ball.y - Ball::RADIUS) / bh) * 2.0f;
        const float size = (static_cast<float>(Ball::RADIUS * 2) / bw) * 2.0f;

        if (ballsTexture_.loaded()) {
            const std::int32_t frameSize = ballsTexture_.height;
            const std::int32_t frameCount = std::max<std::int32_t>(1, ballsTexture_.width / std::max<std::int32_t>(1, frameSize));
            const std::int32_t frame = std::max<std::int32_t>(0, std::min<std::int32_t>(frameCount - 1, ballFrameTick_ >> 3));
            const float srcX = static_cast<float>(frame * frameSize);
            const auto uv0 = uv(srcX, 0.0f, ballsTexture_.width, ballsTexture_.height);
            const auto uv1 = uv(srcX + static_cast<float>(frameSize), static_cast<float>(ballsTexture_.height), ballsTexture_.width, ballsTexture_.height);
            drawTexturedRect(x, y - size, size, size, ballsTexture_.id, uv0.first, uv0.second, uv1.first, uv1.second);
        } else {
            drawRect(x, y - size, size, size, 0.97f, 0.97f, 0.97f, 1.0f);
        }
    }

    for (const Bullet& laser : board.lasers) {
        if (!laser.isActive()) {
            continue;
        }
        const float x = (static_cast<float>(laser.x) / bw) * 2.0f - 1.0f;
        const float y = 1.0f - (static_cast<float>(laser.y) / bh) * 2.0f;
        const float w = (static_cast<float>(std::max(1, laser.width)) / bw) * 2.0f;
        const float h = (static_cast<float>(std::max(1, laser.height)) / bh) * 2.0f;

        if (laserTexture_.loaded()) {
            drawTexturedRect(x, y - h, w, h, laserTexture_.id, 0.0f, 0.0f, 1.0f, 1.0f);
        } else {
            drawRect(x, y - h, w, h, 1.0f, 0.3f, 0.0f, 1.0f);
        }
    }

    if (board.bomb.isActive()) {
        const float x = (static_cast<float>(board.bomb.x) / bw) * 2.0f - 1.0f;
        const float y = 1.0f - (static_cast<float>(board.bomb.y) / bh) * 2.0f;
        const float w = (static_cast<float>(std::max(1, board.bomb.width)) / bw) * 2.0f;
        const float h = (static_cast<float>(std::max(1, board.bomb.height)) / bh) * 2.0f;

        if (bombTexture_.loaded()) {
            drawTexturedRect(x, y - h, w, h, bombTexture_.id, 0.0f, 0.0f, 1.0f, 1.0f);
        } else {
            drawRect(x, y - h, w, h, 0.89f, 0.29f, 0.2f, 1.0f);
        }
    }

    for (const Pill& pill : board.pills.pool()) {
        if (!pill.isActive()) {
            continue;
        }
        const float x = (static_cast<float>(pill.x) / bw) * 2.0f - 1.0f;
        const float y = 1.0f - (static_cast<float>(pill.y) / bh) * 2.0f;
        const float w = (static_cast<float>(std::max(1, pill.width)) / bw) * 2.0f;
        const float h = (static_cast<float>(std::max(1, pill.height)) / bh) * 2.0f;

        if (pillsTexture_.loaded()) {
            const std::int32_t frame = (pillFrameTick_ / 3) % 4;
            const float srcW = static_cast<float>(pillsTexture_.width) / 4.0f;
            const float srcX = frame * srcW;
            const auto uv0 = uv(srcX, 0.0f, pillsTexture_.width, pillsTexture_.height);
            const auto uv1 = uv(srcX + srcW, static_cast<float>(pillsTexture_.height), pillsTexture_.width, pillsTexture_.height);
            drawTexturedRect(x, y - h, w, h, pillsTexture_.id, uv0.first, uv0.second, uv1.first, uv1.second);
        } else {
            drawRect(x, y - h, w, h, 0.64f, 0.9f, 0.21f, 1.0f);
        }
    }

    const float paddleX = (static_cast<float>(board.paddle.x) / bw) * 2.0f - 1.0f;
    const float paddleY = 1.0f - (static_cast<float>(board.paddle.y) / bh) * 2.0f;
    const float paddleW = (static_cast<float>(std::max(1, board.paddle.width)) / bw) * 2.0f;
    const float paddleH = (static_cast<float>(std::max(1, board.paddle.height)) / bh) * 2.0f;

    if (board.paddle.mode == Paddle::MODE_LONG && paddleLongTexture_.loaded()) {
        drawTexturedRect(paddleX, paddleY - paddleH, paddleW, paddleH, paddleLongTexture_.id, 0.0f, 0.0f, 1.0f, 1.0f);
    } else if (paddlesTexture_.loaded()) {
        const std::int32_t row = board.paddle.mode == Paddle::MODE_GUN
            ? 2
            : (board.paddle.mode == Paddle::MODE_LASER ? 1 : 0);
        const float srcH = static_cast<float>(paddlesTexture_.height) / 3.0f;
        const auto uv0 = uv(0.0f, row * srcH, paddlesTexture_.width, paddlesTexture_.height);
        const auto uv1 = uv(static_cast<float>(paddlesTexture_.width), (row + 1) * srcH, paddlesTexture_.width, paddlesTexture_.height);
        drawTexturedRect(paddleX, paddleY - paddleH, paddleW, paddleH, paddlesTexture_.id, uv0.first, uv0.second, uv1.first, uv1.second);
    } else {
        drawRect(paddleX, paddleY - paddleH, paddleW, paddleH, 0.13f, 0.79f, 0.59f, 1.0f);
    }
}

void Renderer::render() {
    updateRenderArea();
    updateFrameTime();
    updateGame();

    glClearColor(0.02f, 0.03f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program_);
    drawScene();

    const EGLBoolean swapOk = eglSwapBuffers(display_, surface_);
    assert(swapOk == EGL_TRUE);
}
