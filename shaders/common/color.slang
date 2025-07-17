module color;

float Luminance(float3 color) {
    return 0.2125 * color.x + 0.7154 * color.y + 0.0721 * color.z;
}

float3 ApplyLuminance(float3 color, const float L1, const float L2) {
    if (L1 == 0.0f) return color;
    return color * (L2 / L1);
}

// -- Tonemapping --
public float3 Reinhard(float3 v) {
    const float L1 = Luminance(v);
    return v / (1.0f + L1);
}

// ACES
// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
static const float3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

public float3 ACESFitted(float3 color)
{
    color = mul(ACESInputMat, color);

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = mul(ACESOutputMat, color);

    // Clamp to [0, 1]
    color = saturate(color);

    return color;
}

// Hable Filmic (Uncharted 2)
float3 HableFilmicPartial(float3 x)
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

public float3 HableFilmic(float3 v) {
    float exposureBias = 2.0f;
    float3 partial = HableFilmicPartial(v * exposureBias);
    float3 W = float3(11.2f);
    float3 whiteScale = float3(1.0f) / HableFilmicPartial(W);
    return partial * whiteScale;
}

// AGX
// https://iolite-engine.com/blog_posts/minimal_agx_implementation
static const float3x3 agxMatInv = float3x3(
    1.19687900512017, -0.0528968517574562, -0.0529716355144438,
    -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
    -0.0990297440797205, -0.0989611768448433, 1.15107367264116);

static const float3x3 agxMat = float3x3(
    0.842479062253094, 0.0423282422610123, 0.0423756549057051,
    0.0784335999999992,  0.878468636469772,  0.0784336,
    0.0792237451477643, 0.0791661274605434, 0.879142973793104);

float3 AgXDefaultContrastApprox(float3 x) {
  float3 x2 = x * x;
  float3 x4 = x2 * x2;
  
  return + 15.5     * x4 * x2
         - 40.14    * x4 * x
         + 31.96    * x4
         - 6.868    * x2 * x
         + 0.4298   * x2
         + 0.1191   * x
         - 0.00232;
}

float3 AgXLook(float3 val) {  
  float3 offset = float3(0.0);
  float3 slope  = float3(1.0);
  float3 power  = float3(1.0);
  float sat = 1.0;
  
  val = pow(val * slope + offset, power);
  
  const float3 lw = float3(0.2126, 0.7152, 0.0722);
  float luma = dot(val, lw);
  
  return luma + sat * (val - luma);
}

public float3 AgX(float3 val) {
  const float minEV = -12.47393f;
  const float maxEV = 4.026069f;

  val = mul(agxMat, val);
  
  val = clamp(log2(val), minEV, maxEV);
  val = (val - minEV) / (maxEV - minEV);
  
  val = AgXDefaultContrastApprox(val);
  val = AgXLook(val);
  
  val = mul(agxMatInv, val);

  return val;
}

public float3 LinearToSRGB(float3 linear) {
    float3 srgb;
    for (int i = 0; i < 3; ++i) {
        if (linear[i] <= 0.0031308) {
            srgb[i] = 12.92f * linear[i];
        } else {
            srgb[i] = 1.055 * pow(linear[i], 1.0f / 2.4f) - 0.055f;
        }
    }
    return srgb;
}