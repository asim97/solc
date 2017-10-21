#ifndef MENU_H
#define MENU_H

#include <stdint.h>


typedef enum
{
  MENU_EVENT_CLICK,
  MENU_EVENT_HOLD,
  MENU_EVENT_RELEASE,
  MENU_EVENT_TIMER,
  MENU_EVENT_APP,
} menu_event_type_t;

enum
{
 MENU_BUTTON_ID_NAV,
 MENU_BUTTON_ID_SELECT,
};


typedef struct menu_t menu_t;

typedef struct
{
  menu_event_type_t type;
  int id;
} menu_event_t;

typedef enum
{
  PARAM_TYPE_TIME,
  PARAM_TYPE_INT,
  PARAM_TYPE_STR,
  PARAM_TYPE_SELECTION,
} menu_param_type_t;


typedef struct menu_item_t menu_item_t;

enum
{
  MENU_ITEM_EDITABLE = 1,
};

typedef void (*menu_item_entry_handler_t)(menu_t *m, const menu_item_t *item);
typedef void (*menu_item_event_handler_t)(menu_t *m, const menu_item_t *item, const menu_event_t *evt);
typedef void (*menu_item_draw_handler_t)(menu_t *m, const menu_item_t *item);


typedef struct
{
  menu_item_entry_handler_t entry;
  menu_item_event_handler_t event;
  menu_item_draw_handler_t draw;
  uint8_t subItemStart;
  uint8_t subItems;
  uint8_t subItemChars;
  uint8_t subItemDistance;
} menu_item_class_t;




extern const menu_item_class_t menuItemTime;
extern const menu_item_class_t menuItemInt;
extern const menu_item_class_t menuItemSelect;
extern const menu_item_class_t menuItemStr;
extern const menu_item_class_t menuItemSubMenu;

typedef void (*menu_item_confirm_callback_t)(menu_t *m, const menu_item_t *self);

struct menu_item_t
{
  const char *name;
  const menu_item_class_t *ic;
  uint32_t flags;
  const void *subItems;
  void *storage;
  menu_item_confirm_callback_t cfmCallback;
};


struct menu_t
{
  const menu_item_t *item_list;
  const menu_item_t *item;
  int editIndex;
  int state;
  volatile uint8_t update;
  uint8_t blinkState;
  char is[17];
};

void menu_update(menu_t *self);
void menu_blink(menu_t *self);
void menu_init(menu_t *self, const menu_item_t *first);
void menu_event(menu_t *self, const menu_event_t *evt);
void menu_draw(menu_t *self);



#endif
