#include "rand.hpp"

static thread_local uint32_t rngState = 1;

uint32_t pcgRand() {
    const auto state = rngState;
    rngState         = rngState * RXS_M_XS_MULT + RXS_M_XS_INCR;
    const auto word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 2u) ^ word;
}