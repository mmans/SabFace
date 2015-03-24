/*
 TODO:
   * Settings --> Struct
   * Vib-settings
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
#define CONFIG_SETTINGS 99
  
#define CMD_REQUEST_UPDATE 1
#define CMD_SEND_CONFIG 2
  
typedef struct sab_settings_struct{
  bool config_sab_use_ssl;
  char config_sab_host[50];
  int config_sab_port;
  char config_sab_apikey[35];
  char config_sab_username[25];
  char config_sab_password[25];
  bool config_vibrate_new;
  bool config_vibrate_finish;
  int config_interval_idle;
  int config_interval_active;
} __attribute__((__packed__)) sab_settings_struct;


sab_settings_struct sab_settings;

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
BitmapLayer *s_refresh_layer;
BitmapLayer *s_no_connection_layer;

GFont *my_font;
GBitmap *my_background;
GBitmap *icon_downloads;
GBitmap *icon_speed;
GBitmap *refresh_icon;
GBitmap *no_connection_icon;

static bool sab_gui_visible = false;
static bool sab_valid_connection_prev = false;
static bool sab_valid_connection = false;
static uint32_t sab_mb_total = 0;
static uint32_t sab_mb_left = 0;
static int sab_downloads_prev = -1;
static int sab_downloads = 0;
static char sab_downloads_txt[3] = "000";
static char sab_time_left[8] = "00:00:00";
static char sab_speed[8] = "";
static int sab_proc_left = 0;
static int sab_current_interval = 30;

char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

// ----------------------------------------------------------------------------------------
// ---- CONFIGURATION
// ----------------------------------------------------------------------------------------
void read_configuration(){
  //APP_LOG(APP_LOG_LEVEL_INFO, "Reading configuration");
  
  if (persist_exists(CONFIG_SETTINGS)){
    persist_read_data(CONFIG_SETTINGS, &sab_settings, sizeof(sab_settings));
  } else {
    // Set Defaults
    sab_settings.config_sab_use_ssl = false,
    strcpy(sab_settings.config_sab_host, "my.sabnzb.server");
    sab_settings.config_sab_port = 80;
    strcpy(sab_settings.config_sab_apikey, "");
    strcpy(sab_settings.config_sab_username, "");
    strcpy(sab_settings.config_sab_password, "");
    sab_settings.config_vibrate_new = true;
    sab_settings.config_vibrate_finish = true;
    sab_settings.config_interval_idle = 900;
    sab_settings.config_interval_active= 5;
  }
}

void send_configuration(){
    //APP_LOG(APP_LOG_LEVEL_INFO, "Sending configuration to phone");
  
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, CMD_TYPE, CMD_SEND_CONFIG);
    dict_write_int8(iter, CONFIG_SAB_USE_SSL, sab_settings.config_sab_use_ssl);
    dict_write_cstring(iter, CONFIG_SAB_HOST, sab_settings.config_sab_host);
    dict_write_int16(iter, CONFIG_SAB_PORT, sab_settings.config_sab_port);
    dict_write_cstring(iter, CONFIG_SAB_APIKEY, sab_settings.config_sab_apikey);
    dict_write_cstring(iter, CONFIG_SAB_USERNAME, sab_settings.config_sab_username);
    dict_write_cstring(iter, CONFIG_SAB_PASSWORD, sab_settings.config_sab_password);
    dict_write_int8(iter, CONFIG_WATCH_VIB_NEW, sab_settings.config_vibrate_new);
    dict_write_int8(iter, CONFIG_WATCH_VIB_FIN, sab_settings.config_vibrate_finish);
    dict_write_int16(iter, CONFIG_WATCH_INT_IDLE, sab_settings.config_interval_idle);
    dict_write_int16(iter, CONFIG_WATCH_INT_ACTIVE, sab_settings.config_interval_active);
  
    // Send the message!
    app_message_outbox_send();  
}

void save_configuration(){
  persist_write_data(CONFIG_SETTINGS, &sab_settings, sizeof(sab_settings));
}

// ----------------------------------------------------------------------------------------
// ---- COMMANDS
// ----------------------------------------------------------------------------------------
void request_update(){
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, CMD_TYPE, CMD_REQUEST_UPDATE);

    // Send the message!
    app_message_outbox_send();
}

// ----------------------------------------------------------------------------------------
// ---- UI
// ----------------------------------------------------------------------------------------

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

void showSabGUI(){
  if (!sab_gui_visible){
    layer_set_hidden(bitmap_layer_get_layer(s_downloads_icon_layer), false);
    layer_set_hidden(bitmap_layer_get_layer(s_speed_icon_layer), false);
    layer_set_hidden(s_progress_layer, false);
    sab_gui_visible = true;
  }
}

void hideSabGui(){
  if (sab_gui_visible){
    layer_set_hidden(bitmap_layer_get_layer(s_downloads_icon_layer), true);
    layer_set_hidden(bitmap_layer_get_layer(s_speed_icon_layer), true);
    layer_set_hidden(s_progress_layer, true);
    text_layer_set_text(s_downloads_layer, "");
    text_layer_set_text(s_speed_layer, "");
    text_layer_set_text(s_time_left_layer, "");
    sab_gui_visible = false;
  }
}

static void updateDataOnScreen(){
  //APP_LOG(APP_LOG_LEVEL_INFO, "Updating screen");
  layer_set_hidden(bitmap_layer_get_layer(s_refresh_layer), true);
  
  if (sab_downloads_prev>-1){
    if ((sab_settings.config_vibrate_new) && (sab_downloads > sab_downloads_prev)){
      // New download detected
      vibes_short_pulse();
    } else if ((sab_settings.config_vibrate_finish) && (sab_downloads < sab_downloads_prev)){
      // Download complete/aborted
      vibes_double_pulse();
    }
  }
  
  if ((sab_downloads>0) && (sab_downloads_prev<=0)){
    // Changing from IDLE to ACTIVE
    sab_current_interval = sab_settings.config_interval_active;
    showSabGUI();
  } else if ((sab_downloads==0) && (sab_downloads_prev>0)){
    // Changing from ACTIVE to IDLE
    sab_current_interval = sab_settings.config_interval_idle;
    hideSabGui();
  }
  
  sab_downloads_prev = sab_downloads;
  
  if (sab_downloads>0){
    // Show header
    text_layer_set_text(s_speed_layer, sab_speed);
    text_layer_set_text(s_downloads_layer, sab_downloads_txt);
    text_layer_set_text(s_time_left_layer, sab_time_left);
    layer_mark_dirty(s_progress_layer);
  }
}

// TAPPING
static void tap_handler(AccelAxisType axis, int32_t direction) {
  // Update data
  if (!sab_gui_visible && sab_valid_connection){
    layer_set_hidden(bitmap_layer_get_layer(s_refresh_layer), false);
    request_update();
  }
}

// ----------------------------------------------------------------------------------------
// ---- TICKER
// ----------------------------------------------------------------------------------------
static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  if (init || (units_changed & MINUTE_UNIT)) {
    update_time(tick_time);
    init = false;
  }
  
  static int tick_counter = 0;
  tick_counter++;

  if (tick_counter>sab_current_interval){
    // Request update
    tick_counter=0;
    request_update();
  }
}


// ----------------------------------------------------------------------------------------
// ---- APP MESSAGE HANDLING
// ----------------------------------------------------------------------------------------
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  //APP_LOG(APP_LOG_LEVEL_INFO, "Processing incomming app message!");
 // Read first item
  Tuple *t = dict_read_first(iterator);
  
  bool configReceived = false;
  
  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_VALID_CONNECTION: 
      sab_valid_connection = t->value->int8;
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
      sab_settings.config_sab_use_ssl = t->value->int8;
      break;
    case CONFIG_SAB_HOST:
      snprintf(sab_settings.config_sab_host, sizeof(sab_settings.config_sab_host), "%s", t->value->cstring);
      break;
    case CONFIG_SAB_PORT:
      sab_settings.config_sab_port = t->value->int16;
      break;
    case CONFIG_SAB_APIKEY:
      snprintf(sab_settings.config_sab_apikey, sizeof(sab_settings.config_sab_apikey), "%s", t->value->cstring);
      break;
    case CONFIG_SAB_USERNAME:
      snprintf(sab_settings.config_sab_username, sizeof(sab_settings.config_sab_username), "%s", t->value->cstring);
      break;
    case CONFIG_SAB_PASSWORD:
      snprintf(sab_settings.config_sab_password, sizeof(sab_settings.config_sab_password), "%s", t->value->cstring);
      break;
    case CONFIG_WATCH_VIB_NEW:
      sab_settings.config_vibrate_new = t->value->int8;
      break;
    case CONFIG_WATCH_VIB_FIN:
      sab_settings.config_vibrate_finish = t->value->int8;
      break;
    case CONFIG_WATCH_INT_IDLE:
      sab_settings.config_interval_idle = t->value->int16;
      break;
    case CONFIG_WATCH_INT_ACTIVE:
      sab_settings.config_interval_active = t->value->int16;
      configReceived = true;
      break;
    
    default:
      //APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }
    
    // Look for next item
    t = dict_read_next(iterator);
  }
  
  if (configReceived){
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Config received");
    hideSabGui();
    request_update();
  } else {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Data received (%d,%d)", sab_valid_connection, sab_valid_connection_prev);
    if (sab_valid_connection && !sab_valid_connection_prev){
      // Connection is OK now
      layer_set_hidden(bitmap_layer_get_layer(s_no_connection_layer), true);
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "Connection valid!!!");
    } else if (!sab_valid_connection){
      // Connetion id NOK now
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "Connection not valid!!!");
      hideSabGui();
      layer_set_hidden(bitmap_layer_get_layer(s_no_connection_layer), false);
    }
    
    sab_valid_connection_prev = sab_valid_connection;
    
    if (sab_valid_connection) updateDataOnScreen();
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  //APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped! (%s)", translate_error(reason));
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  //APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  //APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

// ----------------------------------------------------------------------------------------
// ---- INIT / DEINIT
// ----------------------------------------------------------------------------------------
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
 
  // Add Refresh Icon
  refresh_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TELEMANS_REFRESH);
  s_refresh_layer = bitmap_layer_create(GRect(56, 120, 32, 32));
  layer_set_hidden(bitmap_layer_get_layer(s_refresh_layer), true);
  bitmap_layer_set_bitmap(s_refresh_layer, refresh_icon);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_refresh_layer));

  // Add Refresh Icon
  no_connection_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TELEMANS_NO_CONNECTION);
  s_no_connection_layer = bitmap_layer_create(GRect(56, 120, 32, 32));
  layer_set_hidden(bitmap_layer_get_layer(s_no_connection_layer), true);
  bitmap_layer_set_bitmap(s_no_connection_layer, no_connection_icon);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_no_connection_layer));

  
  //APP_LOG(APP_LOG_LEVEL_INFO, "Init complete!");
  
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
  bitmap_layer_destroy(s_no_connection_layer);
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
  
  // Tap service
  accel_tap_service_subscribe(tap_handler);
  
  // Read and send config to Phone
  read_configuration();
  send_configuration();
}

void handle_deinit(void) {
  accel_tap_service_unsubscribe();
  app_message_deregister_callbacks();
  save_configuration();
  window_destroy(my_window);
}


int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
