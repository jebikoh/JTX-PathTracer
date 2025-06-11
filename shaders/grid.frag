#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inNearPoint;
layout(location = 1) in vec3 inFarPoint;
layout(location = 2) in mat4 inViewProj;

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
    vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(minDistance, 1.0));

    // Lines for X and Z axes
    // We want to adjust the color of grid fragments if they fall within a certain
    // distance of the X or Z axes.
    //
    // The scale is adjusted based the clamped derivative to ensure that the line
    // width is consistent across different zoom levels.
    float minimumx = min(derivative.x, 1);
    if (fragPos.x > -0.1 * minimumx && fragPos.x < 0.1 * minimumx)  color.r = 1.0;

    float minimumz = min(derivative.y, 1);
    if (fragPos.z > -0.1 * minimumz && fragPos.z < 0.1 * minimumz) color.g = 1.0;

    return color;
}

void main() {
    // Calculate the time at which a ray from inNearPoint to inFarPoint intersects the plane y=0
    float t = -inNearPoint.y / (inFarPoint.y - inNearPoint.y);
    vec3 fragPos  = inNearPoint + t * (inFarPoint - inNearPoint);

    // Compute depth
    vec4 clipSpacePos = inViewProj * vec4(fragPos, 1.0);
    gl_FragDepth = clipSpacePos.z / clipSpacePos.w;

    // If the t <= 0 or t >= 1.0, the ray intersects the plane outside the near and far planes.
    outColor = grid(fragPos, 1) * float(t > 0 && t < 1.0);
}

