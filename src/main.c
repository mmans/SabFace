#include <pebble.h>

Window *my_window;
BitmapLayer *s_background_layer;
TextLayer *s_dnloads_layer;
TextLayer *s_time_layer;

GFont *my_font;
GBitmap *my_background;

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
  update_time(tick_time);
}


// Communication

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

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
  
  APP_LOG(APP_LOG_LEVEL_INFO, "Init complete!");
  
}

static void main_window_unload(Window *window) {
  bitmap_layer_destroy(s_background_layer);
  text_layer_destroy(s_time_layer);
  fonts_unload_custom_font(my_font);
}

void handle_init(void) {
  my_window = window_create();
  
  window_set_window_handlers(my_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
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
