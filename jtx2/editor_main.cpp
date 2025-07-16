#include <bvh/bvh.hpp>
#include <editor/editor.hpp>

using namespace jtx;

int main(int argc, char *argv[]) {
    Logger::AddDefaultSink();

    Editor editor;
    editor.Init();
    editor.Run();
    editor.Destroy();

    return 0;
}
