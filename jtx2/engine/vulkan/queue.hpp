#pragma once

namespace jtx {

/**
 * UI and render threads communicate via a command queue
 */
struct JtxCommand {
    enum kType {
        UPDATE_SCENE_DATA = 0,
        NUM_COMMAND_TYPES
    };

    kType type;
};

struct CommandQueue {
    void Push(JtxCommand cmd) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push_back(cmd);
    }

    std::vector<JtxCommand> PopAll() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::move(m_queue);
    }

private:
    std::vector<JtxCommand> m_queue;
    mutable std::mutex m_mutex;
};

}