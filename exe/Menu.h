#pragma once

#include <windows.h>
#include <functional>
#include <memory>


class AbstractMenuItem
{
public:
    UINT_PTR id() = 0;
};

class MenuItem : public AbstractMenuItem
{
public:
    MenuItem(UINT flags, std::function<void()> on_click);

    UINT_PTR id();

private:
    UINT flags_;
    std::function<void()> on_click_;
    UINT id_;
}

class Menu : public AbstractMenuItem
{
public:
    explicit Menu();
    ~Menu();

    UINT_PTR id();

    void append(LPWSTR text, std::unique_ptr<AbstractMenuItem> item);

private:
    HMENU menu_;
    std::vector<std::unique_ptr<MenuItem*>> items_;
}
