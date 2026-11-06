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
    lv_obj_t *fl_tire;
    lv_obj_t *fl_tire__obj0;
    lv_obj_t *fl_tire__obj1;
    lv_obj_t *fl_tire__obj2;
    lv_obj_t *fl_tire__obj3;
    lv_obj_t *fl_tire__obj4;
    lv_obj_t *fl_tire__obj5;
    lv_obj_t *fl_tire__obj6;
    lv_obj_t *fl_tire__obj7;
    lv_obj_t *fl_tire__obj8;
    lv_obj_t *fl_tire__obj9;
    lv_obj_t *fl_tire__obj10;
    lv_obj_t *fr_tire;
    lv_obj_t *fr_tire__obj0;
    lv_obj_t *fr_tire__obj1;
    lv_obj_t *fr_tire__obj2;
    lv_obj_t *fr_tire__obj3;
    lv_obj_t *fr_tire__obj4;
    lv_obj_t *fr_tire__obj5;
    lv_obj_t *fr_tire__obj6;
    lv_obj_t *fr_tire__obj7;
    lv_obj_t *fr_tire__obj8;
    lv_obj_t *fr_tire__obj9;
    lv_obj_t *fr_tire__obj10;
    lv_obj_t *rl_tire;
    lv_obj_t *rl_tire__obj0;
    lv_obj_t *rl_tire__obj1;
    lv_obj_t *rl_tire__obj2;
    lv_obj_t *rl_tire__obj3;
    lv_obj_t *rl_tire__obj4;
    lv_obj_t *rl_tire__obj5;
    lv_obj_t *rl_tire__obj6;
    lv_obj_t *rl_tire__obj7;
    lv_obj_t *rl_tire__obj8;
    lv_obj_t *rl_tire__obj9;
    lv_obj_t *rl_tire__obj10;
    lv_obj_t *rr_tire;
    lv_obj_t *rr_tire__obj0;
    lv_obj_t *rr_tire__obj1;
    lv_obj_t *rr_tire__obj2;
    lv_obj_t *rr_tire__obj3;
    lv_obj_t *rr_tire__obj4;
    lv_obj_t *rr_tire__obj5;
    lv_obj_t *rr_tire__obj6;
    lv_obj_t *rr_tire__obj7;
    lv_obj_t *rr_tire__obj8;
    lv_obj_t *rr_tire__obj9;
    lv_obj_t *rr_tire__obj10;
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