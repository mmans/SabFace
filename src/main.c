/*
 TODO:
   * Settings --> Struct
   * Vib-settings
   * Int-settings
   * icon, not-connected
*/

#include <pebble.h>
#define KEY_VALID_CONNECTION  0
#define KEY_MB_TOTAL 1
#define KEY_MB_LEFT 2
#define KEY_DOWNLOADS 3
#define KEY_TIME_LEFT 4
#define KEY_SPEED 5
#define KEY_PROC_LEFT 6
#define CMD_TYPE 7
#define CONFIG_SAB_USE_SSL 8
#define CONFIG_SAB_HOST 9
#define CONFIG_SAB_PORT 10
#define CONFIG_SAB_APIKEY 11
#define CONFIG_SAB_USERNAME 12
#define CONFIG_SAB_PASSWORD 13
#define CONFIG_WATCH_VIB_NEW 14
#define CONFIG_WATCH_VIB_FIN 15
#define CONFIG_WATCH_INT_IDLE 16
#define CONFIG_WATCH_INT_ACTIVE 17 

static bool config_sab_use_ssl = false;
static char config_sab_host[50] = "my.sabnzb.server";
static int config_sab_port = 8080;
static char config_sab_apikey[35] = "";
static char config_sab_username[25] = "";
static char config_sab_password[25] = "";
static bool config_vibrate_new = false;
static bool config_vibrate_finish = false;
static int config_interval_idle = 900;
static int config_interval_active = 10;
  
bool init = true;
Window *my_window;
BitmapLayer *s_background_layer;
Layer *s_progress_layer;
TextLayer *s_info_layer;
TextLayer *s_time_layer;
BitmapLayer *s_downloads_icon_layer;
TextLayer *s_downloads_layer;
BitmapLayer *s_speed_icon_layer;
TextLayer *s_speed_layer;
TextLayer *s_time_left_layer;

GFont *my_font;
GBitmap *my_background;
GBitmap *icon_downloads;
GBitmap *icon_speed;

static int tick_counter = 0;
static bool sab_valid_connection = false;
static uint32_t sab_mb_total = 0;
static uint32_t sab_mb_left = 0;
static int sab_downloads_prev = -1;
static int sab_downloads = 0;
static char sab_downloads_txt[3] = "000";
static char sab_time_left[8] = "00:00:00";
static char sab_speed[8] = "";
static int sab_proc_left = 0;

// Upgrade Progressbar
static void update_progressbar(struct Layer *layer, GContext *ctx){
    int pg = (100-sab_proc_left)*114/100;
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
  if (tick_counter>5){
    tick_counter=0;
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    //CMD_TYPE:
    // 1 = Update data
    // 2 = Send Config
    
    // Add a key-value pair
    dict_write_uint8(iter, CMD_TYPE, 1);

    // Send the message!
    app_message_outbox_send();
  }
}

static void read_configuration(){
  config_sab_use_ssl = persist_read_bool(CONFIG_SAB_USE_SSL);
  persist_read_string(CONFIG_SAB_HOST, config_sab_host, sizeof(config_sab_host));
  config_sab_port = persist_read_int(CONFIG_SAB_PORT);
  persist_read_string(CONFIG_SAB_APIKEY, config_sab_apikey, sizeof(config_sab_apikey));
  persist_read_string(CONFIG_SAB_USERNAME, config_sab_username, sizeof(config_sab_username));
  persist_read_string(CONFIG_SAB_PASSWORD, config_sab_password, sizeof(config_sab_password));
  config_vibrate_new = persist_read_bool(CONFIG_WATCH_VIB_NEW);
  config_vibrate_finish = persist_read_bool(CONFIG_WATCH_VIB_FIN);
  config_interval_idle = persist_read_int(CONFIG_WATCH_INT_IDLE);
  config_interval_active = persist_read_int(CONFIG_WATCH_INT_ACTIVE);
}

static void send_configuration(){
    APP_LOG(APP_LOG_LEVEL_INFO, "Sending configuration");
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    //CMD_TYPE:
    // 1 = Update data
    // 2 = Send Config
    
    // Add a key-value pair
    dict_write_uint8(iter, CMD_TYPE, 2);
    dict_write_int8(iter, CONFIG_SAB_USE_SSL, config_sab_use_ssl);
    dict_write_cstring(iter, CONFIG_SAB_HOST, config_sab_host);
    dict_write_int16(iter, CONFIG_SAB_PORT, config_sab_port);
    dict_write_cstring(iter, CONFIG_SAB_APIKEY, config_sab_apikey);
    dict_write_cstring(iter, CONFIG_SAB_USERNAME, config_sab_username);
    dict_write_cstring(iter, CONFIG_SAB_PASSWORD, config_sab_password);
    dict_write_int8(iter, CONFIG_WATCH_VIB_NEW, config_vibrate_new);
    dict_write_int8(iter, CONFIG_WATCH_VIB_FIN, config_vibrate_finish);
    dict_write_int16(iter, CONFIG_WATCH_INT_IDLE, config_interval_idle);
    dict_write_int16(iter, CONFIG_WATCH_INT_ACTIVE, config_interval_active);
  
    // Send the message!
    app_message_outbox_send();  
}

static void save_configuration(){
  persist_write_bool(CONFIG_SAB_USE_SSL, config_sab_use_ssl);
  persist_write_string(CONFIG_SAB_HOST, config_sab_host);
  persist_write_int(CONFIG_SAB_PORT, config_sab_port);
  persist_write_string(CONFIG_SAB_APIKEY, config_sab_apikey);
  persist_write_string(CONFIG_SAB_USERNAME, config_sab_username);
  persist_write_string(CONFIG_SAB_PASSWORD, config_sab_password);
  persist_write_bool(CONFIG_WATCH_VIB_NEW, config_vibrate_new);
  persist_write_bool(CONFIG_WATCH_VIB_FIN, config_vibrate_finish);
  persist_write_int(CONFIG_WATCH_INT_IDLE, config_interval_idle);
  persist_write_int(CONFIG_WATCH_INT_ACTIVE, config_interval_active);
}

static void updateDataOnScreen(){
  if ((sab_downloads_prev>-1) && (sab_downloads > sab_downloads_prev)){
    // New download detected
    vibes_short_pulse();
  } else if (sab_downloads < sab_downloads_prev){
    // Download complete/aborted
    vibes_double_pulse();
  }
  sab_downloads_prev = sab_downloads;
  
  if (sab_downloads>0){
    // Show header
    if (layer_get_hidden(bitmap_layer_get_layer(s_downloads_icon_layer))){
      layer_set_hidden(bitmap_layer_get_layer(s_downloads_icon_layer), false);
      layer_set_hidden(bitmap_layer_get_layer(s_speed_icon_layer), false);
    }
    text_layer_set_text(s_speed_layer, sab_speed);
    text_layer_set_text(s_downloads_layer, sab_downloads_txt);
  } else if (!layer_get_hidden(bitmap_layer_get_layer(s_downloads_icon_layer))){
    // Hide header
    layer_set_hidden(bitmap_layer_get_layer(s_downloads_icon_layer), true);
    layer_set_hidden(bitmap_layer_get_layer(s_speed_icon_layer), true);
    text_layer_set_text(s_downloads_layer, "");
    text_layer_set_text(s_speed_layer, "");
  }
  
  
  if (sab_mb_left > 0){
    text_layer_set_text(s_time_left_layer, sab_time_left);
    if (layer_get_hidden(s_progress_layer)) {
      layer_set_hidden(s_progress_layer, false);
    } else {
      layer_mark_dirty(s_progress_layer);
    }
  } else if (!layer_get_hidden(s_progress_layer)) {
    layer_set_hidden(s_progress_layer, true);
    text_layer_set_text(s_time_left_layer, "");
  }
  
}

// Communication

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
 // Read first item
  Tuple *t = dict_read_first(iterator);
  
  bool configReceived = false;
  
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
      snprintf(sab_downloads_txt, sizeof(sab_downloads_txt), "%d", sab_downloads);
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
    case CONFIG_SAB_USE_SSL:
      config_sab_use_ssl = t->value->int8;
      break;
    case CONFIG_SAB_HOST:
      snprintf(config_sab_host, sizeof(config_sab_host), "%s", t->value->cstring);
      break;
    case CONFIG_SAB_PORT:
      config_sab_port = t->value->int16;
      break;
    case CONFIG_SAB_APIKEY:
      snprintf(config_sab_apikey, sizeof(config_sab_apikey), "%s", t->value->cstring);
      break;
    case CONFIG_SAB_USERNAME:
      snprintf(config_sab_username, sizeof(config_sab_username), "%s", t->value->cstring);
      break;
    case CONFIG_SAB_PASSWORD:
      snprintf(config_sab_password, sizeof(config_sab_password), "%s", t->value->cstring);
      break;
    case CONFIG_WATCH_VIB_NEW:
      config_vibrate_new = t->value->int8;
      break;
    case CONFIG_WATCH_VIB_FIN:
      config_vibrate_finish = t->value->int8;
      break;
    case CONFIG_WATCH_INT_IDLE:
      config_interval_idle = t->value->int16;
      break;
    case CONFIG_WATCH_INT_ACTIVE:
      config_interval_active = t->value->int16;
      break;
    
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }
    
    // Look for next item
    t = dict_read_next(iterator);
  }
  
  if (configReceived){
    // TODO: Reset screen
  } else {
    updateDataOnScreen();
  }
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
  
  // Add Info layer
  s_info_layer = text_layer_create(GRect(0, 15, 144, 22));
  text_layer_set_text(s_info_layer, "");
  text_layer_set_font(s_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_info_layer, GTextAlignmentCenter);
  text_layer_set_text_color(s_info_layer, GColorWhite);
  text_layer_set_background_color(s_info_layer, GColorClear);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_info_layer));
  
  // Load speacial font
  my_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TELEMANS_CAPTURE_IT_48));
  
  // Create time layer
  s_time_layer = text_layer_create(GRect(0, 44, 144, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "--:--");
  
  // Format time layer
  text_layer_set_font(s_time_layer, my_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Add time layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
 
  // Add Progress Bar
  s_progress_layer = layer_create(GRect(10, 125, 120, 8));
  layer_set_hidden(s_progress_layer, true);
  layer_set_update_proc(s_progress_layer, update_progressbar);
  layer_add_child(window_get_root_layer(window), s_progress_layer);

  // Add Speed icon
  icon_speed = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TELEMANS_SPEED_ICON);
  s_speed_icon_layer = bitmap_layer_create(GRect(123, 12, 16, 16));
  layer_set_hidden(bitmap_layer_get_layer(s_speed_icon_layer), true);
  bitmap_layer_set_bitmap(s_speed_icon_layer, icon_speed);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_speed_icon_layer));

  // Add Speed text
  s_speed_layer = text_layer_create(GRect(55, 6, 64, 18));
  text_layer_set_text(s_speed_layer, "");
  text_layer_set_font(s_speed_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_speed_layer, GTextAlignmentRight);
  text_layer_set_text_color(s_speed_layer, GColorWhite);
  text_layer_set_background_color(s_speed_layer, GColorClear);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_speed_layer));

  // Add Downloads icon
  icon_downloads = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TELEMANS_DNLOAD_ICON);
  s_downloads_icon_layer = bitmap_layer_create(GRect(5, 12, 16, 16));
  layer_set_hidden(bitmap_layer_get_layer(s_downloads_icon_layer), true);
  bitmap_layer_set_bitmap(s_downloads_icon_layer, icon_downloads);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_downloads_icon_layer));

  // Add Downloads text
  s_downloads_layer = text_layer_create(GRect(25, 6, 30, 18));
  text_layer_set_text(s_downloads_layer, "");
  text_layer_set_font(s_downloads_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_downloads_layer, GTextAlignmentLeft);
  text_layer_set_text_color(s_downloads_layer, GColorWhite);
  text_layer_set_background_color(s_downloads_layer, GColorClear);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_downloads_layer));

  // Add Time Left Layer
  s_time_left_layer = text_layer_create(GRect(10, 135, 120, 18));
  text_layer_set_text(s_time_left_layer, "");
  text_layer_set_font(s_time_left_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_time_left_layer, GTextAlignmentCenter);
  text_layer_set_text_color(s_time_left_layer, GColorWhite);
  text_layer_set_background_color(s_time_left_layer, GColorClear);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_left_layer));
 
  APP_LOG(APP_LOG_LEVEL_INFO, "Init complete!");
  
}

static void main_window_unload(Window *window) {
  bitmap_layer_destroy(s_background_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_time_left_layer);
  text_layer_destroy(s_downloads_layer);
  bitmap_layer_destroy(s_downloads_icon_layer);
  text_layer_destroy(s_speed_layer);
  bitmap_layer_destroy(s_speed_icon_layer);
  layer_destroy(s_progress_layer);
  fonts_unload_custom_font(my_font);
}

void handle_init(void) {
  read_configuration();
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
  
  // Send Config to Phone
  send_configuration();
}

void handle_deinit(void) {
  save_configuration();
  window_destroy(my_window);
}


int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
