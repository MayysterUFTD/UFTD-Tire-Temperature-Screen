#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    _SCREEN_ID_LAST = 1
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *obj0;
    lv_obj_t *obj0__obj0;
    lv_obj_t *obj0__obj1;
    lv_obj_t *obj0__obj2;
    lv_obj_t *obj0__obj3;
    lv_obj_t *obj0__obj4;
    lv_obj_t *obj0__obj5;
    lv_obj_t *obj0__obj6;
    lv_obj_t *obj0__obj7;
    lv_obj_t *obj0__obj8;
    lv_obj_t *obj0__obj9;
    lv_obj_t *obj0__obj10;
    lv_obj_t *obj1;
    lv_obj_t *obj1__obj0;
    lv_obj_t *obj1__obj1;
    lv_obj_t *obj1__obj2;
    lv_obj_t *obj1__obj3;
    lv_obj_t *obj1__obj4;
    lv_obj_t *obj1__obj5;
    lv_obj_t *obj1__obj6;
    lv_obj_t *obj1__obj7;
    lv_obj_t *obj1__obj8;
    lv_obj_t *obj1__obj9;
    lv_obj_t *obj1__obj10;
    lv_obj_t *obj2;
    lv_obj_t *obj2__obj0;
    lv_obj_t *obj2__obj1;
    lv_obj_t *obj2__obj2;
    lv_obj_t *obj2__obj3;
    lv_obj_t *obj2__obj4;
    lv_obj_t *obj2__obj5;
    lv_obj_t *obj2__obj6;
    lv_obj_t *obj2__obj7;
    lv_obj_t *obj2__obj8;
    lv_obj_t *obj2__obj9;
    lv_obj_t *obj2__obj10;
    lv_obj_t *obj3;
    lv_obj_t *obj3__obj0;
    lv_obj_t *obj3__obj1;
    lv_obj_t *obj3__obj2;
    lv_obj_t *obj3__obj3;
    lv_obj_t *obj3__obj4;
    lv_obj_t *obj3__obj5;
    lv_obj_t *obj3__obj6;
    lv_obj_t *obj3__obj7;
    lv_obj_t *obj3__obj8;
    lv_obj_t *obj3__obj9;
    lv_obj_t *obj3__obj10;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void create_user_widget_tire(lv_obj_t *parent_obj, int startWidgetIndex);
void tick_user_widget_tire(int startWidgetIndex);

void create_user_widget_tire_max(lv_obj_t *parent_obj, int startWidgetIndex);
void tick_user_widget_tire_max(int startWidgetIndex);

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/