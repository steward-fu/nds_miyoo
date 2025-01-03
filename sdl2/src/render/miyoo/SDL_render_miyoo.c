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
#include "../../video/miyoo/SDL_video_miyoo.h"
#include "../../video/miyoo/SDL_event_miyoo.h"
#include "../SDL_sysrender.h"
#include "SDL_hints.h"

typedef struct Miyoo_TextureData {
    void *data;
    unsigned int size;
    unsigned int width;
    unsigned int height;
    unsigned int bits;
    unsigned int format;
    unsigned int pitch;
} Miyoo_TextureData;

typedef struct {
    SDL_Texture *boundTarget;
    SDL_bool initialized;
    unsigned int bpp;
    SDL_bool vsync;
} Miyoo_RenderData;

extern NDS nds;
extern int show_fps;

static void Miyoo_WindowEvent(SDL_Renderer *renderer, const SDL_WindowEvent *event)
{
}

static int Miyoo_CreateTexture(SDL_Renderer *renderer, SDL_Texture *texture)
{
    Miyoo_TextureData *mmiyoo_texture = (Miyoo_TextureData *)SDL_calloc(1, sizeof(*mmiyoo_texture));

    if(!mmiyoo_texture) {
        printf(PREFIX"Failed to create texture\n");
        return SDL_OutOfMemory();
    }

    mmiyoo_texture->width = texture->w;
    mmiyoo_texture->height = texture->h;
    mmiyoo_texture->format = texture->format;

    switch(mmiyoo_texture->format) {
    case SDL_PIXELFORMAT_RGB565:
        mmiyoo_texture->bits = 16;
        break;
    case SDL_PIXELFORMAT_ARGB8888:
        mmiyoo_texture->bits = 32;
        break;
    default:
        return -1;
    }

    mmiyoo_texture->pitch = mmiyoo_texture->width * SDL_BYTESPERPIXEL(texture->format);
    mmiyoo_texture->size = mmiyoo_texture->height * mmiyoo_texture->pitch;
    mmiyoo_texture->data = SDL_calloc(1, mmiyoo_texture->size);

    if(!mmiyoo_texture->data) {
        printf(PREFIX"Failed to create texture data\n");
        SDL_free(mmiyoo_texture);
        return SDL_OutOfMemory();
    }

    mmiyoo_texture->data = SDL_calloc(1, mmiyoo_texture->size);
    texture->driverdata = mmiyoo_texture;
    GFX_Clear();
    return 0;
}

static int Miyoo_LockTexture(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *rect, void **pixels, int *pitch)
{
    Miyoo_TextureData *mmiyoo_texture = (Miyoo_TextureData*)texture->driverdata;

    *pixels = mmiyoo_texture->data;
    *pitch = mmiyoo_texture->pitch;
    return 0;
}

static int Miyoo_UpdateTexture(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch)
{
    return 0;
}

static void Miyoo_UnlockTexture(SDL_Renderer *renderer, SDL_Texture *texture)
{
    SDL_Rect rect = {0};
    Miyoo_TextureData *mmiyoo_texture = (Miyoo_TextureData*)texture->driverdata;

    rect.x = 0;
    rect.y = 0;
    rect.w = texture->w;
    rect.h = texture->h;
    Miyoo_UpdateTexture(renderer, texture, &rect, mmiyoo_texture->data, mmiyoo_texture->pitch);
}

static void Miyoo_SetTextureScaleMode(SDL_Renderer *renderer, SDL_Texture *texture, SDL_ScaleMode scaleMode)
{
}

static int Miyoo_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture)
{
    return 0;
}

static int Miyoo_QueueSetViewport(SDL_Renderer *renderer, SDL_RenderCommand *cmd)
{
    return 0;
}

static int Miyoo_QueueDrawPoints(SDL_Renderer *renderer, SDL_RenderCommand *cmd, const SDL_FPoint *points, int count)
{
    return 0;
}

static int Miyoo_QueueGeometry(SDL_Renderer *renderer,
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
    return 0;
}

static int Miyoo_QueueFillRects(SDL_Renderer *renderer, SDL_RenderCommand *cmd, const SDL_FRect *rects, int count)
{
    return 0;
}

static int Miyoo_QueueCopy(SDL_Renderer *renderer, SDL_RenderCommand *cmd, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_FRect *dstrect)
{
    show_fps = 0;
    nds.menu.drastic.enable = 1;
    usleep(100000);

    process_drastic_menu();
    return 0;
}

static int Miyoo_QueueCopyEx(SDL_Renderer *renderer, SDL_RenderCommand *cmd, SDL_Texture *texture,
    const SDL_Rect *srcrect, const SDL_FRect *dstrect, const double angle, const SDL_FPoint *center, const SDL_RendererFlip flip)
{
    return 0;
}

static int Miyoo_RunCommandQueue(SDL_Renderer *renderer, SDL_RenderCommand *cmd, void *vertices, size_t vertsize)
{
    return 0;
}

static int Miyoo_RenderReadPixels(SDL_Renderer *renderer, const SDL_Rect *rect, Uint32 pixel_format, void *pixels, int pitch)
{
    return SDL_Unsupported();
}

static void Miyoo_RenderPresent(SDL_Renderer *renderer)
{
}

static void Miyoo_DestroyTexture(SDL_Renderer *renderer, SDL_Texture *texture)
{
    Miyoo_TextureData *mmiyoo_texture = (Miyoo_TextureData*)texture->driverdata;

    if (mmiyoo_texture) {
        if (mmiyoo_texture->data) {
            SDL_free(mmiyoo_texture->data);
        }
        SDL_free(mmiyoo_texture);
        texture->driverdata = NULL;
    }
}

static void Miyoo_DestroyRenderer(SDL_Renderer *renderer)
{
    Miyoo_RenderData *data = (Miyoo_RenderData *)renderer->driverdata;

    if(data) {
        if(!data->initialized) {
            return;
        }

        data->initialized = SDL_FALSE;
        SDL_free(data);
    }
    SDL_free(renderer);
}

static int Miyoo_SetVSync(SDL_Renderer *renderer, const int vsync)
{
    return 0;
}

SDL_Renderer *Miyoo_CreateRenderer(SDL_Window *window, Uint32 flags)
{
    int pixelformat = 0;
    SDL_Renderer *renderer = NULL;
    Miyoo_RenderData *data = NULL;

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if(!renderer) {
        printf(PREFIX"Failed to create render\n");
        SDL_OutOfMemory();
        return NULL;
    }

    data = (Miyoo_RenderData *) SDL_calloc(1, sizeof(*data));
    if(!data) {
        printf(PREFIX"Failed to create render data\n");
        Miyoo_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }

    renderer->WindowEvent = Miyoo_WindowEvent;
    renderer->CreateTexture = Miyoo_CreateTexture;
    renderer->UpdateTexture = Miyoo_UpdateTexture;
    renderer->LockTexture = Miyoo_LockTexture;
    renderer->UnlockTexture = Miyoo_UnlockTexture;
    renderer->SetTextureScaleMode = Miyoo_SetTextureScaleMode;
    renderer->SetRenderTarget = Miyoo_SetRenderTarget;
    renderer->QueueSetViewport = Miyoo_QueueSetViewport;
    renderer->QueueSetDrawColor = Miyoo_QueueSetViewport;
    renderer->QueueDrawPoints = Miyoo_QueueDrawPoints;
    renderer->QueueDrawLines = Miyoo_QueueDrawPoints;
    renderer->QueueGeometry = Miyoo_QueueGeometry;
    renderer->QueueFillRects = Miyoo_QueueFillRects;
    renderer->QueueCopy = Miyoo_QueueCopy;
    renderer->QueueCopyEx = Miyoo_QueueCopyEx;
    renderer->RunCommandQueue = Miyoo_RunCommandQueue;
    renderer->RenderReadPixels = Miyoo_RenderReadPixels;
    renderer->RenderPresent = Miyoo_RenderPresent;
    renderer->DestroyTexture = Miyoo_DestroyTexture;
    renderer->DestroyRenderer = Miyoo_DestroyRenderer;
    renderer->SetVSync = Miyoo_SetVSync;
    renderer->info = Miyoo_RenderDriver.info;
    renderer->info.flags = (SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    renderer->driverdata = data;
    renderer->window = window;

    if(data->initialized != SDL_FALSE) {
        return 0;
    }
    data->initialized = SDL_TRUE;

    if(flags & SDL_RENDERER_PRESENTVSYNC) {
        data->vsync = SDL_TRUE;
    }
    else {
        data->vsync = SDL_FALSE;
    }

    pixelformat = SDL_GetWindowPixelFormat(window);
    switch(pixelformat) {
    case SDL_PIXELFORMAT_RGB565:
        data->bpp = 2;
        break;
    case SDL_PIXELFORMAT_ARGB8888:
        data->bpp = 4;
        break;
    }
    return renderer;
}

SDL_RenderDriver Miyoo_RenderDriver = {
    .CreateRenderer = Miyoo_CreateRenderer,
    .info = {
        .name = "MMiyoo",
        .flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE,
        .num_texture_formats = 2,
        .texture_formats = {
            [0] = SDL_PIXELFORMAT_RGB565, [2] = SDL_PIXELFORMAT_ARGB8888,
        },
        .max_texture_width = 800,
        .max_texture_height = 600,
    }
};

#ifdef UT
#include "unity_fixture.h"

TEST_GROUP(sdl2_render_mmiyoo);

TEST_SETUP(sdl2_render_mmiyoo)
{
}

TEST_TEAR_DOWN(sdl2_render_mmiyoo)
{
}

TEST(sdl2_render_mmiyoo, Miyoo_SetVSync)
{
    TEST_ASSERT_EQUAL(Miyoo_SetVSync(NULL, 0), 0);
}

TEST_GROUP_RUNNER(sdl2_render_mmiyoo)
{
    RUN_TEST_CASE(sdl2_render_mmiyoo, Miyoo_SetVSync);
}
#endif

