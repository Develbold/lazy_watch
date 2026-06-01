#include "pebble.h"
#include "num2words.h"

#define BUFFER_SIZE 86
#define FONT_MARGIN 20

#ifdef CAPITAL
#define HEIGHT_CORRECTION 0
#elif defined HALF_CAPITAL
#define HEIGHT_CORRECTION 0
#else
#define HEIGHT_CORRECTION 5
#endif

static struct CommonWordsData {
  TextLayer *label;
  Window *window;
  char buffer[BUFFER_SIZE];
} s_data;

static PropertyAnimation *slide_animation;
static GRect frame;
static GFont s_font_small;
static GFont s_font_medium;
static GFont s_font_large;
static Layer *root_layer;

static bool word_fits_width(const char *text, GFont font) {
  GSize narrow = graphics_text_layout_get_content_size(
      text, font, GRect(0, 0, frame.size.w, 10000),
      GTextOverflowModeWordWrap, GTextAlignmentLeft);
  GSize wide = graphics_text_layout_get_content_size(
      text, font, GRect(0, 0, 10000, 10000),
      GTextOverflowModeWordWrap, GTextAlignmentLeft);
  return narrow.h == wide.h;
}

static GFont choose_font(const char *text) {
  GRect measure_box = GRect(0, 0, frame.size.w, frame.size.h);
  int16_t max_h = frame.size.h - FONT_MARGIN;
  GSize sz;

  sz = graphics_text_layout_get_content_size(
      text, s_font_large, measure_box, GTextOverflowModeWordWrap, GTextAlignmentCenter);
  if (sz.h <= max_h && word_fits_width(text, s_font_large)) return s_font_large;

  sz = graphics_text_layout_get_content_size(
      text, s_font_medium, measure_box, GTextOverflowModeWordWrap, GTextAlignmentCenter);
  if (sz.h <= max_h && word_fits_width(text, s_font_medium)) return s_font_medium;

  return s_font_small;
}

static void update_time(struct tm *t) {
  fuzzy_time_to_words(t->tm_hour, t->tm_min, s_data.buffer, BUFFER_SIZE);

  text_layer_set_font(s_data.label, choose_font(s_data.buffer));
  text_layer_set_text(s_data.label, s_data.buffer);

  GSize content_size = text_layer_get_content_size(s_data.label);
  int16_t y = (frame.size.h - content_size.h) / 2 - HEIGHT_CORRECTION;

  GRect frame_from = GRect(frame.size.w, y, frame.size.w, frame.size.h);
  GRect frame_to = GRect(0, y, frame.size.w, frame.size.h);

  layer_set_frame(text_layer_get_layer(s_data.label), frame_to);

  slide_animation = property_animation_create_layer_frame(
      text_layer_get_layer(s_data.label), &frame_from, &frame_to);
  animation_set_duration((Animation *)slide_animation, 400);
  animation_set_curve((Animation *)slide_animation, AnimationCurveEaseIn);
  animation_set_delay((Animation *)slide_animation, 0);
  animation_schedule((Animation *)slide_animation);
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
}

static void do_init(void) {
  s_data.window = window_create();
  window_stack_push(s_data.window, true);

  window_set_background_color(s_data.window, GColorBlack);
  s_font_small  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CaviarDreamsBold_29));
  s_font_medium = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CaviarDreamsBold_38));
  s_font_large  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CaviarDreamsBold_52));

  root_layer = window_get_root_layer(s_data.window);
  frame = layer_get_frame(root_layer);

  s_data.label = text_layer_create(GRect(0, 0, frame.size.w, frame.size.h));
  text_layer_set_background_color(s_data.label, GColorBlack);
  text_layer_set_text_color(s_data.label, GColorWhite);
  text_layer_set_font(s_data.label, s_font_small);
  text_layer_set_text_alignment(s_data.label, GTextAlignmentCenter);
  layer_add_child(root_layer, text_layer_get_layer(s_data.label));

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  update_time(t);

  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
}

static void do_deinit(void) {
  tick_timer_service_unsubscribe();
  text_layer_destroy(s_data.label);
  fonts_unload_custom_font(s_font_small);
  fonts_unload_custom_font(s_font_medium);
  fonts_unload_custom_font(s_font_large);
  window_destroy(s_data.window);
}

int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}
