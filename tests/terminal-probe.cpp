#include "terminal.h"
#include <cstdio>
int main()
{
    const auto line = readBlinkingLine();
    if (!line) return 2;
    std::fputs(line->c_str(), stdout);
    return 0;
}
