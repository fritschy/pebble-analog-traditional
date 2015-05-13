// Copyright (c) 2015 Marcus Fritzsch
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <pebble.h>

#if 1
#undef APP_LOG
#define APP_LOG(...)
#define START_TIME_MEASURE()
#define END_TIME_MEASURE()
#else
static unsigned int get_time(void)
{
   time_t s;
   uint16_t ms;
   time_ms(&s, &ms);
   return (s & 0xfffff) * 1000 + ms;
}

#define START_TIME_MEASURE() unsigned tm_0 = get_time()
#define END_TIME_MEASURE()                                                  \
   do                                                                       \
   {                                                                        \
      unsigned tm_1 = get_time();                                           \
      APP_LOG(APP_LOG_LEVEL_DEBUG, "%s: took %dms", __func__, tm_1 - tm_0); \
   } while (0)
#endif

static Window *window;
static Layer *effect_layer;
static GRect bounds;
static GPoint center;
static GFont font;
static char day[3];

typedef struct {
   int32_t width;
   int32_t main_len;
   int32_t tail_len;
   int32_t angle;
   GPoint center;
   GColor color;
} HandInfo;

static void draw_simple_hand(GContext *ctx, HandInfo hi) {
   int32_t sin = sin_lookup(hi.angle);
   int32_t cos = cos_lookup(hi.angle);

   GPoint p0 = {
       sin * hi.main_len / TRIG_MAX_RATIO + center.x,
      -cos * hi.main_len / TRIG_MAX_RATIO + center.y,
   };

   GPoint p1 = {
       sin * -hi.tail_len / TRIG_MAX_RATIO + center.x,
      -cos * -hi.tail_len / TRIG_MAX_RATIO + center.y,
   };

   graphics_draw_line(ctx, p0, p1);
}

static void update_effect_layer(Layer *l, GContext *ctx) {
   START_TIME_MEASURE();

   time_t t = time(NULL);
   struct tm *tm = localtime(&t);
   int32_t sec = tm->tm_sec;
   int32_t min = tm->tm_min;
   int32_t hr = tm->tm_hour;

   const int radius = 144 / 2 - 5;
   const int minlen = radius - 12;

   // background
   graphics_context_set_fill_color(ctx, GColorBlack);
   graphics_fill_rect(ctx, bounds, 0, 0);

   // clock face
   graphics_context_set_fill_color(ctx, GColorDarkGray);
   graphics_fill_circle(ctx, center, radius);

   // small second/minute ticks
   graphics_context_set_stroke_color(ctx, GColorLightGray);
   graphics_context_set_stroke_width(ctx, 1);
   for (int i = 0; i < 60; i++) {
      int angle = TRIG_MAX_ANGLE * i / 60;
      int32_t sin = sin_lookup(angle);
      int32_t cos = cos_lookup(angle);

      GPoint p1 = {
         sin * radius / TRIG_MAX_RATIO + center.x,
         -cos * radius / TRIG_MAX_RATIO + center.y,
      };

      GPoint p0 = {
         sin * (radius - 4) / TRIG_MAX_RATIO + center.x,
         -cos * (radius - 4) / TRIG_MAX_RATIO + center.y,
      };

      graphics_draw_line(ctx, p0, p1);
   }
   //graphics_fill_circle(ctx, center, radius - 6);

   // face rim
   graphics_context_set_stroke_color(ctx, GColorWhite);
   graphics_context_set_stroke_width(ctx, 5);
   graphics_draw_circle(ctx, center, radius+2);

   // hour ticks
   graphics_context_set_stroke_width(ctx, 3);
   for (int i = 0; i < 12; i++) {
      int angle = TRIG_MAX_ANGLE * i / 12;
      int32_t sin = sin_lookup(angle);
      int32_t cos = cos_lookup(angle);

      GPoint p1 = {
         sin * radius / TRIG_MAX_RATIO + center.x,
         -cos * radius / TRIG_MAX_RATIO + center.y,
      };

      GPoint p0 = {
         sin * minlen / TRIG_MAX_RATIO + center.x,
         -cos * minlen / TRIG_MAX_RATIO + center.y,
      };

      graphics_draw_line(ctx, p0, p1);
   }
   graphics_fill_circle(ctx, center, radius - 9);

   const int dayx = 102;
   const int dayy = 113;

   // circle for day of month
   graphics_context_set_fill_color(ctx, GColorBlack);
   graphics_fill_circle(ctx, GPoint(dayx, dayy), 10);
   graphics_context_set_stroke_width(ctx, 1);
   graphics_context_set_stroke_color(ctx, GColorLightGray);
   graphics_draw_circle(ctx, GPoint(dayx, dayy), 10);

   // day of month
   graphics_context_set_text_color(ctx, GColorLightGray);
   graphics_draw_text(ctx, day, font, GRect(dayx-10, dayy-8,20,20), GTextOverflowModeFill, GTextAlignmentCenter, NULL);

   // outline of minute and hour hand
   graphics_context_set_stroke_color(ctx, GColorBlack);
   graphics_context_set_stroke_width(ctx, 5);
   draw_simple_hand(ctx,
         (HandInfo){
            .main_len = radius - 40,
            .tail_len = 0,
            .angle = TRIG_MAX_ANGLE * (hr * 60 + min) / (12 * 60),
            .center = center});
   draw_simple_hand(ctx,
         (HandInfo){
            .main_len = radius - 13,
            .tail_len = 0,
            .angle = TRIG_MAX_ANGLE * min / 60,
            .center = center});

   // minute and hour hand
   graphics_context_set_stroke_color(ctx, GColorWhite);
   graphics_context_set_stroke_width(ctx, 3);
   draw_simple_hand(ctx,
         (HandInfo){
            .main_len = radius - 40,
            .tail_len = 0,
            .angle = TRIG_MAX_ANGLE * (hr * 60 + min) / (12 * 60),
            .center = center});
   draw_simple_hand(ctx,
         (HandInfo){
            .main_len = radius - 13,
            .tail_len = 0,
            .angle = TRIG_MAX_ANGLE * min / 60,
            .center = center});

   // minute and hour hand 'fill'
   graphics_context_set_stroke_color(ctx, GColorLightGray);
   graphics_context_set_stroke_width(ctx, 1);
   draw_simple_hand(ctx,
         (HandInfo){
            .main_len = radius - 40,
            .tail_len = 0,
            .angle = TRIG_MAX_ANGLE * (hr * 60 + min) / (12 * 60),
            .center = center});
   draw_simple_hand(ctx,
         (HandInfo){
            .main_len = radius - 13,
            .tail_len = 0,
            .angle = TRIG_MAX_ANGLE * min / 60,
            .center = center});

   // second hand
   graphics_context_set_stroke_color(ctx, GColorRajah);
   graphics_context_set_stroke_width(ctx, 1);
   draw_simple_hand(ctx,
         (HandInfo){
            .main_len = radius - 7,
            .tail_len = 9,
            .angle = TRIG_MAX_ANGLE * sec / 60,
            .center = center});

   // center of second hand
   graphics_context_set_fill_color(ctx, GColorRajah);
   graphics_fill_circle(ctx, center, 3);
   // center screw
   graphics_context_set_fill_color(ctx, GColorBlack);
   graphics_fill_circle(ctx, center, 2);

   END_TIME_MEASURE();
}

static void window_load(Window *window) {
   Layer *window_layer = window_get_root_layer(window);
   bounds = layer_get_bounds(window_layer);
   center = grect_center_point(&bounds);
   effect_layer = layer_create(bounds);
   layer_set_update_proc(effect_layer, update_effect_layer);
   layer_add_child(window_layer, effect_layer);
}

static void window_unload(Window *window) {
   layer_destroy(effect_layer);
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
   layer_mark_dirty(effect_layer);
   if (units_changed & DAY_UNIT) {
      snprintf(day, sizeof day, "%d", (int) tick_time->tm_mday);
   }
}

static void init(void) {
   window = window_create();
   tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
   window_set_window_handlers(window,
                              (WindowHandlers){
                                 .load = window_load, .unload = window_unload,
                              });
   font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SANS_13));
   window_stack_push(window, true);

   time_t t = time(NULL);
   struct tm *tm = localtime(&t);
   snprintf(day, sizeof day, "%d", (int) tm->tm_mday);
}

static void deinit(void) {
   window_destroy(window);
   fonts_unload_custom_font(font);
}

int main(void) {
   init();

   APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

   app_event_loop();
   deinit();
}
