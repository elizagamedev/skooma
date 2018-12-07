#pragma once

#include <cstdint>

class Config
{
public:
    Config();

    bool auto_tdt;
    bool limit_framerate;
    bool scrollclick;
    uint8_t scrollclick_modifier_key;
    bool scrollclick_disable_scroll;
};

extern Config global_config;
