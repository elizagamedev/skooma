#pragma once

#include "WindowsHook.h"


class Skooma
{
public:
    explicit Skooma(HINSTANCE hInstance);
    ~Skooma();

    void run();

private:
    HINSTANCE instance_;
};
