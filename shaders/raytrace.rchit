#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require

#include "common/global.glsl"
#include "common/sampling.glsl"
#include "input_structures.glsl"
#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT HitPayload prd;
hitAttributeEXT vec2 attribs;

void CreateONB(vec3 n, out vec3 t, out vec3 bt) {
  float sign = (n.z >= 0.0) ? 1.0 : -1.0;
  float a    = -1.0 / (sign + n.z);
  float b    = n.x * n.y * a;
  t          = vec3(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x);
  bt         = vec3(b, sign + n.y * n.y * a, -n.y);
}

void DiffuseSample(vec3 R, vec3 wo, vec2 s1, out vec3 f, out vec3 wi, out float pdf) {
    wi = CosineHemisphere(s1);
    if (wo.z < 0) {
        wi.z = -1;
    }
    pdf = CosineHemispherePDF(abs(wi.z));
    f = R * INV_PI;
}

float DiffusePDF(vec3 wo, vec3 wi) {
    if (wo.z * wi.z <= 0) return 0;
    return abs(wi.z) * INV_PI;
}

void main()
{
  GPUObjectData obj = objectData[gl_InstanceCustomIndexEXT];
  GPUMaterialData mat = materialData[obj.material];

  const vec3 b = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

  ivec3 idx = indices.data[obj.startIndex + gl_PrimitiveID];

  vec3 v0 = vertices.data[idx.x];
  vec3 v1 = vertices.data[idx.y];
  vec3 v2 = vertices.data[idx.z];

  const vec3 pos = v0 * b.x + v1 * b.y + v2 * b.z;

  vec3 n0 = normals.data[idx.x];
  vec3 n1 = normals.data[idx.y];
  vec3 n2 = normals.data[idx.z];

  const vec3 n = n0 * b.x + n1 * b.y + n2 * b.z;

  const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));
  const vec3 worldN = normalize(vec3(n * gl_WorldToObjectEXT));

  prd.position = worldPos;
  prd.normal   = worldN;

  vec3 diffuse;
  if (mat.diffuseTexture >= 0) {
    vec2 tx0 = texCoords.data[idx.x];
    vec2 tx1 = texCoords.data[idx.y];
    vec2 tx2 = texCoords.data[idx.z];

    const vec2 tx = tx0 * b.x + tx1 * b.y + tx2 * b.z;
    diffuse = texture(textures[mat.diffuseTexture], tx).rgb;
  } else {
    diffuse = mat.diffuse.rgb;
  }

  prd.bIsMiss  = false;
  prd.emission = mat.emission.rgb;

  vec3 tangent, bitangent;
  CreateONB(worldN, tangent, bitangent);
  mat3 TBN = mat3(tangent, bitangent, worldN);

  // Convert wo to local space
  vec3 wo = prd.direction;
  vec3 woLocal = transpose(TBN) * wo;
  
  // Sample diffuse BRDF
  vec3 f, wiLocal;
  float pdf;
  DiffuseSample(diffuse, woLocal, prd.s1, f, wiLocal, pdf);

  prd.direction = TBN * wiLocal;
  prd.f         = f;
  prd.pdf       = pdf;
}
