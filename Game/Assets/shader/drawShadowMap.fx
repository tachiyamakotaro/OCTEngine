////////////////////////////////////////////////
// Shadow map (depth-only) pass.
//
// Reuses the shared VSMain / VSMainSkin / VSMainInstancing entry points and
// the ModelCb (mWorld/mView/mProj) / SVSIn / bone matrices provided by
// ModelVSCommon.h (same as model.fx), so skinned / instanced models still
// cast correct shadows without extra code here.
////////////////////////////////////////////////

////////////////////////////////////////////////
// Pixel shader input.
////////////////////////////////////////////////
struct SPSIn
{
    float4 pos      : SV_POSITION; // Clip-space position (rasterizer will turn this into screen pos).
    float4 posInLVP : TEXCOORD0;   // Un-touched light-view-projection clip-space position.
};

///////////////////////////////////////
// Common vertex shader code.
// Provides: ModelCb(b0: mWorld/mView/mProj), SVSIn, bone matrices (t3),
//           and the entry points VSMain / VSMainSkin / VSMainInstancing, etc.
///////////////////////////////////////
#include "ModelVSCommon.h"

////////////////////////////////////////////////
// Vertex shader core (called by the VSMain* entry points in ModelVSCommon.h).
////////////////////////////////////////////////
SPSIn VSMainCore(SVSIn vsIn, float4x4 mWorldLocal, uniform bool isUsePreComputedVertexBuffer)
{
    SPSIn psIn;

    // Local space -> world space.
    float4 worldPos = CalcVertexPositionInWorldSpace(vsIn.pos, mWorldLocal, isUsePreComputedVertexBuffer);

    // World -> view -> projection (clip) space.
    // We're rendering from the light's camera here, so this clip-space
    // position IS the light-view-projection ("LVP") position.
    psIn.pos = mul(mView, worldPos);
    psIn.pos = mul(mProj, psIn.pos);

    // SV_POSITION gets perspective-divided by the rasterizer before the pixel
    // shader sees it, so we keep an untouched copy to divide ourselves below.
    psIn.posInLVP = psIn.pos;

    return psIn;
}

////////////////////////////////////////////////
// Pixel shader.
// The shadow render target is DXGI_FORMAT_R32_FLOAT (see ModelRender::Init),
// so we only need to output a single float, not a full float4.
////////////////////////////////////////////////
float PSMain(SPSIn psIn) : SV_Target0
{
    // Depth as seen from the light (closer to the light = smaller value).
    return psIn.posInLVP.z / psIn.posInLVP.w;
}
