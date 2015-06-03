#include <pebble.h>


#define ABS(a) (((a) > 0) ? (a) : -1 * (a))
#define IMAGES_LENGTH 2



static Window *s_main_window;
static Layer *icon_layer;
static GDrawCommandImage* images [IMAGES_LENGTH];


static int to_square_normalized;
static int current_image;

static int16_t prv_int_attract_to(int16_t i, int16_t bounds, int32_t normalized) {
  const int16_t delta_0 = (int16_t) ((0 + 1) - i);
  const int16_t delta_b = (int16_t) ((bounds - 1) - i);
  const int16_t delta = ABS(delta_0) < ABS(delta_b) ? delta_0 : delta_b;

  return (int16_t) (i + delta * normalized / ANIMATION_NORMALIZED_MAX);
}

GPoint gpoint_attract_to_square(GPoint point, GSize size, int32_t normalized) {
  point.y += 1;
  point = GPoint(
      prv_int_attract_to(point.x, size.w, normalized),
      prv_int_attract_to(point.y, size.h, normalized));
  return point;
}

typedef struct {
  GSize size;
  int32_t normalized;
} ToSquareCBContext;

static bool prv_attract_draw_command_list_to_square_cb(GDrawCommand *command, uint32_t index, void *context) {

  ToSquareCBContext *to_square = context;
  for (int i = 0; i < gdraw_command_get_num_points(command); i++) {
    gdraw_command_set_point(command, i, gpoint_attract_to_square(gdraw_command_get_point(command, i), to_square->size, to_square->normalized));
  }
  return true;
}

void attract_draw_command_list_to_square(GDrawCommandList *list, GSize size, int32_t normalized) {
  ToSquareCBContext ctx = {
      .size = size,
      .normalized = normalized,
  };
  gdraw_command_list_iterate(list, prv_attract_draw_command_list_to_square_cb, &ctx);
}

void attract_draw_command_image_to_square(GDrawCommandImage *image, int32_t normalized) {
  attract_draw_command_list_to_square(gdraw_command_image_get_command_list(image), gdraw_command_image_get_bounds_size(image), normalized);
}

//Update callback for the background layer

static void bg_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect screen_rect = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_rect(ctx, screen_rect, 0, GCornerNone);
}


//Update callback for the icon layer

static void icon_layer_update_proc(Layer *layer, GContext *ctx) {
  if (!images[current_image]) {
    return;
  }

  GDrawCommandImage *temp_copy = gdraw_command_image_clone(images[current_image]);
  attract_draw_command_image_to_square(temp_copy, to_square_normalized);
  graphics_context_set_antialiased(ctx, true);
  gdraw_command_image_draw(ctx, temp_copy, GPoint(0, 0));
  free(temp_copy);
}

//Animation update callback

static void update_icon_square_normalized(Animation *animation, const uint32_t distance_normalized) {
  to_square_normalized = distance_normalized;
  layer_mark_dirty(icon_layer);
}

static const PropertyAnimationImplementation s_icon_square_normalized_implementation = {
  .base = {
    .update = (AnimationUpdateImplementation) update_icon_square_normalized,
  },
};

//function prototype
static void stopped_animation_handler(Animation *animation, bool finished, void *context);

static void schedule_animation_to_square(){
  Animation * animation_to_square = (Animation *) property_animation_create(&s_icon_square_normalized_implementation, images[0], NULL, NULL);
  animation_set_duration(animation_to_square, 1200);
  animation_set_curve(animation_to_square, AnimationCurveEaseInOut);
  animation_set_handlers(animation_to_square, (AnimationHandlers) {
    .stopped = stopped_animation_handler,
  }, NULL);




  animation_schedule(animation_to_square);
}

static void schedule_animation_from_square(){

  Animation *animation_from_square = (Animation *) property_animation_create(&s_icon_square_normalized_implementation, images[1], NULL, NULL);
  animation_set_duration(animation_from_square, 1200);
  animation_set_reverse(animation_from_square, true);
  animation_set_handlers(animation_from_square, (AnimationHandlers) {
    .stopped = stopped_animation_handler,
  }, NULL);



  animation_schedule(animation_from_square);
}

//Animation stopped callback handler. Switches between the animations

static void stopped_animation_handler(Animation *animation, bool finished, void *context) {
    current_image = (current_image+1)%2;
    switch(current_image){
      case 0: schedule_animation_to_square();break;
      default: schedule_animation_from_square();break;
    }
  }



static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  layer_set_update_proc(window_layer, bg_layer_update_proc);
  GRect icon_rect = GRect(50, 50, 50, 50);
  icon_layer = layer_create(icon_rect);
  layer_set_update_proc(icon_layer, icon_layer_update_proc);
  layer_add_child(window_layer, icon_layer);

  schedule_animation_to_square();
}

static void main_window_unload(Window *window) {
  layer_destroy(icon_layer);
}


static void init() {
  images[0] = gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_HEAVY_RAIN);
  images[1] = gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_DOCUMENT);
  current_image = 0;
  to_square_normalized = 0;

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  for(int j = 0; j < IMAGES_LENGTH; j++)free(images[j]);
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}