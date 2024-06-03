#include "inputapplicant.h"
#include "utils.h"
#include "config.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <thread>
#include <cstring>
#include <signal.h>

X11InputApplicant::X11InputApplicant()
{
}

X11InputApplicant::~X11InputApplicant()
{
}

void X11InputApplicant::start()
{
    if (running.test())
        return;
    running.test_and_set();
    thread = std::thread(&X11InputApplicant::listen_loop, this);
}

void X11InputApplicant::stop()
{
    if (!running.test())
    {
        thread.join();
        return;
    }
    running.clear();
    ::shutdown(listen_socket, SHUT_RDWR);
    thread.join();
}

void X11InputApplicant::listen_loop()
{
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    if (setsockopt(listen_socket, SOL_SOCKET,
                   SO_REUSEADDR, &opt, sizeof(opt)))
    {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    if (listen_socket == -1)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(IO_TCP_PORT);
    if (bind(listen_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        std::cerr << "Failed to bind socket" << std::endl;
        ::close(listen_socket);
        return;
    }
    if (listen(listen_socket, 1) == -1)
    {
        std::cerr << "Failed to listen on socket" << std::endl;
        ::close(listen_socket);
        return;
    }
    std::cout << "Listening for incoming connections..." << std::endl;
    struct sockaddr_in client_address;
    socklen_t client_addr_length = sizeof(client_address);
    int clientSocket = accept(listen_socket, (struct sockaddr *)&client_address, &client_addr_length);
    if (clientSocket == -1)
        std::cerr << "Failed to accept connection" << std::endl;
    while (running.test())
    {
        Event event;
        if (!read(clientSocket, &event))
            break;
        consume(event);
    }
    ::close(clientSocket);
    ::close(listen_socket);
    running.clear();
}

void X11InputApplicant::consume(const Event &event)
{
    switch (event.mode)
    {
    case Mode::KeyDown:
        exec("xdotool keydown " + std::to_string(event.code));
        break;
    case Mode::KeyUp:
        exec("xdotool keyup " + std::to_string(event.code));
        break;
    case Mode::MouseDown:
        exec("xdotool mousemove " + std::to_string(event.x) + " " + std::to_string(event.y) + " mousedown " + std::to_string(event.code));
        break;
    case Mode::MouseUp:
        exec("xdotool mousemove " + std::to_string(event.x) + " " + std::to_string(event.y) + " mouseup " + std::to_string(event.code));
        break;
    case Mode::MouseMove:
        exec("xdotool mousemove " + std::to_string(event.x) + " " + std::to_string(event.y));
        break;
    }
}
