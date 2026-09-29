/*!
 * @brief   Minimal model shader (STARTING POINT).
 *
 * Right now this only draws the albedo (base color) texture. There is NO lighting.
 * Your job is to add lighting here, step by step:
 *   1. ambient        (add a constant brightness)
 *   2. diffuse         (directional light + Lambert:  dot(normal, -lightDir))
 *   3. specular        (Phong / Blinn-Phong highlight)
 *   4. specular map    (control the highlight strength per-pixel with a texture)
 *   5. normal map      (per-pixel normals using the tangent space / TBN)
 *
 * See: 学習ガイド_自作レンダリングエンジン.md  (Phase 1)
 *
 * NOTE: To pass values (light direction/color, ambient, etc.) from C++ into this
 *       shader, add your own cbuffer here and fill it from a ConstantBuffer on the
 *       C++ side. The engine does NOT feed lighting data anymore (that used to be
 *       k2Engine's job, which you are now replacing).
 */

////////////////////////////////////////////////
// Pixel shader input.
////////////////////////////////////////////////
struct SPSIn
{
    float4 pos      : SV_POSITION;  // Clip-space position.
    float3 normal   : NORMAL;       // World-space normal.
    float3 tangent  : TANGENT;      // World-space tangent   (for normal mapping later).
    float3 biNormal : BINORMAL;     // World-space binormal  (for normal mapping later).
    float2 uv       : TEXCOORD0;    // UV.
    float3 worldPos : TEXCOORD1;    // World-space position  (for specular later).
};

///////////////////////////////////////
// Common vertex shader code.
// Provides: ModelCb(b0: mWorld/mView/mProj), SVSIn, bone matrices (t3),
//           and the entry points VSMain / VSMainSkin / VSMainInstancing, etc.
///////////////////////////////////////
#include "ModelVSCommon.h"

cbuffer DirectionLightCb : register(b1)
{
    float3 ambientLight;
    float3 ligDirection;
    float3 ligColor;
    float3 eyePos;
    float  specPow;
    float  specIntensity;
    float  shadowBias;
    float4x4 mLVP;
}

///////////////////////////////////////
// Shader resources.
// The tkm material binds the albedo texture to t0.
// (t1 = normal map, t2 = metallic/smooth — you can add them when you need them.)
///////////////////////////////////////
Texture2D<float4> albedoTexture : register(t0);
Texture2D<float4> normalMap : register(t1);
Texture2D<float4> specularMap : register(t2);
Texture2D<float4> g_shadowMap : register(t10);   // ModelRender が t10 に繋いだシャドウマップ
sampler Sampler : register(s0);

///////////////////////////////////////
// Shadow settings.
///////////////////////////////////////
static const float SHADOW_MAP_SIZE = 1024.0f;  // RenderingEngine の m_shadowMap.Create のサイズと合わせる
// static const float SHADOW_BIAS     = 0.001f;   // シャドウバイアス（シャドウアクネ対策）

////////////////////////////////////////////////
// Vertex shader core (called by the VSMain* entry points in ModelVSCommon.h).
////////////////////////////////////////////////
SPSIn VSMainCore(SVSIn vsIn, float4x4 mWorldLocal, uniform bool isUsePreComputedVertexBuffer)
{
    SPSIn psIn;

    // Local space -> world space.
    psIn.pos = CalcVertexPositionInWorldSpace(vsIn.pos, mWorldLocal, isUsePreComputedVertexBuffer);
    psIn.worldPos = psIn.pos;

    // World -> view -> projection (clip) space.
    psIn.pos = mul(mView, psIn.pos);
    psIn.pos = mul(mProj, psIn.pos);

    // World-space normal / tangent / binormal.
    CalcVertexNormalTangentBiNormalInWorldSpace(
        psIn.normal,
        psIn.tangent,
        psIn.biNormal,
        mWorldLocal,
        vsIn.normal,
        vsIn.tangent,
        vsIn.biNormal,
        isUsePreComputedVertexBuffer
    );

    psIn.uv = vsIn.uv;
    return psIn;
}

////////////////////////////////////////////////
// Soft shadow (PCF).
// 周囲 3x3 テクセルで深度比較を行い、「影だった割合」を返す。
// 戻り値：0.0 = 影なし、1.0 = 完全に影
////////////////////////////////////////////////
float CalcShadowRatePCF(float2 shadowMapUV, float zInLVP)
{
    float2 texelSize   = 1.0f / SHADOW_MAP_SIZE;   // シャドウマップ1テクセル分のUV幅
    float  shadowCount = 0.0f;

    [unroll]
    for (int y = -1; y <= 1; y++)
    {
        [unroll]
        for (int x = -1; x <= 1; x++)
        {
            float2 uv = shadowMapUV + float2(x, y) * texelSize;
            float  zInShadowMap = g_shadowMap.Sample(Sampler, uv).r;

            // Step 2-3 と同じ判定（バイアス付き）。影なら 1 を数える
            if (zInLVP > zInShadowMap + shadowBias)
            {
                shadowCount += 1.0f;
            }
        }
    }
    return shadowCount / 9.0f;   // 3x3 = 9 サンプルの平均
}

////////////////////////////////////////////////
// Soft shadow (VSM).
// ぼかし済みの (E[d], E[d^2]) から、チェビシェフの不等式で影の割合を求める。
// 戻り値：0.0 = 影なし、1.0 = 完全に影
////////////////////////////////////////////////
float CalcShadowRateVSM(float2 shadowMapUV, float zInLVP)
{
    zInLVP -=shadowBias;
    float2 moments = g_shadowMap.Sample(Sampler, shadowMapUV).xy;

    // 平均より手前なら光が当たっている
    if (zInLVP <= moments.x)
    {
        return 0.0f;
    }

    // 分散 = E[d^2] - E[d]^2
    float variance = moments.y - moments.x * moments.x;
    variance = max(variance, 0.00002f);   // 精度誤差対策（シャドウバイアスの役目も兼ねる）

    // 光が届く確率の上限
    float d    = zInLVP - moments.x;
    float pMax = variance / (variance + d * d);

    return 1.0f - pMax;
}

////////////////////////////////////////////////
// Pixel shader.
// For now: just output the albedo texture. Add your lighting here.
////////////////////////////////////////////////
float4 CalcLitColor(SPSIn In, bool receiveShadow)
{
    float4 albedoColor = albedoTexture.Sample(Sampler, In.uv);

    float3 normal = normalize(In.normal);

    float3 localNormal = normalMap.Sample(Sampler, In.uv).xyz;
    localNormal = (localNormal - 0.5f) * 2.0f;
    normal = normalize(In.tangent * localNormal.x + In.biNormal * localNormal.y + normal * localNormal.z);

    float t = max(0.0f, dot(normal, -ligDirection));
    float3 diffuse = ligColor * t;

    float3 refVec = reflect(ligDirection, normal);
    float3 toEye = normalize(eyePos - In.worldPos);
    float t2 = pow(max(0.0f, dot(refVec, toEye)), specPow);
    float3 specular = ligColor * specIntensity * t2;

    float specP = specularMap.Sample(Sampler, In.uv).r;
    specular *= specP;

    float3 lig = ambientLight + diffuse + specular;

    // 先にライティングを済ませる（md Step 2-2：影はライティングの後）
    float4 finalColor = albedoColor;
    finalColor.xyz *= lig;

    if (receiveShadow)
    {
        float4 posInLVP = mul(mLVP, float4(In.worldPos, 1.0f));
        float2 shadowMapUV = posInLVP.xy / posInLVP.w;
        shadowMapUV *= float2(0.5f, -0.5f);
        shadowMapUV += 0.5f;

        float zInLVP = posInLVP.z / posInLVP.w;

        if (shadowMapUV.x > 0.0f && shadowMapUV.x < 1.0f
         && shadowMapUV.y > 0.0f && shadowMapUV.y < 1.0f)
        {
            float shadowRate = CalcShadowRateVSM(shadowMapUV, zInLVP);   // PCF → VSM
            finalColor.xyz *= lerp(1.0f, 0.5f, shadowRate);   // 0.5 = 影の濃さ
        }
    }

    return finalColor;
}

float4 PSMain(SPSIn In) : SV_Target0
{
    return CalcLitColor(In, false);   // 影のサンプリングをしない
}

float4 PSMainShadowReceiver(SPSIn In) : SV_Target0
{
    return CalcLitColor(In, true);    // 影のサンプリングをする
}
