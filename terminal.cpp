#include "terminal.h"
#include <cerrno>
#include <cstdio>
#include <poll.h>
#include <unistd.h>

namespace {
void showCursor(bool visible)
{
    std::fputs(visible ? "\033[?25h" : "\033[?25l", stdout);
    std::fflush(stdout);
}
}

std::optional<std::string> readBlinkingLine()
{
    std::string line;
    bool visible = true;
    std::fputs("\033[6 q", stdout);
    showCursor(true);
    while (true) {
        pollfd descriptor{STDIN_FILENO, POLLIN, 0};
        const int result = ::poll(&descriptor, 1, 500);
        if (result < 0 && errno == EINTR) continue;
        if (result < 0 || (descriptor.revents & (POLLERR | POLLNVAL))) {
            showCursor(true);
            return std::nullopt;
        }
        if (result == 0) {
            visible = !visible;
            showCursor(visible);
            continue;
        }
        char character;
        const ssize_t count = ::read(STDIN_FILENO, &character, 1);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) {
            showCursor(true);
            if (line.empty()) return std::nullopt;
            return line;
        }
        visible = true;
        showCursor(true);
        if (character == '\n') return line;
        line.push_back(character);
    }
}
