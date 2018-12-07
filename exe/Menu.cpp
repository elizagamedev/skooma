#include "Menu.h"

#include "win32.h"


static UINT_PTR num_item_ids = 0;

MenuItem::MenuItem(UINT flags, std::function<void()> on_click)
    : flags_(flags)
    , on_click_(std::move(on_click))
    , id_(++num_item_ids)
{
}

UINT_PTR MenuItem::id()
{
    return id_;
}

Menu::Menu()
{
    if ((menu_ = CreatePopupMenu()) == NULL) {
        throw win32::get_last_error_exception();
    }
}

Menu::~Menu()
{
    if (menu_ != NULL) {
        DestroyMenu(menu_);
    }
}

UINT_PTR Menu::id()
{
    return static_cast<UINT_PTR>(menu_);
}

void Menu::append(LPWSTR text, std::unique_ptr<AbstractMenuItem> item)
{

}
