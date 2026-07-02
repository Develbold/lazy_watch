#include "pebble.h"
#include "num2words.h"

#define FONT_MARGIN 20

#define PERSIST_KEY_BG_COLOR   1
#define PERSIST_KEY_TEXT_COLOR 2
#define PERSIST_KEY_ALIGN      3

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
  char buffer[FUZZY_TIME_BUFFER_SIZE];
} s_data;

static PropertyAnimation *slide_animation;
static PropertyAnimation *slide_out_animation;
static GRect frame;
static GFont s_font_small;
static GFont s_font_medium;
static GFont s_font_large;
static Layer *root_layer;
static GFont s_current_font;
static GRect s_label_dest;
static bool s_align_longest;

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

static GRect label_rect(const char *text, GFont font, int16_t y) {
  if (!s_align_longest) {
    return GRect(0, y, frame.size.w, frame.size.h);
  }
  GRect wide_box = GRect(0, 0, 10000, 10000);
  int16_t max_w = 0;
  const char *p = text;
  while (true) {
    const char *nl = strchr(p, '\n');
    size_t len = nl ? (size_t)(nl - p) : strlen(p);
    if (len > 0) {
      char line[FUZZY_TIME_BUFFER_SIZE];
      if (len >= FUZZY_TIME_BUFFER_SIZE) len = FUZZY_TIME_BUFFER_SIZE - 1;
      strncpy(line, p, len);
      line[len] = '\0';
      GSize sz = graphics_text_layout_get_content_size(
          line, font, wide_box, GTextOverflowModeWordWrap, GTextAlignmentLeft);
      if (sz.w > max_w) max_w = sz.w;
    }
    if (!nl) break;
    p = nl + 1;
  }
  if (max_w == 0 || max_w > frame.size.w) max_w = frame.size.w;
  return GRect((frame.size.w - max_w) / 2, y, max_w, frame.size.h);
}

typedef struct {
  TextLayer *label;
  char text[FUZZY_TIME_BUFFER_SIZE];
} SlideOutCtx;

static void old_label_anim_stopped(Animation *animation, bool finished, void *context) {
  SlideOutCtx *ctx = (SlideOutCtx *)context;
  layer_remove_from_parent(text_layer_get_layer(ctx->label));
  text_layer_destroy(ctx->label);
  free(ctx);
  slide_out_animation = NULL;
}

static void slide_out_old(const char *old_text, GFont old_font, GRect old_rect) {
  if (slide_out_animation) {
    animation_unschedule((Animation *)slide_out_animation);
  }
  SlideOutCtx *ctx = malloc(sizeof(SlideOutCtx));
  if (!ctx) return;
  strncpy(ctx->text, old_text, FUZZY_TIME_BUFFER_SIZE - 1);
  ctx->text[FUZZY_TIME_BUFFER_SIZE - 1] = '\0';
  ctx->label = text_layer_create(old_rect);
  text_layer_set_background_color(ctx->label, s_bg_color);
  text_layer_set_text_color(ctx->label, s_text_color);
  text_layer_set_font(ctx->label, old_font);
  text_layer_set_text_alignment(ctx->label,
      s_align_longest ? GTextAlignmentLeft : GTextAlignmentCenter);
  text_layer_set_text(ctx->label, ctx->text);
  layer_add_child(root_layer, text_layer_get_layer(ctx->label));

  GRect frame_from = old_rect;
  GRect frame_to = GRect(-frame.size.w, old_rect.origin.y, old_rect.size.w, old_rect.size.h);

  PropertyAnimation *anim = property_animation_create_layer_frame(
      text_layer_get_layer(ctx->label), &frame_from, &frame_to);
  animation_set_duration((Animation *)anim, 400);
  animation_set_curve((Animation *)anim, AnimationCurveEaseIn);
  animation_set_handlers((Animation *)anim,
      (AnimationHandlers){ .stopped = old_label_anim_stopped }, ctx);
  animation_schedule((Animation *)anim);
  slide_out_animation = anim;
}

static void slide_anim_stopped(Animation *animation, bool finished, void *context) {
  slide_animation = NULL;
}

static void update_time(struct tm *t) {
  bool has_old_text = s_data.buffer[0] != '\0';
  GFont old_font = s_current_font;
  GRect old_rect = s_label_dest;

  if (has_old_text) {
    slide_out_old(s_data.buffer, old_font, old_rect);
  }

  fuzzy_time_to_words(t->tm_hour, t->tm_min, s_data.buffer, FUZZY_TIME_BUFFER_SIZE);

  GSize content_size;
  GFont new_font = choose_font(s_data.buffer, &content_size);
  text_layer_set_font(s_data.label, new_font);
  text_layer_set_text_alignment(s_data.label,
      s_align_longest ? GTextAlignmentLeft : GTextAlignmentCenter);
  text_layer_set_text(s_data.label, s_data.buffer);
  int16_t y = (frame.size.h - content_size.h) / 2 - HEIGHT_CORRECTION;
  s_current_font = new_font;

  GRect frame_to = label_rect(s_data.buffer, new_font, y);
  GRect frame_from = GRect(frame.size.w, y, frame_to.size.w, frame_to.size.h);
  s_label_dest = frame_to;

  if (slide_animation) {
    animation_unschedule((Animation *)slide_animation);
    slide_animation = NULL;
  }
  layer_set_frame(text_layer_get_layer(s_data.label), frame_from);
  slide_animation = property_animation_create_layer_frame(
      text_layer_get_layer(s_data.label), &frame_from, &frame_to);
  animation_set_duration((Animation *)slide_animation, 400);
  animation_set_curve((Animation *)slide_animation, AnimationCurveEaseIn);
  animation_set_handlers((Animation *)slide_animation,
      (AnimationHandlers){ .stopped = slide_anim_stopped }, NULL);
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
  s_align_longest = persist_exists(PERSIST_KEY_ALIGN)
      ? (bool)persist_read_int(PERSIST_KEY_ALIGN) : false;
}

static void apply_colors(void) {
  window_set_background_color(s_data.window, s_bg_color);
  text_layer_set_background_color(s_data.label, s_bg_color);
  text_layer_set_text_color(s_data.label, s_text_color);
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *bg    = dict_find(iter, MESSAGE_KEY_backgroundColor);
  Tuple *fg    = dict_find(iter, MESSAGE_KEY_textColor);
  Tuple *align = dict_find(iter, MESSAGE_KEY_blockAlign);
  if (bg) {
    persist_write_int(PERSIST_KEY_BG_COLOR, bg->value->uint32);
    s_bg_color = GColorFromHEX(bg->value->uint32);
  }
  if (fg) {
    persist_write_int(PERSIST_KEY_TEXT_COLOR, fg->value->uint32);
    s_text_color = GColorFromHEX(fg->value->uint32);
  }
  apply_colors();
  if (align) {
    bool val = align->value->int8 != 0;
    persist_write_int(PERSIST_KEY_ALIGN, val ? 1 : 0);
    s_align_longest = val;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t) update_time(t);
  }
}

static void __attribute__((unused)) unobstructed_area_will_change(GRect final_area, void *context) {
  frame = final_area;
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  if (t) update_time(t);
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
  if (t) update_time(t);

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
