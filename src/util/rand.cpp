#include "rand.hpp"

static thread_local uint32_t rngState = 1;

uint32_t randPCG() {
    const auto state = rngState;
    rngState         = rngState * 747796405u + 2891336453u;
    const auto word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 2u) ^ word;
}