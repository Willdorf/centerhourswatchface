#include <pebble.h>
#include <stdio.h>

#define KEY_BACKGROUND_COLOR 0
#define KEY_MINUTES_COLOR 1
#define KEY_TWENTY_FOUR_HOUR_FORMAT 2

static Window *window;
static Layer *s_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;

static GColor background_color;
static GColor minutes_color;

static Layer *s_bluetooth_icon_layer;
static bool s_bluetooth_connected;

static uint8_t s_hour;
static uint8_t s_min;
static uint8_t s_sec;

static bool twenty_four_hour_format = false;

static const GPathInfo CENTERBLOCK_INFO = {
	.num_points = 36,
	.points = (GPoint []) {
		{52, 64},{71, 64},{71, 33},{67, 33},{67, 32},{76, 32},{76, 33},{72, 33},{72, 64},{91, 64},
		{91, 83},{122, 83},{122, 79},{123, 79},{123, 88},{122, 88},{122, 84},{91, 84},{91, 103},{72, 103},
		{72, 134},{76, 134},{76, 135},{67, 135},{67, 134},{71, 134},{71, 103},{52, 103},{52, 84},{21, 84},
		{21, 88},{20, 88},{20, 79},{21, 79},{21, 83},{52, 83}}
};

static GPath *center_block = NULL;

static const GPathInfo HORZ_MARKER_LEFT_INFO = {
	.num_points = 2,
	.points = (GPoint []) {{60,43},{65, 43}}
};

static GPath *horz_marker_left = NULL;

static const GPathInfo HORZ_MARKER_CENTER_INFO = {
	.num_points = 2,
	.points = (GPoint []) {{70, 43},{73, 43}}
};

static GPath *horz_marker_center = NULL;

static const GPathInfo HORZ_MARKER_RIGHT_INFO = {
	.num_points = 2,
	.points = (GPoint []) {{78, 43},{83, 43}}
};

static GPath *horz_marker_right = NULL;

static const GPathInfo VERT_MARKER_TOP = {
	.num_points = 2,
	.points = (GPoint []) {{31, 72},{31, 77}}
};

static GPath *vert_marker_top = NULL;

static const GPathInfo VERT_MARKER_MIDDLE = {
	.num_points = 2,
	.points = (GPoint []) {{31, 82},{31, 85}}
};

static GPath *vert_marker_middle = NULL;

static const GPathInfo VERT_MARKER_BOTTOM = {
	.num_points = 2,
	.points = (GPoint []) {{31, 90},{31, 95}}
};

static GPath *vert_marker_bottom = NULL;

static const GPathInfo BLUETOOTH_INFO = {
	.num_points = 9,
	.points = (GPoint []) {{3,26},{26,3},{14,15},{14,26},{20,20},{9,9},{14,14},{14,3},{20,9}}
};

GPath *bluetooth_path = NULL;

static void bluetooth_callback(bool connected) {
	
	//show icon if disconnected
	if (!connected) {
		//issue a vibrating alert
		vibes_double_pulse();
	}
	
	s_bluetooth_connected = connected;
	layer_mark_dirty(s_bluetooth_icon_layer);
}

static void bluetooth_update_proc(Layer *layer, GContext *ctx) {
	if (!s_bluetooth_connected) {
		graphics_context_set_stroke_width(ctx, 3);
		graphics_context_set_stroke_color(ctx, gcolor_legible_over(background_color));
		gpath_draw_outline(ctx, bluetooth_path);
	}
}

static void update_time(struct tm *tick_time) {
	s_hour = tick_time->tm_hour;
	s_min = tick_time->tm_min;
	s_sec = tick_time->tm_sec;

	static char buffer[] = "00";

	if (twenty_four_hour_format == true) {
		//update hour
		strftime(buffer, sizeof("00"), "%H", tick_time);
		text_layer_set_text(s_time_layer, buffer);
	} else {
		//update hour
		strftime(buffer, sizeof("00"), "%I", tick_time);
		text_layer_set_text(s_time_layer, buffer);
	}

	//update the date using localized format
	text_layer_set_text_color(s_date_layer, gcolor_legible_over(background_color));
	static char date_buffer[20];
	strftime(date_buffer, sizeof(date_buffer), "%x", tick_time);
	text_layer_set_text(s_date_layer, date_buffer);

	layer_mark_dirty(s_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time(tick_time);
}

static void draw_watchface(Layer *layer, GContext *ctx) {
	graphics_context_set_fill_color(ctx, COLOR_FALLBACK(minutes_color, GColorOrange));

	//draw minutes 
	if (s_min > 0 && s_min < 15) {
		graphics_fill_rect(ctx, GRect(67,34 + (2*s_min) - 2,10,2), 0,0);
	} else if (s_min >= 15) {
		graphics_fill_rect(ctx, GRect(52,65, 40, 10), 0, 0);
	}

	if (s_min > 15 && s_min < 30) {
		uint8_t cur_min = s_min - 15;
		graphics_fill_rect(ctx, GRect(120 - (2*cur_min) + 2,79,2,10), 0,0);
	} else if (s_min >= 30) {
		graphics_fill_rect(ctx, GRect(82,64, 10, 40), 0, 0);
	}

	if (s_min > 30 && s_min < 45) {
		uint8_t cur_min = s_min - 30;
		graphics_fill_rect(ctx, GRect(67, 132 - (2*cur_min) + 2, 10, 2), 0, 0);
	} else if (s_min >= 45) {
		graphics_fill_rect(ctx, GRect(52, 94, 40, 10), 0, 0);
	}

	if (s_min > 45 && s_min < 60) {
		uint8_t cur_min = s_min - 45;
		graphics_fill_rect(ctx, GRect(22 + (2*cur_min) - 2, 79, 2, 10), 0, 0);
	} 

	graphics_context_set_stroke_color(ctx, gcolor_legible_over(background_color));
	//draw center block
	gpath_draw_outline(ctx, center_block);

	//HORZ markers------------------------------
	gpath_move_to(horz_marker_left, GPoint(0, 0));
	gpath_move_to(horz_marker_center, GPoint(0, 0));
	gpath_move_to(horz_marker_right, GPoint(0, 0));
	gpath_draw_outline(ctx, horz_marker_left);
	gpath_draw_outline(ctx, horz_marker_center);
	gpath_draw_outline(ctx, horz_marker_right);

	//move down and draw next marker
	gpath_move_to(horz_marker_left, GPoint(0, 10));
	gpath_move_to(horz_marker_center, GPoint(0, 10));
	gpath_move_to(horz_marker_right, GPoint(0, 10));
	gpath_draw_outline(ctx, horz_marker_left);
	gpath_draw_outline(ctx, horz_marker_center);
	gpath_draw_outline(ctx, horz_marker_right);

	//move down and draw next marker
	gpath_move_to(horz_marker_left, GPoint(0, 71));
	gpath_move_to(horz_marker_center, GPoint(0, 71));
	gpath_move_to(horz_marker_right, GPoint(0, 71));
	gpath_draw_outline(ctx, horz_marker_left);
	gpath_draw_outline(ctx, horz_marker_center);
	gpath_draw_outline(ctx, horz_marker_right);

	//move down and draw next marker
	gpath_move_to(horz_marker_left, GPoint(0, 81));
	gpath_move_to(horz_marker_center, GPoint(0, 81));
	gpath_move_to(horz_marker_right, GPoint(0, 81));
	gpath_draw_outline(ctx, horz_marker_left);
	gpath_draw_outline(ctx, horz_marker_center);
	gpath_draw_outline(ctx, horz_marker_right);

	//VERT markers------------------------------
	gpath_move_to(vert_marker_top, GPoint(0, 0));
	gpath_move_to(vert_marker_middle, GPoint(0, 0));
	gpath_move_to(vert_marker_bottom, GPoint(0, 0));
	gpath_draw_outline(ctx, vert_marker_top);
	gpath_draw_outline(ctx, vert_marker_middle);
	gpath_draw_outline(ctx, vert_marker_bottom);

	gpath_move_to(vert_marker_top, GPoint(10, 0));
	gpath_move_to(vert_marker_middle, GPoint(10, 0));
	gpath_move_to(vert_marker_bottom, GPoint(10, 0));
	gpath_draw_outline(ctx, vert_marker_top);
	gpath_draw_outline(ctx, vert_marker_middle);
	gpath_draw_outline(ctx, vert_marker_bottom);

	gpath_move_to(vert_marker_top, GPoint(71, 0));
	gpath_move_to(vert_marker_middle, GPoint(71, 0));
	gpath_move_to(vert_marker_bottom, GPoint(71, 0));
	gpath_draw_outline(ctx, vert_marker_top);
	gpath_draw_outline(ctx, vert_marker_middle);
	gpath_draw_outline(ctx, vert_marker_bottom);

	gpath_move_to(vert_marker_top, GPoint(81, 0));
	gpath_move_to(vert_marker_middle, GPoint(81, 0));
	gpath_move_to(vert_marker_bottom, GPoint(81, 0));
	gpath_draw_outline(ctx, vert_marker_top);
	gpath_draw_outline(ctx, vert_marker_middle);
	gpath_draw_outline(ctx, vert_marker_bottom);
}

static void setup_paths() {
	//eleven_path = gpath_create(&ELEVEN_INFO);
	center_block = gpath_create(&CENTERBLOCK_INFO);

	//horz marker 
	horz_marker_left = gpath_create(&HORZ_MARKER_LEFT_INFO);
	horz_marker_center = gpath_create(&HORZ_MARKER_CENTER_INFO);
	horz_marker_right = gpath_create(&HORZ_MARKER_RIGHT_INFO);

	//vert marker
	vert_marker_top = gpath_create(&VERT_MARKER_TOP);
	vert_marker_middle = gpath_create(&VERT_MARKER_MIDDLE);
	vert_marker_bottom = gpath_create(&VERT_MARKER_BOTTOM);
}

static void set_background_color(int color) {
	background_color = GColorFromHEX(color);
	window_set_background_color(window, background_color);
	text_layer_set_text_color(s_time_layer, gcolor_legible_over(background_color));
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "inbox received handler");
	Tuple *background_color_t = dict_find(iter, KEY_BACKGROUND_COLOR);
	Tuple *minutes_color_t = dict_find(iter, KEY_MINUTES_COLOR);
	Tuple *twenty_four_hour_format_t = dict_find(iter, KEY_TWENTY_FOUR_HOUR_FORMAT);

	if (twenty_four_hour_format_t) {
		twenty_four_hour_format = twenty_four_hour_format_t->value->int8;
		persist_write_int(KEY_TWENTY_FOUR_HOUR_FORMAT, twenty_four_hour_format);
	}

	if (background_color_t) {
		int background_color = background_color_t->value->int32;

		if (background_color == 0) { //quick fix so that black colour persists
			background_color++;
		}
		persist_write_int(KEY_BACKGROUND_COLOR, background_color);

		set_background_color(background_color);

		APP_LOG(APP_LOG_LEVEL_DEBUG, "background color: %d", background_color);
	}

	if (minutes_color_t) {
		int mc = minutes_color_t->value->int32;
		if (mc == 0) { //quick fix so that black colour persists
			mc++;
		}
		persist_write_int(KEY_MINUTES_COLOR, mc);
		minutes_color = GColorFromHEX(mc);

		APP_LOG(APP_LOG_LEVEL_DEBUG, "minutes color: %d", mc);
	}

	//display the time right away
	time_t start_time = time(NULL);
	update_time(localtime(&start_time));
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	s_layer = layer_create(layer_get_bounds(window_get_root_layer(window)));
	layer_add_child(window_get_root_layer(window), s_layer);
	layer_set_update_proc(s_layer, draw_watchface);

	window_set_background_color(window, GColorDarkGray);

	setup_paths();

	//text layer for displaying minutes in center
	s_time_layer = text_layer_create(GRect(63, 75, 19, 19));
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorWhite);
	text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

	if (persist_read_int(KEY_BACKGROUND_COLOR)) {
		set_background_color(persist_read_int(KEY_BACKGROUND_COLOR));
		APP_LOG(APP_LOG_LEVEL_DEBUG, "background color int: %d", (int) persist_read_int(KEY_BACKGROUND_COLOR));
		APP_LOG(APP_LOG_LEVEL_DEBUG, "background color hex: %x", (int) persist_read_int(KEY_BACKGROUND_COLOR));
	} else {
		background_color = GColorDarkGray;
	}

	if (persist_read_int(KEY_MINUTES_COLOR)) {
		minutes_color = GColorFromHEX(persist_read_int(KEY_MINUTES_COLOR));
	} else {
		minutes_color = GColorWindsorTan;
	}

	if (persist_read_bool(KEY_TWENTY_FOUR_HOUR_FORMAT)) {
		twenty_four_hour_format = persist_read_bool(KEY_TWENTY_FOUR_HOUR_FORMAT);
	}

	s_bluetooth_icon_layer = layer_create(GRect(0,0,30,30));
	layer_set_update_proc(s_bluetooth_icon_layer, bluetooth_update_proc);
	bluetooth_path = gpath_create(&BLUETOOTH_INFO);
	layer_add_child(window_get_root_layer(window), s_bluetooth_icon_layer);

	//show the correct state of the bluetooth connection from the start
#ifdef PBL_SDK_2
	bluetooth_callback(bluetooth_connection_service_peek());
#elif PBL_SDK_3
	bluetooth_callback(connection_service_peek_pebble_app_connection());
#endif

	s_date_layer = text_layer_create(GRect(0,0,144,14));
	text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_color(s_date_layer, gcolor_legible_over(background_color));
	text_layer_set_background_color(s_date_layer, GColorClear);
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

}

static void window_unload(Window *window) {
	text_layer_destroy(s_time_layer);

	//destroy the main layer
	layer_destroy(s_layer);

	//destroy the bluetooth stuffs
	layer_destroy(s_bluetooth_icon_layer);
	gpath_destroy(bluetooth_path);

	//destroy the date layer
	text_layer_destroy(s_date_layer);

	//destroy everything!!
	gpath_destroy(horz_marker_left);
	gpath_destroy(horz_marker_center);
	gpath_destroy(horz_marker_right);

	gpath_destroy(vert_marker_top);
	gpath_destroy(vert_marker_bottom);
	gpath_destroy(vert_marker_middle);

	gpath_destroy(center_block);
}

static void init(void) {
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
			.load = window_load,
			.unload = window_unload,
			});
	const bool animated = true;
	window_stack_push(window, animated);

	//Register with TickTimerService
	tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

	app_message_register_inbox_received(inbox_received_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

	//Register for Bluetooth connections updates
#ifdef PBL_SDK_2
	bluetooth_connection_service_subscribe(bluetooth_callback);
#elif PBL_SDK_3
	connection_service_subscribe((ConnectionHandlers) {
			.pebble_app_connection_handler = bluetooth_callback
			});
#endif

	//display the time right away
	time_t start_time = time(NULL);
	update_time(localtime(&start_time));
}

static void deinit(void) {
	window_destroy(window);
	tick_timer_service_unsubscribe();
}

int main(void) {
	init();

	APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

	app_event_loop();
	deinit();
}
