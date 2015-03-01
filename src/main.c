#include <pebble.h>
#define KEY_VALID_CONNECTION  0
#define KEY_MB_TOTAL 1
#define KEY_MB_LEFT 2
#define KEY_DOWNLOADS 3
#define KEY_TIME_LEFT 4
#define KEY_SPEED 5
#define KEY_PROC_LEFT 6

bool init = true;
Window *my_window;
BitmapLayer *s_background_layer;
Layer *s_progress_layer;
TextLayer *s_dnloads_layer;
TextLayer *s_time_layer;

GFont *my_font;
GBitmap *my_background;

static int tick_counter = 0;
static bool sab_valid_connection = false;
static uint32_t sab_mb_total = 0;
static uint32_t sab_mb_left = 0;
static int sab_downloads = 0;
static char sab_time_left[8] = "00:00:00";
static char sab_speed[15] = "";
static int sab_proc_left = 0;

// Upgrade Progressbar
static void update_progressbar(struct Layer *layer, GContext *ctx){
  static int i=0;
  i+=5;
  if (i>100) i=0;  
  int pg = i*114/100;
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_round_rect(ctx, GRect(0, 0, 120, 8), 2);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, GRect(3, 3, pg, 2));
}

// Time updating
static void update_time(struct tm *tick_time){ 
  static char strbuf[] = "00:00";
  if (clock_is_24h_style()){
    strftime(strbuf, sizeof(strbuf), "%H:%M" , tick_time);
  } else {
    strftime(strbuf, sizeof(strbuf), "%I:%M" , tick_time);
  }
  text_layer_set_text(s_time_layer, strbuf);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  if (init || (units_changed & MINUTE_UNIT)) {
    update_time(tick_time);
    init = false;
  }
  tick_counter++;
  if (tick_counter>10){
    tick_counter=0;
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}


// Communication

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
 // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_VALID_CONNECTION: 
      sab_valid_connection = (bool) t->value;
      break;
    case KEY_MB_TOTAL:
      sab_mb_total = t->value->uint32;
      break;
    case KEY_MB_LEFT:
      sab_mb_left = t->value->uint32;
      break;
    case KEY_DOWNLOADS:
      sab_downloads = t->value->int32;
      break;
    case KEY_TIME_LEFT:
      snprintf(sab_time_left, sizeof(sab_time_left), "%s", t->value->cstring);
      break;
    case KEY_SPEED:
      snprintf(sab_speed, sizeof(sab_speed), "%s", t->value->cstring);
      break;
    case KEY_PROC_LEFT:
      sab_proc_left = t->value->int16;
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }
    
    // Look for next item
    t = dict_read_next(iterator);
  }
  
  APP_LOG(APP_LOG_LEVEL_INFO, "Valid Connection = %d", sab_valid_connection);
  APP_LOG(APP_LOG_LEVEL_INFO, "MB Total = %lu", sab_mb_total);
  APP_LOG(APP_LOG_LEVEL_INFO, "MB Left = %lu", sab_mb_left);
  APP_LOG(APP_LOG_LEVEL_INFO, "Downloads = %d", sab_downloads);
  APP_LOG(APP_LOG_LEVEL_INFO, "Time Left = %s", sab_time_left);
  APP_LOG(APP_LOG_LEVEL_INFO, "Speed = %s", sab_speed);
  APP_LOG(APP_LOG_LEVEL_INFO, "Proc Left = %d", sab_proc_left);

}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

// Initialisation

static void main_window_load(Window *window) {
  // Load background
  my_background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TELEMANS_FACE);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, my_background);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  
  // Add downloads layer
  s_dnloads_layer = text_layer_create(GRect(0, 15, 144, 22));
  text_layer_set_text(s_dnloads_layer, "...SabFace...");
  text_layer_set_font(s_dnloads_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_dnloads_layer, GTextAlignmentCenter);
  text_layer_set_text_color(s_dnloads_layer, GColorWhite);
  text_layer_set_background_color(s_dnloads_layer, GColorClear);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_dnloads_layer));
  
  // Load speacial font
  my_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TELEMANS_CAPTURE_IT_48));
  
  // Create time layer
  s_time_layer = text_layer_create(GRect(0, 55, 144, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "--:--");
  
  // Format time layer
  text_layer_set_font(s_time_layer, my_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Add time layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
 
  // Add Progress Bar
  s_progress_layer = layer_create(GRect(10, 135, 120, 8));
  layer_set_update_proc(s_progress_layer, update_progressbar);
  layer_add_child(window_get_root_layer(window), s_progress_layer);
  
  APP_LOG(APP_LOG_LEVEL_INFO, "Init complete!");
  
}

static void main_window_unload(Window *window) {
  bitmap_layer_destroy(s_background_layer);
  text_layer_destroy(s_time_layer);
  layer_destroy(s_progress_layer);
  fonts_unload_custom_font(my_font);
}

void handle_init(void) {
  my_window = window_create();
  
  window_set_window_handlers(my_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  window_stack_push(my_window, true);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void handle_deinit(void) {
  window_destroy(my_window);
}


int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
