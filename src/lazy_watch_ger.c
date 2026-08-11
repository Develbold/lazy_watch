#include "pebble.h"
#include "num2words.h"

#define FONT_MARGIN 20

#define PERSIST_KEY_BG_COLOR   1
#define PERSIST_KEY_TEXT_COLOR 2
#define PERSIST_KEY_ALIGN      3
#define PERSIST_KEY_WORD_STYLE 4

enum { FONT_TIER_LARGE = 0, FONT_TIER_MEDIUM = 1, FONT_TIER_SMALL = 2, FONT_TIER_COUNT = 3 };
enum { WORD_STYLE_BOLD = 0, WORD_STYLE_NORMAL = 1, WORD_STYLE_ITALIC = 2 };

static GColor s_bg_color;
static GColor s_text_color;
static int s_word_style;

#ifdef CAPITAL
#define HEIGHT_CORRECTION 0
#elif defined HALF_CAPITAL
#define HEIGHT_CORRECTION 0
#else
#define HEIGHT_CORRECTION 5
#endif

static struct CommonWordsData {
  Layer *label;
  Window *window;
  char buffer[FUZZY_TIME_BUFFER_SIZE];
} s_data;

static PropertyAnimation *slide_animation;
static PropertyAnimation *slide_out_animation;
static GRect frame;
static GFont s_font_small;
static GFont s_font_medium;
static GFont s_font_large;
static GFont s_font_regular[FONT_TIER_COUNT];
static GFont s_font_italic[FONT_TIER_COUNT];
static Layer *root_layer;
static GFont s_current_font;
static int s_current_font_tier;
static GRect s_label_dest;
static bool s_align_longest;

static GFont choose_font(const char *text, GSize *out_size, int *out_tier) {
  GRect narrow_box = GRect(0, 0, frame.size.w, 10000);
  GRect wide_box   = GRect(0, 0, 10000, 10000);
  int16_t max_h = frame.size.h - FONT_MARGIN;

  GFont candidates[2] = {s_font_large, s_font_medium};
  int tiers[2] = {FONT_TIER_LARGE, FONT_TIER_MEDIUM};
  for (int i = 0; i < 2; i++) {
    GSize sz = graphics_text_layout_get_content_size(
        text, candidates[i], narrow_box, GTextOverflowModeWordWrap, GTextAlignmentCenter);
    if (sz.h > max_h) continue;
    GSize wide = graphics_text_layout_get_content_size(
        text, candidates[i], wide_box, GTextOverflowModeWordWrap, GTextAlignmentLeft);
    if (sz.h != wide.h) continue;
    *out_size = sz;
    *out_tier = tiers[i];
    return candidates[i];
  }
  *out_size = graphics_text_layout_get_content_size(
      text, s_font_small, narrow_box, GTextOverflowModeWordWrap, GTextAlignmentCenter);
  *out_tier = FONT_TIER_SMALL;
  return s_font_small;
}

// "vor"/"nach"/"Uhr" render in the number font (bold) unless the readability
// setting picked a different style at the same size tier as the numbers.
static GFont style_font_for_connector_word(GFont number_font, int tier) {
  switch (s_word_style) {
    case WORD_STYLE_NORMAL: return s_font_regular[tier];
    case WORD_STYLE_ITALIC: return s_font_italic[tier];
    default: return number_font;
  }
}

// vor/nach/Uhr always occupy a whole line on their own (see
// fuzzy_time_to_words), so per-word styling only needs per-line font choice,
// not intra-line mixed-font drawing.
static void draw_label_text(GContext *ctx, GRect bounds, const char *text,
                             GFont number_font, int tier, GTextAlignment alignment) {
  graphics_context_set_fill_color(ctx, s_bg_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, s_text_color);

  int16_t y = 0;
  const char *p = text;
  while (true) {
    const char *nl = strchr(p, '\n');
    size_t len = nl ? (size_t)(nl - p) : strlen(p);
    if (len > 0) {
      char line[FUZZY_TIME_BUFFER_SIZE];
      if (len >= FUZZY_TIME_BUFFER_SIZE) len = FUZZY_TIME_BUFFER_SIZE - 1;
      strncpy(line, p, len);
      line[len] = '\0';

      GFont line_font = fuzzy_time_is_connector_word(line)
          ? style_font_for_connector_word(number_font, tier) : number_font;
      GRect measure_box = GRect(0, 0, bounds.size.w, 10000);
      GSize line_size = graphics_text_layout_get_content_size(
          line, line_font, measure_box, GTextOverflowModeWordWrap, alignment);
      GRect line_rect = GRect(0, y, bounds.size.w, line_size.h);
      graphics_draw_text(ctx, line, line_font, line_rect,
          GTextOverflowModeWordWrap, alignment, NULL);
      y += line_size.h;
    }
    if (!nl) break;
    p = nl + 1;
  }
}

static void main_label_update_proc(Layer *layer, GContext *ctx) {
  draw_label_text(ctx, layer_get_bounds(layer), s_data.buffer, s_current_font, s_current_font_tier,
      s_align_longest ? GTextAlignmentLeft : GTextAlignmentCenter);
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
  char text[FUZZY_TIME_BUFFER_SIZE];
  GFont number_font;
  int tier;
  GTextAlignment alignment;
} SlideOutData;

static void slide_out_update_proc(Layer *layer, GContext *ctx) {
  SlideOutData *d = layer_get_data(layer);
  draw_label_text(ctx, layer_get_bounds(layer), d->text, d->number_font, d->tier, d->alignment);
}

static void old_label_anim_stopped(Animation *animation, bool finished, void *context) {
  Layer *old_layer = (Layer *)context;
  layer_remove_from_parent(old_layer);
  layer_destroy(old_layer);
  slide_out_animation = NULL;
}

static void slide_out_old(const char *old_text, GFont old_font, int old_tier, GRect old_rect) {
  if (slide_out_animation) {
    animation_unschedule((Animation *)slide_out_animation);
  }
  Layer *old_layer = layer_create_with_data(old_rect, sizeof(SlideOutData));
  SlideOutData *d = layer_get_data(old_layer);
  strncpy(d->text, old_text, FUZZY_TIME_BUFFER_SIZE - 1);
  d->text[FUZZY_TIME_BUFFER_SIZE - 1] = '\0';
  d->number_font = old_font;
  d->tier = old_tier;
  d->alignment = s_align_longest ? GTextAlignmentLeft : GTextAlignmentCenter;
  layer_set_update_proc(old_layer, slide_out_update_proc);
  layer_add_child(root_layer, old_layer);

  GRect frame_from = old_rect;
  GRect frame_to = GRect(-frame.size.w, old_rect.origin.y, old_rect.size.w, old_rect.size.h);

  PropertyAnimation *anim = property_animation_create_layer_frame(old_layer, &frame_from, &frame_to);
  animation_set_duration((Animation *)anim, 400);
  animation_set_curve((Animation *)anim, AnimationCurveEaseIn);
  animation_set_handlers((Animation *)anim,
      (AnimationHandlers){ .stopped = old_label_anim_stopped }, old_layer);
  animation_schedule((Animation *)anim);
  slide_out_animation = anim;
}

static void slide_anim_stopped(Animation *animation, bool finished, void *context) {
  slide_animation = NULL;
}

static void update_time(struct tm *t) {
  char new_text[FUZZY_TIME_BUFFER_SIZE];
  fuzzy_time_to_words(t->tm_hour, t->tm_min, new_text, FUZZY_TIME_BUFFER_SIZE);

  bool has_old_text = s_data.buffer[0] != '\0';
  bool text_changed = !has_old_text || strcmp(s_data.buffer, new_text) != 0;
  GFont old_font = s_current_font;
  int old_tier = s_current_font_tier;
  GRect old_rect = s_label_dest;

  if (has_old_text && text_changed) {
    slide_out_old(s_data.buffer, old_font, old_tier, old_rect);
  }

  memcpy(s_data.buffer, new_text, FUZZY_TIME_BUFFER_SIZE);

  GSize content_size;
  int new_tier;
  GFont new_font = choose_font(s_data.buffer, &content_size, &new_tier);
  int16_t y = (frame.size.h - content_size.h) / 2 - HEIGHT_CORRECTION;
  s_current_font = new_font;
  s_current_font_tier = new_tier;
  layer_mark_dirty(s_data.label);

  GRect frame_to = label_rect(s_data.buffer, new_font, y);
  s_label_dest = frame_to;

  if (slide_animation) {
    animation_unschedule((Animation *)slide_animation);
    slide_animation = NULL;
  }

  if (!text_changed) {
    // Reposition instantly (e.g. a reflow after obstruction/frame change) —
    // nothing actually changed on screen, so replaying the slide would be a
    // redundant duplicate transition.
    layer_set_frame(s_data.label, frame_to);
    return;
  }

  GRect frame_from = GRect(frame.size.w, y, frame_to.size.w, frame_to.size.h);
  layer_set_frame(s_data.label, frame_from);
  slide_animation = property_animation_create_layer_frame(
      s_data.label, &frame_from, &frame_to);
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
  s_word_style = persist_exists(PERSIST_KEY_WORD_STYLE)
      ? persist_read_int(PERSIST_KEY_WORD_STYLE) : WORD_STYLE_BOLD;
}

static void apply_colors(void) {
  window_set_background_color(s_data.window, s_bg_color);
  layer_mark_dirty(s_data.label);
}

static int word_style_from_string(const char *value) {
  if (strcmp(value, "normal") == 0) return WORD_STYLE_NORMAL;
  if (strcmp(value, "italic") == 0) return WORD_STYLE_ITALIC;
  return WORD_STYLE_BOLD;
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *bg         = dict_find(iter, MESSAGE_KEY_backgroundColor);
  Tuple *fg         = dict_find(iter, MESSAGE_KEY_textColor);
  Tuple *align      = dict_find(iter, MESSAGE_KEY_blockAlign);
  Tuple *word_style = dict_find(iter, MESSAGE_KEY_wordStyle);
  if (bg) {
    persist_write_int(PERSIST_KEY_BG_COLOR, bg->value->uint32);
    s_bg_color = GColorFromHEX(bg->value->uint32);
  }
  if (fg) {
    persist_write_int(PERSIST_KEY_TEXT_COLOR, fg->value->uint32);
    s_text_color = GColorFromHEX(fg->value->uint32);
  }
  apply_colors();
  if (word_style) {
    s_word_style = word_style_from_string(word_style->value->cstring);
    persist_write_int(PERSIST_KEY_WORD_STYLE, s_word_style);
    layer_mark_dirty(s_data.label);
  }
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

  s_font_regular[FONT_TIER_LARGE]  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CaviarDreamsRegular_52));
  s_font_regular[FONT_TIER_MEDIUM] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CaviarDreamsRegular_38));
  s_font_regular[FONT_TIER_SMALL]  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CaviarDreamsRegular_29));

  s_font_italic[FONT_TIER_LARGE]  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CaviarDreamsItalic_52));
  s_font_italic[FONT_TIER_MEDIUM] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CaviarDreamsItalic_38));
  s_font_italic[FONT_TIER_SMALL]  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CaviarDreamsItalic_29));

  root_layer = window_get_root_layer(s_data.window);
  frame = layer_get_frame(root_layer);

  s_current_font = s_font_small;
  s_current_font_tier = FONT_TIER_SMALL;
  s_data.label = layer_create(GRect(0, 0, frame.size.w, frame.size.h));
  layer_set_update_proc(s_data.label, main_label_update_proc);
  layer_add_child(root_layer, s_data.label);

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
  layer_destroy(s_data.label);
  fonts_unload_custom_font(s_font_small);
  fonts_unload_custom_font(s_font_medium);
  fonts_unload_custom_font(s_font_large);
  for (int i = 0; i < FONT_TIER_COUNT; i++) {
    fonts_unload_custom_font(s_font_regular[i]);
    fonts_unload_custom_font(s_font_italic[i]);
  }
  window_destroy(s_data.window);
}

int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}
