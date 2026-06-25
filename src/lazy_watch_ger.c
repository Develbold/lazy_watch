#include "pebble.h"
#include "num2words.h"

#define BUFFER_SIZE 86
#define FONT_MARGIN 20

#define PERSIST_KEY_BG_COLOR   1
#define PERSIST_KEY_TEXT_COLOR 2

static GColor s_bg_color;
static GColor s_text_color;

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

static GFont choose_font(const char *text, GSize *out_size) {
  GRect narrow_box = GRect(0, 0, frame.size.w, 10000);
  GRect wide_box   = GRect(0, 0, 10000, 10000);
  int16_t max_h = frame.size.h - FONT_MARGIN;

  GFont candidates[2] = {s_font_large, s_font_medium};
  for (int i = 0; i < 2; i++) {
    GSize sz = graphics_text_layout_get_content_size(
        text, candidates[i], narrow_box, GTextOverflowModeWordWrap, GTextAlignmentCenter);
    if (sz.h > max_h) continue;
    GSize wide = graphics_text_layout_get_content_size(
        text, candidates[i], wide_box, GTextOverflowModeWordWrap, GTextAlignmentLeft);
    if (sz.h != wide.h) continue;
    *out_size = sz;
    return candidates[i];
  }
  *out_size = graphics_text_layout_get_content_size(
      text, s_font_small, narrow_box, GTextOverflowModeWordWrap, GTextAlignmentCenter);
  return s_font_small;
}

static void update_time(struct tm *t) {
  fuzzy_time_to_words(t->tm_hour, t->tm_min, s_data.buffer, BUFFER_SIZE);

  GSize content_size;
  text_layer_set_font(s_data.label, choose_font(s_data.buffer, &content_size));
  text_layer_set_text(s_data.label, s_data.buffer);
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

static void load_colors(void) {
  s_bg_color = persist_exists(PERSIST_KEY_BG_COLOR)
      ? GColorFromHEX(persist_read_int(PERSIST_KEY_BG_COLOR)) : GColorBlack;
  s_text_color = persist_exists(PERSIST_KEY_TEXT_COLOR)
      ? GColorFromHEX(persist_read_int(PERSIST_KEY_TEXT_COLOR)) : GColorWhite;
}

static void apply_colors(void) {
  window_set_background_color(s_data.window, s_bg_color);
  text_layer_set_background_color(s_data.label, s_bg_color);
  text_layer_set_text_color(s_data.label, s_text_color);
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *bg = dict_find(iter, MESSAGE_KEY_backgroundColor);
  Tuple *fg = dict_find(iter, MESSAGE_KEY_textColor);
  if (bg) {
    persist_write_int(PERSIST_KEY_BG_COLOR, bg->value->uint32);
    s_bg_color = GColorFromHEX(bg->value->uint32);
  }
  if (fg) {
    persist_write_int(PERSIST_KEY_TEXT_COLOR, fg->value->uint32);
    s_text_color = GColorFromHEX(fg->value->uint32);
  }
  apply_colors();
}

static void __attribute__((unused)) unobstructed_area_will_change(GRect final_area, void *context) {
  frame = final_area;
  time_t now = time(NULL);
  update_time(localtime(&now));
}

static void do_init(void) {
  s_data.window = window_create();
  window_stack_push(s_data.window, true);

  s_font_small  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CaviarDreamsBold_29));
  s_font_medium = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CaviarDreamsBold_38));
  s_font_large  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CaviarDreamsBold_52));

  root_layer = window_get_root_layer(s_data.window);
  frame = layer_get_frame(root_layer);

  s_data.label = text_layer_create(GRect(0, 0, frame.size.w, frame.size.h));
  text_layer_set_font(s_data.label, s_font_small);
  text_layer_set_text_alignment(s_data.label, GTextAlignmentCenter);
  layer_add_child(root_layer, text_layer_get_layer(s_data.label));

  load_colors();
  apply_colors();

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  update_time(t);

  app_message_register_inbox_received(inbox_received);
  app_message_open(64, 64);
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
  unobstructed_area_service_subscribe(
      (UnobstructedAreaHandlers){ .will_change = unobstructed_area_will_change },
      NULL);
}

static void do_deinit(void) {
  unobstructed_area_service_unsubscribe();
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
