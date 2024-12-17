#include "aabb.hpp"

const AABB AABB::EMPTY    = AABB(Vec3(INF, INF, INF), Vec3(-INF, -INF, -INF));
const AABB AABB::UNIVERSE = AABB(Vec3(-INF, -INF, -INF), Vec3(INF, INF, INF));