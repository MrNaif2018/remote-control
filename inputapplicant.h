#pragma once
#include <thread>
#include <atomic>

#include "event.h"

class X11InputApplicant
{
public:
    X11InputApplicant();
    ~X11InputApplicant();
    void start();
    void listen_loop();
    void stop();
    void consume(const Event &event);

private:
    sigset_t mask;
    std::thread thread;
    std::atomic_flag running = ATOMIC_FLAG_INIT;
};