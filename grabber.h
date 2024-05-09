#pragma once

#include <string>
#include <utility>
#include <optional>
#include <functional>
#include <thread>
#include <arpa/inet.h>

#include "event.h"

#define BUFFER_SIZE 1024 * 1024

std::pair<int, int> get_current_mouse_location();

std::optional<Mode> parse_keyboard(const std::string &line, std::optional<Mode> mode, std::optional<Event> &event);

std::optional<Mode> parse_click(const std::string &line, std::optional<Mode> mode, std::optional<Event> &event);

class X11Grabber
{
public:
    X11Grabber() = default;
    virtual ~X11Grabber();
    virtual void start();
    virtual void stop();
    void set_callback(std::function<void(const Event &)> callback);

private:
    std::function<void(const Event &)> callback;
    std::thread thread;
    std::atomic<bool> running = false;
    void loop();
};

class GrabberSender : public X11Grabber
{

public:
    GrabberSender() = default;
    void set_host(std::string host);
    void start() override;
    void stop() override;

    ~GrabberSender() override;

private:
    std::string host;
    struct timespec ts = {0, 0};
    struct sockaddr_in server_address;
    int socket;
};
