#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require

#include "input_structures.glsl"
#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT HitPayload prd;
hitAttributeEXT vec3 attribs;

void main()
{
  GPUObjectData obj = objectData[gl_InstanceCustomIndexEXT];
  GPUMaterialData mat = materialData[obj.material];

  const vec3 b = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

  ivec3 idx = indices.data[gl_PrimitiveID];

  vec3 v0 = vertices.data[idx.x];
  vec3 v1 = vertices.data[idx.y];
  vec3 v2 = vertices.data[idx.z];

  const vec3 pos = v0 * b.x + v1 * b.y + v2 * b.z;
  const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));

  vec3 n0 = normals.data[idx.x];
  vec3 n1 = normals.data[idx.y];
  vec3 n2 = normals.data[idx.z];

  const vec3 n = n0 * b.x + n1 * b.y + n2 * b.z;
  const vec3 worldN = normalize(vec3(n * gl_WorldToObjectEXT));

  vec3 L = normalize(vec3(1, 1, 1));

  prd.hitValue = vec3(max(dot(n, L), 0.2));
}
