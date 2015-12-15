#include <pebble.h>

#define ABS(a) (((a) > 0) ? (a) : -1 * (a))
#define IMAGES_LENGTH 2
#define ANIMATION_DURATION 1200

static Window *s_main_window;
static Layer *s_icon_layer;
static GDrawCommandImage *s_images[IMAGES_LENGTH];

static int s_to_square_normalized;
static int s_current_image;

typedef struct {
  GSize size;
  int32_t normalized;
} ToSquareCBContext;

// Calculates where a point should be on an axis (determined by bounds). normalized is the progress through the animation.
static int16_t calculate_normalized_value(int16_t point, int16_t bounds, int32_t normalized) {
  const int16_t delta_0 = (int16_t) ((0 + 1) - point);
  const int16_t delta_b = (int16_t) ((bounds - 1) - point);
  const int16_t delta = ABS(delta_0) < ABS(delta_b) ? delta_0 : delta_b;

  return (int16_t) (point + delta * normalized / ANIMATION_NORMALIZED_MAX);
}

// Calculates the position of a point based on the progress through the animation.
static GPoint gpoint_attract_to_square(GPoint point, GSize size, int32_t normalized) {
  point.y += 1;
  point = GPoint(
      calculate_normalized_value(point.x, size.w, normalized),
      calculate_normalized_value(point.y, size.h, normalized));
  return point;
}

static bool gdraw_command_update_normalized_values(GDrawCommand *command, uint32_t index, void *context) {
  ToSquareCBContext *to_square = context;
  for (int i = 0; i < gdraw_command_get_num_points(command); i++) {
    gdraw_command_set_point(command, i, gpoint_attract_to_square(gdraw_command_get_point(command, i), to_square->size, to_square->normalized));
  }
  return true;
}

static void attract_draw_command_list_to_square(GDrawCommandList *list, GSize size, int32_t normalized) {
  ToSquareCBContext ctx = {
      .size = size,
      .normalized = normalized,
  };
  gdraw_command_list_iterate(list, gdraw_command_update_normalized_values, &ctx);
}

static void attract_draw_command_image_to_square(GDrawCommandImage *image, int32_t normalized) {
  attract_draw_command_list_to_square(gdraw_command_image_get_command_list(image), gdraw_command_image_get_bounds_size(image), normalized);
}

// Update callback for the background layer
static void bg_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect screen_rect = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorRed, GColorWhite));
  graphics_fill_rect(ctx, screen_rect, 0, GCornerNone);
}

// Update callback for the icon layer
static void icon_layer_update_proc(Layer *layer, GContext *ctx) {
  if (!s_images[s_current_image]) {
    return;
  }
  GDrawCommandImage *temp_copy = gdraw_command_image_clone(s_images[s_current_image]);
  attract_draw_command_image_to_square(temp_copy, s_to_square_normalized);
  graphics_context_set_antialiased(ctx, true);
  gdraw_command_image_draw(ctx, temp_copy, GPoint(0, 0));
  gdraw_command_image_destroy(temp_copy);
}

// Animation update callback
static void update_icon_square_normalized(Animation *animation, const uint32_t distance_normalized) {
  s_to_square_normalized = distance_normalized;
  layer_mark_dirty(s_icon_layer);
}

static const PropertyAnimationImplementation s_icon_square_normalized_implementation = {
  .base = {
    .update = (AnimationUpdateImplementation) update_icon_square_normalized,
  },
};

// function prototype
static void stopped_animation_handler(Animation *animation, bool finished, void *context);

static void schedule_animation_to_square(){
  Animation * animation_to_square = (Animation *) property_animation_create(&s_icon_square_normalized_implementation, s_images[0], NULL, NULL);
  animation_set_duration(animation_to_square, ANIMATION_DURATION);
  animation_set_curve(animation_to_square, AnimationCurveEaseInOut);
  animation_set_handlers(animation_to_square, (AnimationHandlers) {
    .stopped = stopped_animation_handler,
  }, NULL);
  animation_schedule(animation_to_square);
}

static void schedule_animation_from_square(){
  Animation *animation_from_square = (Animation *) property_animation_create(&s_icon_square_normalized_implementation, s_images[1], NULL, NULL);
  animation_set_duration(animation_from_square, ANIMATION_DURATION);
  animation_set_reverse(animation_from_square, true);
  animation_set_handlers(animation_from_square, (AnimationHandlers) {
    .stopped = stopped_animation_handler,
  }, NULL);
  animation_schedule(animation_from_square);
}

// Animation stopped callback handler. Switches between the animations
static void stopped_animation_handler(Animation *animation, bool finished, void *context) {
    s_current_image = (s_current_image+1)%2;
    switch(s_current_image){
      case 0: schedule_animation_to_square();break;
      case 1: schedule_animation_from_square();break;
    }
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  layer_set_update_proc(window_layer, bg_layer_update_proc);
  GRect icon_rect = GRect(50, 50, 50, 50);
  s_icon_layer = layer_create(icon_rect);
  layer_set_update_proc(s_icon_layer, icon_layer_update_proc);
  layer_add_child(window_layer, s_icon_layer);
  schedule_animation_to_square();
}

static void main_window_unload(Window *window) {
  layer_destroy(s_icon_layer);
}

static void init() {
  s_images[0] = gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_HEAVY_RAIN);
  s_images[1] = gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_DOCUMENT);
  s_current_image = 0;
  s_to_square_normalized = 0;
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  for(int j = 0; j < IMAGES_LENGTH; j++){
    gdraw_command_image_destroy(s_images[j]);
  }
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}