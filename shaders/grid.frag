#version 450


layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    mat4 invViewProj;
    vec4 cameraPos;
} pushConstants;

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inUV;

vec4 grid(vec3 fragPos, float scale) {
    // Isolate XZ coordinates and apply scale
    // Scale determines grid size; higher scale -> smaller grid cells
    // https://thebookofshaders.com/09/
    vec2 xzPos = fragPos.xz * scale;

    // Basic AA
    // fwidth esimates how much grid position changes between adjacent screen pixels.
    // Parts of the grid that are more zoomed in will have a smaller fwidth value.
    vec2 derivative = fwidth(xzPos);

    // Calculates the distance to the nearest integer
    vec2 nearestLine = abs(fract(xzPos - 0.5) - 0.5);
    // Divide by the derivative to get screen pixel distance
    nearestLine = nearestLine / derivative;

    // AA applied by setting alpha based on the distance to the nearest line
    float minDistance = min(nearestLine.x, nearestLine.y);
    vec4 baseColor = vec4(0.2, 0.2, 0.2, 1.0 - min(minDistance, 1.0));

    // Axis masks for X and Z axes
    const float axisWidth = 1.0;
    // The abs operation gives us the distance to the axes in grid units
    // Similar to before, dividing by the derviative gives the distance in screen pixels
    float wx = abs(xzPos.x) / derivative.x;
    float wz = abs(xzPos.y) / derivative.y;
    // Smoothly transition alpha via Hermite interpolation
    float axisMaskX = 1.0 - smoothstep(0.0, axisWidth, wx);
    float axisMaskZ = 1.0 - smoothstep(0.0, axisWidth, wz);

    // Create red and green colors to be mixed into the base color via axis masks
    vec4 axisColorX = vec4(1, 0, 0, axisMaskX);
    vec4 axisColorZ = vec4(0, 1, 0, axisMaskZ);

    // Mix the axis colors over the base color
    vec4 color = baseColor;
    color = mix(color, axisColorX, axisMaskX);
    color = mix(color, axisColorZ, axisMaskZ);

    return color;
}

void main() {
    vec2 ndc = inUV * 2.0 - 1.0;
    vec4 clipSpacePos = vec4(ndc, 1.0, 1.0);
    vec4 worldPos = pushConstants.invViewProj * clipSpacePos;
    worldPos /= worldPos.w;

    vec3 cameraPos = pushConstants.cameraPos.xyz;

    vec3 rayDir = normalize(worldPos.xyz - cameraPos);
    float t = -cameraPos.y / rayDir.y;
    if (t < 0.0) discard;

    vec3 fragPosWorld = cameraPos + t * rayDir;

    // Compute depth
    vec4 fragPosClip = pushConstants.viewProj * vec4(fragPosWorld, 1.0);
    gl_FragDepth = fragPosClip.z / fragPosClip.w;

    vec4 color = grid(fragPosWorld, 1);

    // Apply distance based fade
    const float fadeStartDistance = 25.0;
    const float fadeEndDistance   = 75.0;
    float dist = distance(cameraPos, fragPosWorld);
    float fade = 1.0 - smoothstep(fadeStartDistance, fadeEndDistance, dist);
    color *= fade;

    outColor = color;
}
