#include <bvh/bvh.hpp>
#include <editor/editor.hpp>

using namespace jtx;

#define JTX_USE_DEBUG_FILE_SINK false

int main(int argc, char *argv[]) {
    if (JTX_USE_DEBUG_FILE_SINK) {
        auto fileSink = std::make_shared<FileSink>("debug.log");
        Logger::Get().AddSink([fileSink](const LogEntry &entry) {
           fileSink->Process(entry);
        });
    } else {
        Logger::AddDefaultSink();
    }

    Editor editor;
    editor.Init();
    editor.Run();
    editor.Destroy();

    return 0;
}
