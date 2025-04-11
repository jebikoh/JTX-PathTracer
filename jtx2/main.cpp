#include <ui/display.hpp>

int main(int argc, char *argv[]) {
    Display display;
    display.init();
    display.run();
    display.cleanup();
    return 0;
}
