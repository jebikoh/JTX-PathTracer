#include "rand.hpp"

static thread_local uint32_t rngState = 1;

uint32_t randPCG() {
    const auto state = rngState;
    rngState         = rngState * PCG32_MULT_32 + PCG32_INCR_32;
    const auto word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 2u) ^ word;
}