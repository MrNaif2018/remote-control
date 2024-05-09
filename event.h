#pragma once

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>

enum class Mode
{
    KeyDown,
    KeyUp,
    MouseDown,
    MouseUp,
    MouseMove
};

inline std::string get_mode_value(const Mode &m)
{
    switch (m)
    {
    case Mode::KeyDown:
        return "KeyDown";
    case Mode::KeyUp:
        return "KeyUp";
    case Mode::MouseDown:
        return "MouseDown";
    case Mode::MouseUp:
        return "MouseUp";
    case Mode::MouseMove:
        return "MouseMove";
    }
    return "Unknown";
}

inline std::ostream &operator<<(std::ostream &out, const Mode value)
{
    return out << get_mode_value(value);
}

struct Event
{
    Mode mode;
    int code;
    int x;
    int y;
};

inline std::ostream &operator<<(std::ostream &os, const Event &event)
{
    return os << "Mode: " << event.mode << " Code: " << event.code << " X: " << event.x << " Y: " << event.y;
}

inline bool write(int socket, Event req)
{
    auto bytes_sent = ::send(socket, &req, sizeof(Event), 0);
    return bytes_sent == sizeof(Event);
}

inline bool read(int socket, Event *buf)
{
    auto bytes_read = ::read(socket, buf, sizeof(Event));
    return bytes_read == sizeof(Event);
}
