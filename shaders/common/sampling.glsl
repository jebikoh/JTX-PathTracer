vec3 SampleUniformSphere(vec2 s) {
    const float z = 1 - 2 * s.x;
    const float a = sqrt(1.0f - z * z);
    const float phi = TWO_PI * s.y;
    return vec3(cos(phi) * a, sin(phi) * a, z);
}

float UniformSpherePDF() {
    return INV_FOUR_PI;
}

vec2 UniformDiscPolar(vec2 s) {
    const float r     = sqrt(s.x);
    const float theta = TAU * s.y;
    return vec2(r * cos(theta), r * sin(theta));
}

vec2 UniformDiskConcentric(vec2 s) {
    const vec2 offset = 2.0f * s - vec2(1.0f, 1.0f);
    if (offset.x == 0 && offset.y == 0) return vec2(0.0f, 0.0f);

    float r, theta;
    if (abs(offset.x) > abs(offset.y)) {
        // X is dominant axis
        r = offset.x;
        theta = PI_OVER_4 * (offset.y / offset.x);
    } else {
        // Y is dominant axis
        r = offset.y;
        theta = PI_OVER_2 - PI_OVER_4 * (offset.x / offset.y);
    }

    return vec2(r * cos(theta), r * sin(theta));
}

vec3 UniformHemisphere(vec2 s) {
    const float sinTheta = sqrt(1 - s.x * s.x);
    const float phi      = 2 * PI * s.y;
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, s.x);
}

float UniformHemispherePDF() {
    return INV_TWO_PI;
}

vec3 CosineHemisphere(vec2 s) {
    const vec2 disk = UniformDiskConcentric(s);
    return vec3(disk.x, disk.y, sqrt(1 - disk.x * disk.x - disk.y * disk.y));
}

float CosineHemispherePDF(float cosTheta) {
    return cosTheta * INV_PI;
}

vec3 UniformTriangle(vec2 s) {
    s.x = sqrt(s.x);
    const float b0 = 1 - s.x;
    const float b1 = s.y * s.x;
    return vec3(b0, b1, 1 - b0 - b1);
}
