#pragma once

namespace jtx {

template<typename T>
float calculateMSE(std::vector<T> x, std::vector<T> y) {
    if (x.size() != y.size()) {
        LOG_ERROR(GENERAL, "Both x and y vectors must be the same size");
        return false;
    }

    float mse = 0.0f;
    for (size_t i = 0; i < x.size(); i++) {
        const float diff = x[i] - y[i];
        const float sqr  = diff * diff;
        mse += sqr;
    }

    return mse / x.size();
}

}// namespace jtx