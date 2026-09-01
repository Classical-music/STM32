#include "lv_port_indev.h"

#include "lv_port_disp.h"
#include <SDL.h>

static lv_indev_t *indev_pointer;
static lv_coord_t pointer_x;
static lv_coord_t pointer_y;
static bool pointer_pressed;
static bool quit_requested;

static void pointer_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  (void)indev_drv;
  data->point.x = pointer_x;
  data->point.y = pointer_y;
  data->state = pointer_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

static void update_pointer_position(uint32_t window_id, int32_t x, int32_t y) {
  SDL_Window *window = SDL_GetWindowFromID(window_id);
  int window_width = LV_PORT_DISP_HOR_RES;
  int window_height = LV_PORT_DISP_VER_RES;

  if (window != NULL)
    SDL_GetWindowSize(window, &window_width, &window_height);

  if (window_width > 0)
    x = x * LV_PORT_DISP_HOR_RES / window_width;
  if (window_height > 0)
    y = y * LV_PORT_DISP_VER_RES / window_height;

  if (x < 0)
    x = 0;
  else if (x >= LV_PORT_DISP_HOR_RES)
    x = LV_PORT_DISP_HOR_RES - 1;

  if (y < 0)
    y = 0;
  else if (y >= LV_PORT_DISP_VER_RES)
    y = LV_PORT_DISP_VER_RES - 1;

  pointer_x = (lv_coord_t)x;
  pointer_y = (lv_coord_t)y;
}

void lv_port_indev_init(void) {
  static lv_indev_drv_t indev_drv;

  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = pointer_read;
  indev_pointer = lv_indev_drv_register(&indev_drv);
  (void)indev_pointer;
}

void lv_port_indev_process_events(void) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      quit_requested = true;
      break;

    case SDL_MOUSEMOTION:
      update_pointer_position(event.motion.windowID, event.motion.x,
                              event.motion.y);
      break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      if (event.button.button == SDL_BUTTON_LEFT) {
        update_pointer_position(event.button.windowID, event.button.x,
                                event.button.y);
        pointer_pressed = event.type == SDL_MOUSEBUTTONDOWN;
      }
      break;

    case SDL_FINGERDOWN:
    case SDL_FINGERMOTION:
    case SDL_FINGERUP:
      pointer_x = (lv_coord_t)(event.tfinger.x * LV_PORT_DISP_HOR_RES);
      pointer_y = (lv_coord_t)(event.tfinger.y * LV_PORT_DISP_VER_RES);
      pointer_pressed = event.type != SDL_FINGERUP;
      break;

    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_ESCAPE)
        quit_requested = true;
      break;

    default:
      break;
    }
  }
}

bool lv_port_indev_should_quit(void) { return quit_requested; }
