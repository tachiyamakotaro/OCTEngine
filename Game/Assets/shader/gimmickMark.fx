/*!
 * @brief ギミックの印（受光器の電球マーク・回せる反射鏡の歯車・動かせる反射鏡のレール）
 *
 * 白いテクスチャの薄い箱（solar_panel.tkm）を置き、形と色はここで作る。
 * 形はテクスチャではなく「印の中心からの距離」で作る（大きさを変えても形がくずれないように）。
 * 電球と歯車は、形の外側を clip で捨てるので、板の四角は見えない。
 *
 * model.fx を include して、頂点シェーダーとライトの計算はそのまま使う。
 * テクスチャは真っ白なので、CalcLitColor の結果は「ライト × 影」の明るさになる。
 * 印の値は、model.fx の DirectionLightCb（b1）の最後に足す（MODEL_CB_EXTRA）。
 * C++ 側は GimmickMarkRenderer::MarkValues（ライトの値の後ろにつなげた構造体）。
 *
 * エントリーポイント
 *   PSMainGimmickMark
 */

// model.fx の DirectionLightCb の最後に足す値（C++ の GimmickMarkRenderer::MarkValues と同じ並び）
//   markColor     : rgb = 地の色, a = 自分で光る強さ
//   markDarkColor : rgb = 縁取り・暗い所の色, a = 形（0 = 箱, 1 = 電球, 2 = 歯車, 3 = レール）
//   markCenter    : xyz = 印の中心（ワールド座標）, w = 印の大きさの半分（レールは高さの半分）
//   markParam     : 電球 … y = 光り具合（0〜1）
//                   歯車 … x = 回転（ラジアン）, w = 歯の数
//                   レール … z = 長さの半分, w = 枕木の間隔
//   markLitColor  : rgb = 光っているときの色, a = 光っているときに自分で光る強さ（電球だけ使う）
// （fxc は複数行のマクロの中のコメントをうまく読めないことがあるので、マクロは1行で書く）
#define MODEL_CB_EXTRA float4 markColor; float4 markDarkColor; float4 markCenter; float4 markParam; float4 markLitColor;

#include "model.fx"


// 形の番号（C++ の GimmickMarkRenderer::Shape と同じ）
static const int SHAPE_SOLID = 0;
static const int SHAPE_BULB = 1;
static const int SHAPE_GEAR = 2;
static const int SHAPE_RAIL = 3;

// ---- 電球（印の大きさの半分を 1 とした座標。電球は -0.5〜0.5 くらい、光の線は 0.97 くらいまで）----
// ガラスの玉の中心と半径
static const float2 BULB_GLOBE_CENTER = float2(0.0f, 0.2f);
static const float BULB_GLOBE_RADIUS = 0.4f;
// 口金（ねじの所）の中心と大きさの半分
static const float2 BULB_BASE_CENTER = float2(0.0f, -0.34f);
static const float2 BULB_BASE_HALF = float2(0.18f, 0.18f);
// ガラスと口金の境目（これより下は口金）
static const float BULB_BASE_TOP = -0.16f;
// 口金の先の丸
static const float2 BULB_TIP_CENTER = float2(0.0f, -0.55f);
static const float BULB_TIP_RADIUS = 0.09f;
// 縁取りの太さ
static const float BULB_OUTLINE_WIDTH = 0.07f;
// 口金の色・ねじの溝の色
static const float3 BULB_BASE_COLOR = float3(0.62f, 0.62f, 0.66f);
static const float3 BULB_BASE_GROOVE_COLOR = float3(0.32f, 0.32f, 0.36f);
// フィラメントの色（消えているとき・光っているとき）と、光っているときの強さ
static const float3 FILAMENT_COLOR = float3(0.55f, 0.42f, 0.25f);
static const float3 FILAMENT_LIT_COLOR = float3(1.0f, 0.72f, 0.3f);
static const float FILAMENT_LIT_GLOW = 2.0f;
// 光の線（光っているときだけ出る）
static const float RAY_INNER = 0.54f;
static const float RAY_OUTER = 0.72f;
static const float RAY_WIDTH = 0.045f;

// ---- 歯車（印の大きさの半分を 1 とした座標）----
static const float GEAR_ROOT_RADIUS = 0.74f;	// 歯の根元
static const float GEAR_TIP_RADIUS = 0.97f;		// 歯の先
static const float GEAR_HOLE_RADIUS = 0.13f;	// 肉ぬきの穴
static const float GEAR_HOLE_DISTANCE = 0.46f;	// 中心から肉ぬきの穴まで
static const float GEAR_HUB_RADIUS = 0.22f;		// 軸のまわりの輪
static const float GEAR_AXLE_RADIUS = 0.1f;		// 軸
static const float GEAR_OUTLINE_WIDTH = 0.07f;

// ---- レール（高さの半分を 1 とした座標）----
static const float RAIL_GROOVE_HALF = 0.42f;	// 真ん中のみぞの太さの半分
static const float RAIL_EDGE = 0.86f;			// これより外は縁（暗い）
static const float RAIL_TIE_HALF = 0.12f;		// 枕木の太さの半分（枕木の間隔に対する割合）


// ---------------------------------------------------------------------------
// 距離の関数（負なら形の内側）
// ---------------------------------------------------------------------------
float SdCircle(float2 p, float r)
{
    return length(p) - r;
}

float SdBox(float2 p, float2 halfSize)
{
    float2 d = abs(p) - halfSize;
    return length(max(d, 0.0f)) + min(max(d.x, d.y), 0.0f);
}

float SdSegment(float2 p, float2 a, float2 b)
{
    float2 pa = p - a;
    float2 ba = b - a;
    float h = saturate(dot(pa, ba) / dot(ba, ba));
    return length(pa - ba * h);
}

// なめらかにつなぐ min（電球のガラスと口金をつなぐ）
float SmoothMin(float a, float b, float k)
{
    float h = saturate(0.5f + 0.5f * (b - a) / k);
    return lerp(b, a, h) - k * h * (1.0f - h);
}

// 内側なら 1（縁は1ピクセルぶんぼかして、ちらつかないようにする）
float Inside(float d, float aa)
{
    return 1.0f - smoothstep(-aa, aa, d);
}

// 2Dで回す
float2 Rotate2D(float2 p, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return float2(c * p.x - s * p.y, s * p.x + c * p.y);
}


// ---------------------------------------------------------------------------
// 電球
// ---------------------------------------------------------------------------
void ShadeBulb(float2 q, float aa, out float3 albedo, out float3 emissive)
{
    float glowRate = saturate(markParam.y);

    // 形：ガラスの玉 + 口金（なめらかにつなぐ）+ 先の丸
    float globe = SdCircle(q - BULB_GLOBE_CENTER, BULB_GLOBE_RADIUS);
    float base = SdBox(q - BULB_BASE_CENTER, BULB_BASE_HALF);
    float body = SmoothMin(globe, base, 0.12f);
    float tip = SdCircle(q - BULB_TIP_CENTER, BULB_TIP_RADIUS);
    float bulb = min(body, tip);

    // 光の線（光っているときだけ。ガラスの玉のまわりに7本）
    float rays = 1.0e5f;
    if (glowRate > 0.001f)
    {
        [unroll]
        for (int i = 0; i < 7; ++i)
        {
            float angle = radians(-15.0f + 35.0f * i);
            float2 dir = float2(cos(angle), sin(angle));
            rays = min(rays, SdSegment(q - BULB_GLOBE_CENTER, dir * RAY_INNER, dir * RAY_OUTER) - RAY_WIDTH);
        }
    }

    // 縁取りの外側は捨てる（板の四角は見せない）
    float outer = min(bulb - BULB_OUTLINE_WIDTH, rays - BULB_OUTLINE_WIDTH * 0.6f);
    clip(-outer);

    // ガラス：消えているときは白っぽく少し光る → 光っているときは強く光る
    float3 glassColor = lerp(markColor.rgb, markLitColor.rgb, glowRate);
    float glassGlow = lerp(markColor.a, markLitColor.a, glowRate);

    // 口金：灰色に、ねじの溝を2本
    float groove = max(
        Inside(abs(q.y + 0.27f) - 0.025f, aa),
        Inside(abs(q.y + 0.39f) - 0.025f, aa));
    float3 baseColor = lerp(BULB_BASE_COLOR, BULB_BASE_GROOVE_COLOR, groove);
    float isBase = Inside(q.y - BULB_BASE_TOP, aa);

    float3 inner = lerp(glassColor, baseColor, isBase);
    float3 innerGlow = glassColor * glassGlow * (1.0f - isBase);

    // 先の丸は縁取りと同じ暗い色
    float isTip = Inside(tip, aa) * (1.0f - Inside(body, aa));
    inner = lerp(inner, markDarkColor.rgb, isTip);
    innerGlow *= 1.0f - isTip;

    // フィラメント（ガラスの中の「M」の形）
    float2 p0 = float2(-0.14f, -0.12f);
    float2 p1 = float2(-0.14f, 0.18f);
    float2 p2 = float2(-0.07f, 0.31f);
    float2 p3 = float2(0.0f, 0.18f);
    float2 p4 = float2(0.07f, 0.31f);
    float2 p5 = float2(0.14f, 0.18f);
    float2 p6 = float2(0.14f, -0.12f);
    float filamentDist = min(min(min(SdSegment(q, p0, p1), SdSegment(q, p1, p2)), min(SdSegment(q, p2, p3), SdSegment(q, p3, p4))),
                             min(SdSegment(q, p4, p5), SdSegment(q, p5, p6)));
    float filament = Inside(filamentDist - 0.03f, aa) * (1.0f - isBase);
    float3 filamentColor = lerp(FILAMENT_COLOR, FILAMENT_LIT_COLOR, glowRate);
    inner = lerp(inner, filamentColor, filament);
    innerGlow = lerp(innerGlow, FILAMENT_LIT_COLOR * FILAMENT_LIT_GLOW * glowRate, filament);

    // 縁取り → 電球の中身 → 光の線 の順に重ねる
    float inBulb = Inside(bulb, aa);
    float inRay = Inside(rays, aa) * (1.0f - Inside(bulb - BULB_OUTLINE_WIDTH, aa));
    albedo = lerp(markDarkColor.rgb, inner, inBulb);
    emissive = innerGlow * inBulb;
    albedo = lerp(albedo, markLitColor.rgb, inRay);
    emissive += markLitColor.rgb * markLitColor.a * glowRate * inRay;
}


// ---------------------------------------------------------------------------
// 歯車（鏡と一緒に回る）
// ---------------------------------------------------------------------------
void ShadeGear(float2 q, float aa, out float3 albedo, out float3 emissive)
{
    // 鏡の角度だけ回す（模様のほうを逆に回して調べる）
    float2 r = Rotate2D(q, -markParam.x);
    float radius = length(r);
    float theta = atan2(r.y, r.x);

    // 歯：cos の山を台形にして、歯の先の半径を決める
    float teeth = max(markParam.w, 3.0f);
    float tooth = smoothstep(-0.3f, 0.3f, cos(theta * teeth));
    float outerRadius = lerp(GEAR_ROOT_RADIUS, GEAR_TIP_RADIUS, tooth);
    float gear = radius - outerRadius;

    // 肉ぬきの穴（3つ。45度回すと見た目が変わるので、回ったことがわかる）
    float holes = 1.0e5f;
    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        float angle = radians(90.0f + 120.0f * i);
        holes = min(holes, SdCircle(r - float2(cos(angle), sin(angle)) * GEAR_HOLE_DISTANCE, GEAR_HOLE_RADIUS));
    }

    // 歯車の外と穴の中は捨てる（奥の壁が見える）
    clip(-gear);
    clip(holes);

    // 縁取り（外側・穴のまわり）、軸のまわりの輪、軸
    float outline = max(1.0f - Inside(gear + GEAR_OUTLINE_WIDTH, aa), Inside(holes - GEAR_OUTLINE_WIDTH * 0.7f, aa));
    float hubRing = Inside(abs(radius - GEAR_HUB_RADIUS) - 0.035f, aa);
    float axle = Inside(radius - GEAR_AXLE_RADIUS, aa);

    float3 color = markColor.rgb;
    // 歯の根元へ向かって少し暗くして、立体っぽくする
    color *= lerp(0.85f, 1.0f, saturate((radius - GEAR_HUB_RADIUS) / (GEAR_ROOT_RADIUS - GEAR_HUB_RADIUS)));
    color = lerp(color, markDarkColor.rgb, max(max(outline, hubRing), axle));

    albedo = color;
    emissive = albedo * markColor.a;
}


// ---------------------------------------------------------------------------
// レール（上下の縁・真ん中のみぞ・枕木）
// ---------------------------------------------------------------------------
void ShadeRail(float2 local, float v, float tieCoord, float3 railAA, out float3 albedo, out float3 emissive)
{
    // railAA : x = v の1ピクセルぶん, y = tieCoord の1ピクセルぶん, z = local.x の1ピクセルぶん
    //（fwidth は if の中で使うとコンパイルできないことがあるので、呼ぶ前に計算しておく）
    float halfLength = max(markParam.z, 0.01f);

    // 真ん中のみぞ（暗い）と、みぞの中の枕木（明るい）
    float groove = Inside(abs(v) - RAIL_GROOVE_HALF, railAA.x);
    float tieDistance = abs(frac(tieCoord) - 0.5f);
    float tie = Inside(RAIL_TIE_HALF - tieDistance, railAA.y) * groove;

    // 上下の縁（暗い）と、両端の縁
    float edge = max(1.0f - Inside(abs(v) - RAIL_EDGE, railAA.x), 1.0f - Inside(abs(local.x) - (halfLength - 1.5f), railAA.z));

    float3 color = markColor.rgb;
    color = lerp(color, markDarkColor.rgb, groove);
    color = lerp(color, markColor.rgb * 0.8f, tie);
    color = lerp(color, markDarkColor.rgb, edge);

    albedo = color;
    emissive = albedo * markColor.a;
}


float4 PSMainGimmickMark(SPSIn In) : SV_Target0
{
    // ライト × 影の明るさ（テクスチャが真っ白なので、色は付いていない）
    float4 lit = CalcLitColor(In, true);

    // 印の中心からの位置（印の大きさの半分を 1 にする）
    float2 local = In.worldPos.xy - markCenter.xy;
    float2 q = local / max(markCenter.w, 0.01f);

    // 1ピクセルぶんの長さ（縁をぼかす幅）。fwidth は if の外で計算する
    float aa = max(fwidth(q.x), fwidth(q.y)) * 0.75f + 1.0e-4f;
    float v = q.y;
    float tieCoord = (local.x + max(markParam.z, 0.01f)) / max(markParam.w, 1.0f);
    float3 railAA = float3(fwidth(v), fwidth(tieCoord), fwidth(local.x)) + 1.0e-4f;

    int shape = (int)round(markDarkColor.a);
    float3 albedo = markColor.rgb;
    float3 emissive = markColor.rgb * markColor.a;
    if (shape == SHAPE_BULB)
    {
        ShadeBulb(q, aa, albedo, emissive);
    }
    else if (shape == SHAPE_GEAR)
    {
        ShadeGear(q, aa, albedo, emissive);
    }
    else if (shape == SHAPE_RAIL)
    {
        ShadeRail(local, v, tieCoord, railAA, albedo, emissive);
    }

    // a は model.fx と同じく「カメラからの距離」のまま返す
    return float4(albedo * lit.rgb + emissive, lit.a);
}
