#include "lv_port_disp.h"

#include <SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static lv_color_t *framebuffer;
static lv_color_t *draw_buffer;
static bool flush_enabled = true;

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                       lv_color_t *color_p) {
  int32_t width;
  int32_t height;
  int32_t row;

  if (!flush_enabled) {
    lv_disp_flush_ready(disp_drv);
    return;
  }

  width = area->x2 - area->x1 + 1;
  height = area->y2 - area->y1 + 1;

  for (row = 0; row < height; ++row) {
    lv_color_t *dst = framebuffer +
                      (area->y1 + row) * LV_PORT_DISP_HOR_RES + area->x1;
    const lv_color_t *src = color_p + row * width;
    memcpy(dst, src, (size_t)width * sizeof(lv_color_t));
  }

  if (lv_disp_flush_is_last(disp_drv)) {
    SDL_UpdateTexture(texture, NULL, framebuffer,
                      LV_PORT_DISP_HOR_RES * (int)sizeof(lv_color_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  lv_disp_flush_ready(disp_drv);
}

void lv_port_disp_init(void) {
  static lv_disp_draw_buf_t draw_buf;
  static lv_disp_drv_t disp_drv;
  const uint32_t draw_buffer_pixels = LV_PORT_DISP_HOR_RES * 40U;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
    abort();

  window = SDL_CreateWindow("LVGL UI Simulator", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, LV_PORT_DISP_HOR_RES,
                            LV_PORT_DISP_VER_RES,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (window == NULL)
    abort();

  renderer = SDL_CreateRenderer(window, -1,
                                SDL_RENDERER_ACCELERATED |
                                    SDL_RENDERER_PRESENTVSYNC);
  if (renderer == NULL)
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  if (renderer == NULL)
    abort();

  SDL_RenderSetLogicalSize(renderer, LV_PORT_DISP_HOR_RES,
                          LV_PORT_DISP_VER_RES);

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
                              SDL_TEXTUREACCESS_STREAMING,
                              LV_PORT_DISP_HOR_RES, LV_PORT_DISP_VER_RES);
  if (texture == NULL)
    abort();

  framebuffer = calloc((size_t)LV_PORT_DISP_HOR_RES * LV_PORT_DISP_VER_RES,
                       sizeof(lv_color_t));
  draw_buffer = malloc((size_t)draw_buffer_pixels * sizeof(lv_color_t));
  if (framebuffer == NULL || draw_buffer == NULL)
    abort();

  lv_disp_draw_buf_init(&draw_buf, draw_buffer, NULL, draw_buffer_pixels);
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LV_PORT_DISP_HOR_RES;
  disp_drv.ver_res = LV_PORT_DISP_VER_RES;
  disp_drv.flush_cb = disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
}

void lv_port_disp_deinit(void) {
  free(draw_buffer);
  draw_buffer = NULL;
  free(framebuffer);
  framebuffer = NULL;

  if (texture != NULL)
    SDL_DestroyTexture(texture);
  if (renderer != NULL)
    SDL_DestroyRenderer(renderer);
  if (window != NULL)
    SDL_DestroyWindow(window);

  texture = NULL;
  renderer = NULL;
  window = NULL;
  SDL_Quit();
}

void disp_enable_update(void) { flush_enabled = true; }

void disp_disable_update(void) { flush_enabled = false; }
