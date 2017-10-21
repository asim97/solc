#include <stdio.h>
#include <string.h>
#include "mcal.h"
#include "menu.h"
#include "hd44780.h"



enum
{
  MENU_STATE_NAVIGATE,
  MENU_STATE_EDIT,
};


static void menu_item_edit_confirm(menu_t *m, const menu_item_t *item)
{
  m->state = MENU_STATE_NAVIGATE;
  m->editIndex = 0;
  if(item->cfmCallback != 0)
    item->cfmCallback(m, item);
}

//----------------------------------------------------------------------------
// Time
static void menu_item_time_event(menu_t *m, const menu_item_t *item, const menu_event_t *evt)
{
  RTC_time_t *t = (RTC_time_t *)item->storage;

  if(evt->type == MENU_EVENT_CLICK)
  {
    if(evt->id == MENU_BUTTON_ID_NAV)
    {
      switch(m->editIndex)
      {
        case 0 : if(++t->h == 24) t->h = 0; break;
        case 1 : if(++t->m == 60) t->m = 0; break;
        case 2 : if(++t->s == 60) t->s = 0; break;
      }
    }
    else if(evt->id == MENU_BUTTON_ID_SELECT)
    {
      if(m->editIndex < 2)
        m->editIndex++;
      else
        menu_item_edit_confirm(m, item);

    }
  }
}

static void menu_item_time_draw(menu_t *m, const menu_item_t *self)
{
  RTC_time_t *t = (RTC_time_t *)self->storage;
  sprintf(m->is, "%02d:%02d:%02d", t->h, t->m, t->s);
}

//----------------------------------------------------------------------------
// Integer
static void menu_item_int_event(menu_t *m, const menu_item_t *item, const menu_event_t *evt)
{
  int *v = (int *)item->storage;

  if(evt->type == MENU_EVENT_CLICK)
  {
    if(evt->id == MENU_BUTTON_ID_NAV)
    {
      int sint = *v;
      uint8_t digit[4];
      int i = 4;
      do {
        i--;
        digit[i] = sint%10;
        sint /= 10;
      } while(i);

      if(++digit[m->editIndex] == 10)
        digit[m->editIndex] = 0;

      i = 0;
      sint = 0;
      do {
        sint = sint * 10 + digit[i];
        i++;
      } while(i!=4);
      *v = sint;
    }
    else if(evt->id == MENU_BUTTON_ID_SELECT)
    {
      if(m->editIndex < 3)
        m->editIndex++;
      else
        menu_item_edit_confirm(m, item);

    }
  }
}

static void menu_item_int_draw(menu_t *m, const menu_item_t *self)
{
  int *v = (int *)self->storage;
  if(m->state == MENU_STATE_EDIT)
    sprintf(m->is, "%04d", *v);
  else
    sprintf(m->is, "%4d", *v);
}

//----------------------------------------------------------------------------
// String
static void menu_item_string_event(menu_t *m, const menu_item_t *item, const menu_event_t *evt)
{

}

static void menu_item_string_draw(menu_t *m, const menu_item_t *self)
{
  const char *v = (const char *)self->storage;
  sprintf(m->is, "%s", v);
}

//----------------------------------------------------------------------------
// Select
static void menu_item_select_entry(menu_t *m, const menu_item_t *self)
{
  int *v = (int *)self->storage;
  m->editIndex = *v;
}

static void menu_item_select_event(menu_t *m, const menu_item_t *item, const menu_event_t *evt)
{
 if(evt->type == MENU_EVENT_CLICK)
  {
    if(evt->id == MENU_BUTTON_ID_NAV)
    {
      const char * const *options = (const char * const *)item->subItems;
      m->editIndex++;
      if(options[m->editIndex] == 0)
        m->editIndex = 0;
    }
    else if(evt->id == MENU_BUTTON_ID_SELECT)
    {
      int *v = (int *)item->storage;
      *v = m->editIndex;
      menu_item_edit_confirm(m, item);
    }
  }
}

static void menu_item_select_draw(menu_t *m, const menu_item_t *item)
{
  int *v = (int *)item->storage;
  int cv;
  if(m->state == MENU_STATE_EDIT)
    cv = m->editIndex;
  else
    cv = *v;
  const char * const *options = (const char * const *)item->subItems;
  if(options != NULL) {
    sprintf(m->is, "%s", options[cv]);
  }
}

//----------------------------------------------------------------------------
// Sub menu

static void menu_item_sub_menu_entry(menu_t *m, const menu_item_t *item)
{
  m->item_list = (const menu_item_t*)item->subItems;
  m->item = m->item_list;
  menu_update(m);

}

static void menu_item_sub_menu_event(menu_t *m, const menu_item_t *item, const menu_event_t *evt)
{
}

static void menu_item_sub_menu_draw(menu_t *m, const menu_item_t *self)
{
  sprintf(m->is, "[MENU]");
}

const menu_item_class_t menuItemTime = {0, menu_item_time_event, menu_item_time_draw, 0, 3, 2, 3};
const menu_item_class_t menuItemInt = {0, menu_item_int_event, menu_item_int_draw, 4, 4, 1, 1};
const menu_item_class_t menuItemSelect = {menu_item_select_entry, menu_item_select_event, menu_item_select_draw, 0, 1, 8, 0};
const menu_item_class_t menuItemStr = {0, menu_item_string_event, menu_item_string_draw, 0, 8, 1, 1};
const menu_item_class_t menuItemSubMenu = {menu_item_sub_menu_entry, menu_item_sub_menu_event, menu_item_sub_menu_draw, 0, 8, 1, 1};

//---------------------------------------------------------------------------------------------------

void menu_init(menu_t *self, const menu_item_t *first)
{
  self->editIndex = 0;
  self->item = first;
  self->item_list = first;
  self->state = MENU_STATE_NAVIGATE;
  self->update = 1;
  self->blinkState = 0;
}

void menu_update(menu_t *self)
{
  self->update = 1;
}

void menu_event(menu_t *self, const menu_event_t *evt)
{
  if(self->state == MENU_STATE_EDIT)
  {
    if(self->item != 0)
    {
      self->item->ic->event(self, self->item, evt);
      menu_update(self);
    }
  }
  else
  {
    if(evt->type == MENU_EVENT_CLICK)
    {
      if(evt->id == MENU_BUTTON_ID_NAV)
      {
        // If reached end of items, wrap around
        if((self->item+1)->name == 0)
          self->item = self->item_list;
        else
          self->item++;
        menu_update(self);
      }
      else if(evt->id == MENU_BUTTON_ID_SELECT)
      {
        if(self->item->flags & MENU_ITEM_EDITABLE)
          self->state = MENU_STATE_EDIT;
        // Call entry handler if set
        if(self->item->ic->entry != 0)
            self->item->ic->entry(self, self->item);
      }
    }
  }
}

void menu_blink(menu_t *self)
{
  self->blinkState = !self->blinkState;
  self->update = 1;
}

void menu_draw(menu_t *self)
{
  if(self->update)
  {
    self->update = 0;
    MCAL_LOCK_IRQ();
    if(self->item != 0)
    {
      self->is[0] = 0;
      self->item->ic->draw(self, self->item);
      if(self->blinkState != 0 && self->state == MENU_STATE_EDIT)
      {
       for(int n = 0; n < self->item->ic->subItemChars; n++)
         self->is[n+self->item->ic->subItemStart+(self->item->ic->subItemDistance * self->editIndex)] = ' ';

      }
      char line1[17];
      char line2[17];

      snprintf(line1, sizeof(line1), "%s        ", self->item->name);
      snprintf(line2, sizeof(line2), "%s        ", self->is);
      MCAL_UNLOCK_IRQ();

      hd44780_set_cursor(0, 0);
      hd44780_print(line1);


      hd44780_set_cursor(0, 1);
      hd44780_print(line2);
    }
    MCAL_UNLOCK_IRQ();
  }
}


