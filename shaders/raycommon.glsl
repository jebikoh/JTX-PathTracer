struct HitPayload {
    bool bIsMiss;
    vec2 s1;

    vec3 emission;    
    vec3 f;
    float pdf;
    vec3 direction;

    vec3 position;
    vec3 normal;
};