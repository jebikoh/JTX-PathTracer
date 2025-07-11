#pragma once
#include <jtx.hpp>
#include <image.hpp>

namespace jtx {

struct Envmap {
    enum kType {
        SOLID = 0,
        IMAGE = 1,
    };

    vec3 solid{0.0f};
    Image32f IBL;


};

}