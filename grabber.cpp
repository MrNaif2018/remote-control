#include "utils.h"
#include "grabber.h"
#include "config.h"
#include <optional>
#include <sys/types.h>
#include <signal.h>
#include "popen2.h"

std::pair<int, int> get_current_mouse_location()
{
    auto output = exec("xdotool getmouselocation");
    auto parts = Split(output, " ");
    if (parts.size() < 2)
        return {0, 0};
    auto x = std::stoi(parts[0].substr(2));
    auto y = std::stoi(parts[1].substr(2));
    return {x, y};
}

std::optional<Mode> parse_keyboard(const std::string &line, std::optional<Mode> mode, std::optional<Event> &event)
{
    if (line.starts_with("    detail: "))
    {
        auto num = std::stoi(line.substr(12));
        event.emplace(Event{mode.value(), num, 0, 0});
        return std::nullopt;
    }
    return mode;
}

std::optional<Mode> parse_click(const std::string &line, std::optional<Mode> mode, std::optional<Event> &event)
{
    if (line.starts_with("    detail: "))
    {
        auto code = 0;
        if (line.size() > 12)
            code = std::stoll(line.substr(12));
        auto coords = get_current_mouse_location();
        event.emplace(Event{mode.value(), code, coords.first, coords.second});
        return std::nullopt;
    }
    return mode;
}

X11Grabber::~X11Grabber()
{
    X11Grabber::stop();
}

void X11Grabber::start()
{
    if (running.load())
        return;
    running.store(true);
    thread = std::thread(&X11Grabber::loop, this);
}

void X11Grabber::stop()
{
    if (!running.load())
        return;
    running.store(false);
    thread.join();
}

void X11Grabber::set_callback(std::function<void(const Event &)> callback)
{
    this->callback = callback;
}

void X11Grabber::loop()
{
    pid_t child_pid;
    auto input = popen2("xinput test-xi2 --root", "r", &child_pid);
    if (!input)
    {
        fprintf(stderr,
                "incorrect parameters or too many files.\n");
        return;
    }
    std::optional<Mode> mode = std::nullopt;
    while (running.load() && !std::feof(input))
    {
        char buffer[BUFFER_SIZE] = {0};
        if (std::fgets(buffer, BUFFER_SIZE, input) != NULL)
        {
            std::string lines = buffer;
            for (const auto &line : Split(lines, "\n"))
            {
                if (!mode.has_value())
                {
                    if (line.starts_with("EVENT type 2"))
                        mode = Mode::KeyDown;
                    else if (line.starts_with("EVENT type 3"))
                        mode = Mode::KeyUp;
                    else if (line.starts_with("EVENT type 4"))
                        mode = Mode::MouseDown;
                    else if (line.starts_with("EVENT type 5"))
                        mode = Mode::MouseUp;
                    else if (line.starts_with("EVENT type 15"))
                        mode = Mode::MouseDown;
                    else if (line.starts_with("EVENT type 16"))
                        mode = Mode::MouseUp;
                    else if (line.starts_with("EVENT type 17"))
                        mode = Mode::MouseMove;
                    continue;
                }
                std::optional<Event> event = std::nullopt;
                switch (mode.value())
                {
                case Mode::KeyDown:
                    mode = parse_keyboard(line, Mode::KeyDown, event);
                    break;
                case Mode::KeyUp:
                    mode = parse_keyboard(line, Mode::KeyUp, event);
                    break;
                case Mode::MouseDown:
                    mode = parse_click(line, Mode::MouseDown, event);
                    break;
                case Mode::MouseUp:
                    mode = parse_click(line, Mode::MouseUp, event);
                    break;
                case Mode::MouseMove:
                    mode = parse_click(line, Mode::MouseMove, event);
                    break;
                }
                if (event.has_value())
                    callback(event.value());
            }
        }
    }
    kill(child_pid, SIGTERM);
    pclose(input);
}

void GrabberSender::set_host(std::string host)
{
    this->host = host;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(this->host.c_str());
    server_address.sin_port = htons(IO_TCP_PORT);
}

void GrabberSender::start()
{
    socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket == -1)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    if (::connect(socket, (struct sockaddr *)&server_address, sizeof(struct sockaddr_in)) == -1)
    {
        ::close(socket);
        std::cerr << "Failed to connect to server" << std::endl;
        return;
    }
    set_callback([this](const Event &event)
                 {
        if (event.mode == Mode::MouseMove) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (now.tv_sec - ts.tv_sec < 1 && now.tv_nsec - ts.tv_nsec < MOUSEMOVE_DELAY_NS)
                return;
            ts = now;
        }
        write(socket, event); });
    X11Grabber::start();
}

void GrabberSender::stop()
{
    X11Grabber::stop();
    ::close(socket);
}

GrabberSender::~GrabberSender()
{
    GrabberSender::stop();
}
