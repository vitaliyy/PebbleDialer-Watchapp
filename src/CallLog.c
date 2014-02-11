#include "pebble.h"
#include "pebble_fonts.h"
#include "Dialer2.h"
#include "util.h"

Window* callLogWindow;

uint16_t numEntries = 10;

int8_t cl_arrayCenterPos = 0;
int16_t cl_centerIndex = 0;

int16_t pickedEntry = -1;
uint8_t pickedMode = 0;

bool cl_sending = false;
char cl_names[36][36] = {};
char cl_dates[21][21] = {};
uint8_t cl_types[21] = {};
char cl_numbers[21][21] = {};

MenuLayer* logMenuLayer;

GBitmap* incomingCall;
GBitmap* outgoingCall;
GBitmap* missedCall;


int8_t cl_convertToArrayPos(uint16_t index)
{
	int16_t indexDiff = index - cl_centerIndex;
	if (indexDiff > 10 || indexDiff < -10)
		return -1;

	int8_t arrayPos = cl_arrayCenterPos + indexDiff;
	if (arrayPos < 0)
		arrayPos += 21;
	if (arrayPos > 20)
		arrayPos -= 21;

	return arrayPos;
}

int8_t cl_convertToArrayPosNames(uint16_t index)
{
	int16_t indexDiff = index - cl_centerIndex;
	if (indexDiff > 10 || indexDiff < -10)
		return -1;

	int8_t arrayPos = cl_arrayCenterPos + indexDiff;
	if (arrayPos < 0)
		arrayPos += 36;
	if (arrayPos > 36)
		arrayPos -= 36;

	return arrayPos;
}

char* cl_getName(uint16_t index)
{
	int8_t arrayPos = cl_convertToArrayPosNames(index);
	if (arrayPos < 0)
		return "";

	return cl_names[arrayPos];
}

void cl_setName(uint16_t index, char *name)
{
	int8_t arrayPos = cl_convertToArrayPos(index);
	if (arrayPos < 0)
		return;

	strcpy(cl_names[arrayPos],name);
}

char* cl_getDate(uint16_t index)
{
	int8_t arrayPos = cl_convertToArrayPos(index);
	if (arrayPos < 0)
		return "";

	return cl_dates[arrayPos];
}

void cl_setDate(uint16_t index, char *name)
{
	int8_t arrayPos = cl_convertToArrayPos(index);
	if (arrayPos < 0)
		return;

	strcpy(cl_dates[arrayPos],name);
}

uint8_t cl_getType(uint16_t index)
{
	int8_t arrayPos = cl_convertToArrayPos(index);
	if (arrayPos < 0)
		return 0;

	return cl_types[arrayPos];
}

void cl_setType(uint16_t index, uint8_t type)
{
	int8_t arrayPos = cl_convertToArrayPos(index);
	if (arrayPos < 0)
		return;

	cl_types[arrayPos] = type;
}

char* cl_getNumber(uint16_t index)
{
	int8_t arrayPos = cl_convertToArrayPos(index);
	if (arrayPos < 0)
		return "";

	return cl_numbers[arrayPos];
}

void cl_setNumber(uint16_t index, char *name)
{
	int8_t arrayPos = cl_convertToArrayPos(index);
	if (arrayPos < 0)
		return;

	strcpy(cl_numbers[arrayPos],name);
}

void cl_shiftArray(int newIndex)
{
	int8_t clearIndex;

	int16_t diff = newIndex - cl_centerIndex;
	if (diff == 0)
		return;

	cl_centerIndex+= diff;
	cl_arrayCenterPos+= diff;

	if (diff > 0)
	{
		if (cl_arrayCenterPos > 36)
			cl_arrayCenterPos -= 36;

		clearIndex = cl_arrayCenterPos - 10;
		if (clearIndex < 0)
			clearIndex += 36;
	}
	else
	{
		if (cl_arrayCenterPos < 0)
			cl_arrayCenterPos += 36;

		clearIndex = cl_arrayCenterPos + 10;
		if (clearIndex > 36)
			clearIndex -= 36;
	}

	*cl_names[clearIndex] = 0;
	*cl_dates[clearIndex] = 0;
	cl_types[clearIndex] = 0;
	*cl_numbers[clearIndex] = 0;

}

uint8_t cl_getEmptySpacesDown()
{
	uint8_t spaces = 0;
	for (int i = cl_centerIndex; i <= cl_centerIndex + 10; i++)
	{
		if (i >= numEntries)
			return 10;

		if (*cl_getName(i) == 0)
		{
			break;
		}

		spaces++;
	}

	return spaces;
}

uint8_t cl_getEmptySpacesUp()
{
	uint8_t spaces = 0;
	for (int i = cl_centerIndex; i >= cl_centerIndex - 10; i--)
	{
		if (i < 0)
			return 10;

		if (*cl_getName(i) == 0)
		{
			break;
		}

		spaces++;
	}

	return spaces;
}

void cl_requestNumbers(uint16_t pos)
{
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 10);
	dict_write_uint16(iterator, 1, pos);
	app_message_outbox_send();

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);

	cl_sending = true;
}

void cl_sendpickedEntry(int16_t pos, uint8_t mode)
{
	if (cl_sending)
	{
		pickedEntry = pos;
		pickedMode = mode;
		return;
	}

	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 11);
	dict_write_int16(iterator, 1, pos);
	dict_write_uint8(iterator, 2, mode);
	app_message_outbox_send();

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
}

void requestAdditionalEntries()
{
	if (cl_sending)
		return;

	int emptyDown = cl_getEmptySpacesDown();
	int emptyUp = cl_getEmptySpacesUp();

	if (emptyDown < 6 && emptyDown <= emptyUp)
	{
		uint8_t startingIndex = cl_centerIndex + emptyDown;
		cl_requestNumbers(startingIndex);
	}
	else if (emptyUp < 6)
	{
		uint8_t startingIndex = cl_centerIndex - emptyUp;
		cl_requestNumbers(startingIndex);
	}
}

uint16_t cl_menu_get_num_sections_callback(MenuLayer *me, void *data) {
	return 1;
}

uint16_t cl_menu_get_num_rows_callback(MenuLayer *me, uint16_t section_index, void *data) {
	return numEntries;
}


int16_t cl_menu_get_row_height_callback(MenuLayer *me,  MenuIndex *cell_index, void *data) {
	return (strlen(cl_getNumber(cell_index->row)) == 0 ? 40 : 50);
}

void cl_menu_pos_changed(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context)
{
	cl_shiftArray(new_index.row);
	requestAdditionalEntries();
}


void cl_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
	bool hasNumberType = strlen(cl_getNumber(cell_index->row)) > 0;

	graphics_context_set_text_color(ctx, GColorBlack);

	graphics_draw_text(ctx, cl_getName(cell_index->row), fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(35, 0, 144 - 30, 36), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, cl_getDate(cell_index->row), fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(35, 15, 144 - 30, 15), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
	if (hasNumberType)
		graphics_draw_text(ctx, cl_getNumber(cell_index->row), fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(35, 30, 144 - 30, 20), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

	GBitmap* image;
	switch (cl_getType(cell_index->row))
	{
	case 1:
		image = incomingCall;
		break;
	case 2:
		image = outgoingCall;
		break;
	default:
		image = missedCall;
		break;
	}

	graphics_draw_bitmap_in_rect(ctx, image, GRect(3, hasNumberType? 14 : 5, 28, 28));

	//menu_cell_basic_draw(ctx, cell_layer, cl_getName(cell_index->row), cl_getDate(cell_index->row), NULL);
}


void cl_menu_select_callback(MenuLayer *me, MenuIndex *cell_index, void *data) {
	cl_sendpickedEntry(cell_index->row, 0);
}

void cl_menu_long_select_callback(MenuLayer *me, MenuIndex *cell_index, void *data) {
	if (strlen(cl_getNumber(cell_index->row)) == 0)
		return;

	cl_sendpickedEntry(cell_index->row, 1);
}

void cl_receivedEntries(DictionaryIterator* data)
{
	uint16_t offset = dict_find(data, 1)->value->uint16;
	numEntries = dict_find(data, 2)->value->uint16;

	cl_setType(offset, dict_find(data, 3)->value->uint8);
	cl_setName(offset, dict_find(data, 4)->value->cstring);
	cl_setDate(offset, dict_find(data, 5)->value->cstring);
	cl_setNumber(offset, dict_find(data, 6)->value->cstring);

	menu_layer_reload_data(logMenuLayer);
	cl_sending = false;

	if (pickedEntry >= 0)
	{
		cl_sendpickedEntry(pickedEntry, pickedMode);
		return;
	}
	requestAdditionalEntries();
}

void cl_data_received(int packetId, DictionaryIterator* data)
{
	switch (packetId)
	{
	case 4:
		cl_receivedEntries(data);
		break;

	}

}

void cl_window_load(Window *me) {
	setCurWindow(4);

	//cl_requestNumbers(0);
	pickedEntry = -1;
	pickedMode = 0;
}

void cl_window_unload(Window *me) {
	gbitmap_destroy(incomingCall);
	gbitmap_destroy(outgoingCall);
	gbitmap_destroy(missedCall);
	accel_tap_service_unsubscribe();
}

void init_call_log_window()
{
	incomingCall = gbitmap_create_with_resource(RESOURCE_ID_INCOMING_CALL);
	outgoingCall = gbitmap_create_with_resource(RESOURCE_ID_OUTGOING_CALL);
	missedCall = gbitmap_create_with_resource(RESOURCE_ID_MISSED_CALL);

	callLogWindow = window_create();

	Layer* topLayer = window_get_root_layer(callLogWindow);

	logMenuLayer = menu_layer_create(GRect(0, 0, 144, 168 - 16));

	// Set all the callbacks for the menu layer
	menu_layer_set_callbacks(logMenuLayer, NULL, (MenuLayerCallbacks){
		.get_num_sections = cl_menu_get_num_sections_callback,
				.get_num_rows = cl_menu_get_num_rows_callback,
				.get_cell_height = cl_menu_get_row_height_callback,
				.draw_row = cl_menu_draw_row_callback,
				.select_click = cl_menu_select_callback,
				.select_long_click = cl_menu_long_select_callback,
				.selection_changed = cl_menu_pos_changed
	});

	menu_layer_set_click_config_onto_window(logMenuLayer, callLogWindow);

	layer_add_child(topLayer, (Layer*) logMenuLayer);

	window_set_window_handlers(callLogWindow, (WindowHandlers){
		.appear = cl_window_load,
		.unload = cl_window_unload

	});

	window_stack_push(callLogWindow, true /* Animated */);
}
