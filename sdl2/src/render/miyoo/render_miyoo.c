//
//    NDS Emulator (DraStic) for Miyoo Handheld
//
//    This software is provided 'as-is', without any express or implied
//    warranty.  In no event will the authors be held liable for any damages
//    arising from the use of this software.
//
//    Permission is granted to anyone to use this software for any purpose,
//    including commercial applications, and to alter it and redistribute it
//    freely, subject to the following restrictions:
//
//    1. The origin of this software must not be misrepresented; you must not
//       claim that you wrote the original software. If you use this software
//       in a product, an acknowledgment in the product documentation would be
//       appreciated but is not required.
//    2. Altered source versions must be plainly marked as such, and must not be
//       misrepresented as being the original software.
//    3. This notice may not be removed or altered from any source distribution.
//

#include <unistd.h>
#include <stdbool.h>

#include "../../SDL_internal.h"
#include "../../video/miyoo/video_miyoo.h"
#include "../../video/miyoo/event_miyoo.h"
#include "../SDL_sysrender.h"
#include "SDL_hints.h"

#include "log.h"
#include "render_miyoo.h"

#if defined(UT)
#include "unity_fixture.h"
#endif

extern NDS nds;
extern int show_fps;

static void DestroyRenderer(SDL_Renderer *renderer);
static SDL_Renderer* CreateRenderer(SDL_Window *window, uint32_t flags);

#if defined(UT)
TEST_GROUP(sdl2_render_miyoo);

TEST_SETUP(sdl2_render_miyoo)
{
}

TEST_TEAR_DOWN(sdl2_render_miyoo)
{
}
#endif

static void WindowEvent(SDL_Renderer *renderer, const SDL_WindowEvent *event)
{
    if (!renderer || !event) {
        err(SDL"invalid parameters(0x%x, 0x%x)in %s\n", renderer, event, __func__);
    }
}

#if defined(UT)
TEST(sdl2_render_miyoo, WindowEvent)
{
    WindowEvent(NULL, NULL);
    TEST_PASS();
}
#endif

static void DestroyTexture(SDL_Renderer *renderer, SDL_Texture *texture)
{
    do {
        if (!renderer || !texture) {
            err(SDL"invalid parameters(0x%x, 0x%x)in %s\n", renderer, texture, __func__);
            break;
        }

        if (texture->driverdata) {
            Miyoo_TextureData *m = (Miyoo_TextureData *)texture->driverdata;

            if (!m) {
                break;
            }

            if (m->pixels) {
                SDL_free(m->pixels);
                m->pixels = NULL;
            }

            SDL_free(m);
            texture->driverdata = NULL;
        }
    } while (0);
}

#if defined(UT)
TEST(sdl2_render_miyoo, DestroyTexture)
{
    SDL_Texture t = {0};
    SDL_Renderer r = { 0 };
    Miyoo_TextureData *m = NULL;

    DestroyTexture(NULL, NULL);
    DestroyTexture(&r, NULL);

    m = SDL_calloc(1, sizeof(Miyoo_TextureData));
    TEST_ASSERT_NOT_NULL(m);

    m->pixels = SDL_calloc(1, 32);
    TEST_ASSERT_NOT_NULL(m->pixels);
    t.driverdata = m;

    DestroyTexture(&r, &t);
}
#endif

static int CreateTexture(SDL_Renderer *renderer, SDL_Texture *texture)
{
    if (!renderer || !texture) {
        err(SDL"invalid parameters(0x%x, 0x%x)in %s\n", renderer, texture, __func__);
        return -1;
    }

    do {
        Miyoo_TextureData *m = (Miyoo_TextureData *)SDL_calloc(1, sizeof(Miyoo_TextureData));
        if(!m) {
            err(SDL"failed to allocate miyoo texture in %s\n", __func__);
            return SDL_OutOfMemory();
        }

        m->width = texture->w;
        m->height = texture->h;
        m->format = texture->format;

        switch(m->format) {
        case SDL_PIXELFORMAT_RGB565:
            m->bits = 16;
            break;
        case SDL_PIXELFORMAT_ARGB8888:
            m->bits = 32;
            break;
        default:
            err(SDL"invalid pixel format(0x%x) in %s\n", m->format, __func__);
            return -1;
        }

        m->pitch = m->width * SDL_BYTESPERPIXEL(texture->format);
        m->size = m->height * m->pitch;
        m->pixels = SDL_calloc(1, m->size);

        if(!m->pixels) {
            err(SDL"failed to allocate texture data in %s\n", __func__);
            SDL_free(m);
            return SDL_OutOfMemory();
        }

        texture->driverdata = m;
        GFX_Clear();
    } while(0);

    return 0;
}

#if defined(UT)
TEST(sdl2_render_miyoo, CreateTexture)
{
    SDL_Window w = {0};
    SDL_Texture t = {0};
    SDL_Renderer *r = NULL;
    Miyoo_TextureData *m = NULL;
    const int width = 320;
    const int height = 240;

    TEST_ASSERT_EQUAL_INT(-1, CreateTexture(NULL, NULL));

    r = CreateRenderer(&w, 0);
    TEST_ASSERT_NOT_NULL(r);

    t.w = width;
    t.h = height;
    t.format = SDL_PIXELFORMAT_ARGB8888;
    TEST_ASSERT_EQUAL_INT(-1, CreateTexture(r, NULL));
    TEST_ASSERT_EQUAL_INT(0, CreateTexture(r, &t));
    TEST_ASSERT_NOT_NULL(t.driverdata);

    m = t.driverdata;
    TEST_ASSERT_EQUAL_INT(width, m->width);
    TEST_ASSERT_EQUAL_INT(height, m->height);
    TEST_ASSERT_EQUAL_INT(32, m->bits);

    DestroyTexture(r, &t);
    DestroyRenderer(r);
}
#endif

static int LockTexture(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *rect, void **pixels, int *pitch)
{
    if (!renderer || !texture || !rect || !pixels || !pitch) {
        err(SDL"invalid parameters(0x%x, 0x%x, 0x%x, 0x%x, 0x%x) in %s\n", renderer, texture, rect, pixels, pitch, __func__);
        return -1;
    }

    do {
        Miyoo_TextureData *m = (Miyoo_TextureData *)(texture->driverdata);

        if (m) {
            *pitch = m->pitch;
            *pixels = m->pixels;
        }
    } while (0);
    return 0;
}

#if defined(UT)
TEST(sdl2_render_miyoo, LockTexture)
{
    SDL_Rect rt = { 0 };
    SDL_Texture t = { 0 };
    SDL_Renderer r = { 0 };
    int pitch = 0;
    uint32_t **pixels = NULL;
    Miyoo_TextureData m = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, LockTexture(NULL, NULL, NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, LockTexture(&r, NULL, NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, LockTexture(&r, &t, NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, LockTexture(&r, &t, &rt, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, LockTexture(&r, &t, &rt, (void *)pixels, NULL));

    pixels = SDL_calloc(1, sizeof(uint32_t *));
    TEST_ASSERT_NOT_NULL(pixels);
    pixels[0] = NULL;

    t.driverdata = NULL;
    TEST_ASSERT_EQUAL_INT(0, LockTexture(&r, &t, &rt, (void *)pixels, &pitch));
    TEST_ASSERT_EQUAL_INT(0, pitch);
    TEST_ASSERT_EQUAL_INT(0, pixels[0]);

    t.driverdata = &m;
    m.pitch = 11;
    m.pixels = (void *)22;
    TEST_ASSERT_EQUAL_INT(0, LockTexture(&r, &t, &rt, (void *)pixels, &pitch));
    TEST_ASSERT_EQUAL_INT(11, pitch);
    TEST_ASSERT_EQUAL_INT(22, pixels[0]);

    SDL_free(pixels);
}
#endif

static int UpdateTexture(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch)
{
    if (!renderer || !texture || !rect || !pixels) {
        err(SDL"invalid parameters(0x%x, 0x%x, 0x%x, 0x%x) in %s\n", renderer, texture, rect, pixels, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_render_miyoo, UpdateTexture)
{
    SDL_Rect rt = { 0 };
    SDL_Texture t = { 0 };
    SDL_Renderer r = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, UpdateTexture(NULL, NULL, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, UpdateTexture(&r, NULL, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, UpdateTexture(&r, &t, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, UpdateTexture(&r, &t, &rt, NULL, 0));

    TEST_ASSERT_EQUAL_INT(0, UpdateTexture(&r, &t, &rt, (void *)0xdeadbeef, 0));
}
#endif

static void UnlockTexture(SDL_Renderer *renderer, SDL_Texture *texture)
{
    do {
        if (!renderer || !texture) {
            err(SDL"invalid parameters(0x%x, 0x%x) in %s\n", renderer, texture, __func__);
            break;
        }

        if (texture->driverdata) {
            SDL_Rect rect = { 0 };
            Miyoo_TextureData *m = (Miyoo_TextureData * )texture->driverdata;

            rect.x = 0;
            rect.y = 0;
            rect.w = texture->w;
            rect.h = texture->h;
            UpdateTexture(renderer, texture, &rect, m->pixels, m->pitch);
        }
    } while (0);
}

#if defined(UT)
TEST(sdl2_render_miyoo, UnlockTexture)
{
    SDL_Rect rt = { 0 };
    SDL_Texture t = { 0 };
    SDL_Renderer r = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, UpdateTexture(NULL, NULL, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, UpdateTexture(&r, NULL, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, UpdateTexture(&r, &t, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, UpdateTexture(&r, &t, &rt, NULL, 0));

    TEST_ASSERT_EQUAL_INT(0, UpdateTexture(&r, &t, &rt, (void *)0xdeadbeef, 0));
}
#endif

static void SetTextureScaleMode(SDL_Renderer *renderer, SDL_Texture *texture, SDL_ScaleMode scaleMode)
{
    if (!renderer || !texture) {
        err(SDL"invalid parameters(0x%x, 0x%x) in %s\n", renderer, texture, __func__);
    }
}

#if defined(UT)
TEST(sdl2_render_miyoo, SetTextureScaleMode)
{
    SetTextureScaleMode(NULL, NULL, 0);
    TEST_PASS();
}
#endif

static int SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture)
{
    if (!renderer || !texture) {
        err(SDL"invalid parameters(0x%x, 0x%x) in %s\n", renderer, texture, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_render_miyoo, SetRenderTarget)
{
    SDL_Texture t = { 0 };
    SDL_Renderer r = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, SetRenderTarget(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, SetRenderTarget(&r, NULL));
    TEST_ASSERT_EQUAL_INT(0, SetRenderTarget(&r, &t));
}
#endif

static int QueueSetViewport(SDL_Renderer *renderer, SDL_RenderCommand *cmd)
{
    if (!renderer || !cmd) {
        err(SDL"invalid parameters(0x%x, 0x%x) in %s\n", renderer, cmd, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_render_miyoo, QueueSetViewport)
{
    SDL_Renderer r = { 0 };
    SDL_RenderCommand c = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, QueueSetViewport(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, QueueSetViewport(&r, NULL));
    TEST_ASSERT_EQUAL_INT(0, QueueSetViewport(&r, &c));
}
#endif

static int QueueDrawPoints(SDL_Renderer *renderer, SDL_RenderCommand *cmd, const SDL_FPoint *points, int count)
{
    if (!renderer || !cmd || !points) {
        err(SDL"invalid parameters(0x%x, 0x%x) in %s\n", renderer, cmd, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_render_miyoo, QueueDrawPoints)
{
    SDL_FPoint f = { 0 };
    SDL_Renderer r = { 0 };
    SDL_RenderCommand c = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, QueueDrawPoints(NULL, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueDrawPoints(&r, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueDrawPoints(&r, &c, NULL, 0));
    TEST_ASSERT_EQUAL_INT(0, QueueDrawPoints(&r, &c, &f, 0));
}
#endif

static int QueueGeometry(
    SDL_Renderer *renderer,
    SDL_RenderCommand *cmd,
    SDL_Texture *texture,
    const float *xy,
    int xy_stride,
    const SDL_Color *color,
    int color_stride,
    const float *uv,
    int uv_stride,
    int num_vertices,
    const void *indices,
    int num_indices,
    int size_indices,
    float scale_x,
    float scale_y)
{
    if (!renderer || !cmd || !texture || !xy || !color || !uv || !indices) {
        err(SDL"invalid parameter(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) in %s\n", 
            renderer, cmd, texture, xy, color, uv, indices, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_render_miyoo, QueueGeometry)
{
    float xy = 0;
    SDL_Color c = { 0 };
    SDL_Texture t = { 0 };
    SDL_Renderer r = { 0 };
    SDL_RenderCommand cmd = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, QueueGeometry(NULL, NULL, NULL, NULL, 0, NULL, 0, NULL, 0, 0, NULL, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueGeometry(&r, NULL, NULL, NULL, 0, NULL, 0, NULL, 0, 0, NULL, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueGeometry(&r, &cmd, NULL, NULL, 0, NULL, 0, NULL, 0, 0, NULL, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueGeometry(&r, &cmd, &t, NULL, 0, NULL, 0, NULL, 0, 0, NULL, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueGeometry(&r, &cmd, &t, &xy, 0, NULL, 0, NULL, 0, 0, NULL, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueGeometry(&r, &cmd, &t, &xy, 0, &c, 0, NULL, 0, 0, NULL, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueGeometry(&r, &cmd, &t, &xy, 0, &c, 0, &xy, 0, 0, NULL, 0, 0, 0, 0));

    TEST_ASSERT_EQUAL_INT(0, QueueGeometry(&r, &cmd, &t, &xy, 0, &c, 0, &xy, 0, 0, (const void *)0xdeadbeef, 0, 0, 0, 0));
}
#endif

static int QueueFillRects(SDL_Renderer *renderer, SDL_RenderCommand *cmd, const SDL_FRect *rects, int count)
{
    if (!renderer || !cmd || !rects) {
        err(SDL"invalid parameter(0x%x, 0x%x, 0x%x, 0x%x) in %s\n", renderer, cmd, rects, count, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_render_miyoo, QueueFillRects)
{
    SDL_FRect rt = { 0 };
    SDL_Renderer r = { 0 };
    SDL_RenderCommand cmd = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, QueueFillRects(NULL, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueFillRects(&r, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueFillRects(&r, &cmd, NULL, 0));

    TEST_ASSERT_EQUAL_INT(0, QueueFillRects(&r, &cmd, &rt, 0));
}
#endif

static int QueueCopy(SDL_Renderer *renderer, SDL_RenderCommand *cmd, SDL_Texture *texture, const SDL_Rect *src_rect, const SDL_FRect *dst_rect)
{
    if (!renderer || !cmd || !texture || !src_rect || !dst_rect) {
        err(SDL"invalid parameter(0x%x, 0x%x, 0x%x, 0x%x, 0x%x) in %s\n", renderer, cmd, texture, src_rect, dst_rect, __func__);
        return -1;
    }

    show_fps = 0;
    nds.menu.drastic.enable = 1;
    usleep(100000);

    process_drastic_menu();
    return 0;
}

#if defined(UT)
TEST(sdl2_render_miyoo, QueueCopy)
{
    SDL_Rect r1 = { 0 };
    SDL_FRect r2 = {0};
    SDL_Texture t = { 0 };
    SDL_Renderer r = { 0 };
    SDL_RenderCommand cmd = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, QueueCopy(NULL, NULL, NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, QueueCopy(&r, NULL, NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, QueueCopy(&r, &cmd, NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, QueueCopy(&r, &cmd, &t, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, QueueCopy(&r, &cmd, &t, &r1, NULL));

    TEST_ASSERT_EQUAL_INT(0, QueueCopy(&r, &cmd, &t, &r1, &r2));
}
#endif

static int QueueCopyEx(
    SDL_Renderer *renderer,
    SDL_RenderCommand *cmd,
    SDL_Texture *texture,
    const SDL_Rect *src_rect,
    const SDL_FRect *dst_rect,
    const double angle,
    const SDL_FPoint *center,
    const SDL_RendererFlip flip)
{
    if (!renderer || !cmd || !texture || !src_rect || !dst_rect || !center) {
        err(SDL"invalid parameter(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) in %s\n", renderer, cmd, texture, src_rect, dst_rect, center, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_render_miyoo, QueueCopyEx)
{
    SDL_Rect r1 = { 0 };
    SDL_FPoint c = { 0 };
    SDL_FRect r2 = { 0 };
    SDL_Texture t = { 0 };
    SDL_Renderer r = { 0 };
    SDL_RenderCommand cmd = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, QueueCopyEx(NULL, NULL, NULL, NULL, NULL, 0, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueCopyEx(&r, NULL, NULL, NULL, NULL, 0, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueCopyEx(&r, &cmd, NULL, NULL, NULL, 0, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueCopyEx(&r, &cmd, &t, NULL, NULL, 0, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueCopyEx(&r, &cmd, &t, &r1, NULL, 0, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, QueueCopyEx(&r, &cmd, &t, &r1, &r2, 0, NULL, 0));

    TEST_ASSERT_EQUAL_INT(0, QueueCopyEx(&r, &cmd, &t, &r1, &r2, 0, &c, 0));
}
#endif

static int RunCommandQueue(SDL_Renderer *renderer, SDL_RenderCommand *cmd, void *vertices, size_t vertsize)
{
    if (!renderer || !cmd || !vertices) {
        err(SDL"invalid parameter(0x%x, 0x%x, 0x%x) in %s\n", renderer, cmd, vertices, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_render_miyoo, RunCommandQueue)
{
    SDL_Renderer r = { 0 };
    SDL_RenderCommand cmd = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, RunCommandQueue(NULL, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, RunCommandQueue(&r, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, RunCommandQueue(&r, &cmd, NULL, 0));

    TEST_ASSERT_EQUAL_INT(0, RunCommandQueue(&r, &cmd, (void *)0xdeadbeef, 0));
}
#endif

static int RenderReadPixels(SDL_Renderer *renderer, const SDL_Rect *rect, uint32_t pixel_format, void *pixels, int pitch)
{
    if (!renderer || !rect || !pixels) {
        err(SDL"invalid parameter(0x%x, 0x%x, 0x%x) in %s\n", renderer, rect, pixels, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_render_miyoo, RenderReadPixels)
{
    SDL_Rect rt = { 0 };
    SDL_Renderer r = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, RenderReadPixels(NULL, NULL, 0, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, RenderReadPixels(&r, NULL, 0, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, RenderReadPixels(&r, &rt, 0, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, RenderReadPixels(&r, &rt, 0, NULL, 0));

    TEST_ASSERT_EQUAL_INT(0, RenderReadPixels(&r, &rt, 0, (void *)0xdeadbeef, 0));
}
#endif

static void RenderPresent(SDL_Renderer *renderer)
{
    do {
        if (!renderer) {
            err(SDL"invalid parameter(0x%x) in %s\n", renderer, __func__);
        }
    } while( 0 );
}

#if defined(UT)
TEST(sdl2_render_miyoo, RenderPresent)
{
    RenderPresent(NULL);
    TEST_PASS();
}
#endif

static void DestroyRenderer(SDL_Renderer *renderer)
{
    do {
        if (!renderer) {
            err(SDL"invalid parameter(0x%x) in %s\n", renderer, __func__);
        }

        SDL_free(renderer);
    } while( 0 );
}

#if defined(UT)
TEST(sdl2_render_miyoo, DestroyRenderer)
{
    SDL_Window w = {0};
    SDL_Renderer *r = NULL;

    TEST_ASSERT_NULL(CreateRenderer(NULL, 0));

    r = CreateRenderer(&w, SDL_RENDERER_PRESENTVSYNC);
    DestroyRenderer(NULL);
    DestroyRenderer(r);
}
#endif

static int SetVSync(SDL_Renderer *renderer, const int vsync)
{
    if (!renderer || (vsync < 0)) {
        err(SDL"invalid parameters(0x%x, 0x%x) in %s\n", renderer, vsync, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_render_miyoo, SetVSync)
{
    SDL_Renderer t = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, SetVSync(NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, SetVSync(&t, -1));
    TEST_ASSERT_EQUAL_INT(0, SetVSync(&t, 0));
}
#endif

static SDL_Renderer* CreateRenderer(SDL_Window *window, uint32_t flags)
{
    SDL_Renderer *renderer = NULL;

    if (!window) {
        err(SDL"invalid parameter(0x%x) in %s\n", window, __func__);
        return NULL;
    }

    renderer = (SDL_Renderer *)SDL_calloc(1, sizeof(SDL_Renderer));
    if(!renderer) {
        err(SDL"failed to create render in %s\n", __func__);
        SDL_OutOfMemory();
        return NULL;
    }
    memset(renderer, 0, sizeof(SDL_Renderer));

    renderer->WindowEvent = WindowEvent;
    renderer->CreateTexture = CreateTexture;
    renderer->UpdateTexture = UpdateTexture;
    renderer->LockTexture = LockTexture;
    renderer->UnlockTexture = UnlockTexture;
    renderer->SetTextureScaleMode = SetTextureScaleMode;
    renderer->SetRenderTarget = SetRenderTarget;
    renderer->QueueSetViewport = QueueSetViewport;
    renderer->QueueSetDrawColor = QueueSetViewport;
    renderer->QueueDrawPoints = QueueDrawPoints;
    renderer->QueueDrawLines = QueueDrawPoints;
    renderer->QueueGeometry = QueueGeometry;
    renderer->QueueFillRects = QueueFillRects;
    renderer->QueueCopy = QueueCopy;
    renderer->QueueCopyEx = QueueCopyEx;
    renderer->RunCommandQueue = RunCommandQueue;
    renderer->RenderReadPixels = RenderReadPixels;
    renderer->RenderPresent = RenderPresent;
    renderer->DestroyTexture = DestroyTexture;
    renderer->DestroyRenderer = DestroyRenderer;
    renderer->SetVSync = SetVSync;
    renderer->info = Miyoo_RenderDriver.info;
    renderer->info.flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
    renderer->window = window;

    return renderer;
}

#if defined(UT)
TEST(sdl2_render_miyoo, CreateRenderer)
{
    SDL_Window w = {0};
    SDL_Renderer *r = NULL;

    TEST_ASSERT_NULL(CreateRenderer(NULL, 0));

    r = CreateRenderer(&w, SDL_RENDERER_PRESENTVSYNC);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL(&w, r->window);
    TEST_ASSERT_NOT_NULL(r->CreateTexture);
    TEST_ASSERT_NOT_NULL(r->LockTexture);
    TEST_ASSERT_NOT_NULL(r->QueueCopy);
    TEST_ASSERT_NULL(r->driverdata);
    DestroyRenderer(r);
}
#endif

SDL_RenderDriver Miyoo_RenderDriver = {
    .CreateRenderer = CreateRenderer,
    .info = {
        .name = "Miyoo",
        .flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE,
        .num_texture_formats = 2,
        .texture_formats = {
            [0] = SDL_PIXELFORMAT_RGB565,
            [1] = SDL_PIXELFORMAT_ARGB8888,
        },
        .max_texture_width = 800,
        .max_texture_height = 600,
    }
};

#if defined(UT)
TEST_GROUP_RUNNER(sdl2_render_miyoo)
{
    RUN_TEST_CASE(sdl2_render_miyoo, WindowEvent);
    RUN_TEST_CASE(sdl2_render_miyoo, DestroyTexture);
    RUN_TEST_CASE(sdl2_render_miyoo, CreateTexture);
    RUN_TEST_CASE(sdl2_render_miyoo, LockTexture);
    RUN_TEST_CASE(sdl2_render_miyoo, UpdateTexture);
    RUN_TEST_CASE(sdl2_render_miyoo, UnlockTexture);
    RUN_TEST_CASE(sdl2_render_miyoo, SetTextureScaleMode);
    RUN_TEST_CASE(sdl2_render_miyoo, SetRenderTarget);
    RUN_TEST_CASE(sdl2_render_miyoo, QueueSetViewport);
    RUN_TEST_CASE(sdl2_render_miyoo, QueueDrawPoints);
    RUN_TEST_CASE(sdl2_render_miyoo, QueueGeometry);
    RUN_TEST_CASE(sdl2_render_miyoo, QueueFillRects);
    RUN_TEST_CASE(sdl2_render_miyoo, QueueCopy);
    RUN_TEST_CASE(sdl2_render_miyoo, QueueCopyEx);
    RUN_TEST_CASE(sdl2_render_miyoo, RunCommandQueue);
    RUN_TEST_CASE(sdl2_render_miyoo, RenderReadPixels);
    RUN_TEST_CASE(sdl2_render_miyoo, RenderPresent);
    RUN_TEST_CASE(sdl2_render_miyoo, DestroyRenderer);
    RUN_TEST_CASE(sdl2_render_miyoo, SetVSync);
    RUN_TEST_CASE(sdl2_render_miyoo, CreateRenderer);
}
#endif

