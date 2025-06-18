#pragma once

#include <jtx.hpp>

namespace jtx {

struct Scene;

// Export scene to JTX json format (.jtx)
JtxResult ExportScene(const Scene &scene, const std::filesystem::path &path, bool bEncodeImages = false);

}
