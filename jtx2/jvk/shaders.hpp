#pragma once

#include <jvk/jvk.hpp>
#include <fstream>

namespace jvk {

inline bool LoadShaderModule(const char *filePath, const VkDevice device, VkShaderModule *outShaderModule) {
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    const auto fileSize = file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext    = nullptr;
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode    = buffer.data();

    VkShaderModule shader;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shader) != VK_SUCCESS) {
        return false;
    }

    *outShaderModule = shader;
    return true;
}

}// namespace jvk
