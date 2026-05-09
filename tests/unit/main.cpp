#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <immintrin.h>

#include "er2/unity2/gom/gom_hash_calc.hpp"
#include "er2/unity2/camera/world_to_screen.hpp"
#include "er2/unity2/transform/transform.hpp"

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { ++g_passed; } else { ++g_failed; std::printf("FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); } \
} while(0)

#define CHECK_FLOAT_EQ(a, b, eps, msg) CHECK(std::fabs((a) - (b)) < (eps), msg)

// ---------- CalHashmaskThrougTag ----------

void TestCalHashmaskThrougTag()
{
    // Tag 0 (Untagged) should produce a deterministic hash
    std::uint32_t h0 = er2::CalHashmaskThrougTag(0);
    CHECK((h0 & 3u) == 0, "CalHashmaskThrougTag(0) aligned to 4");

    // Tag 1 (MainCamera, tag=5 in Unity actually; tag=1 is just "Respawn")
    std::uint32_t h1 = er2::CalHashmaskThrougTag(1);
    CHECK((h1 & 3u) == 0, "CalHashmaskThrougTag(1) aligned to 4");

    // Different tags should produce different hashes (with very high probability)
    CHECK(h0 != h1, "CalHashmaskThrougTag(0) != CalHashmaskThrougTag(1)");

    // Same tag should produce the same hash (deterministic)
    CHECK(er2::CalHashmaskThrougTag(5) == er2::CalHashmaskThrougTag(5), "CalHashmaskThrougTag deterministic");

    // Tag 5 is MainCamera tag -> known seed value used in scan_chain.hpp
    std::uint32_t h5 = er2::CalHashmaskThrougTag(5);
    CHECK(h5 == 0x01F266ECu, "CalHashmaskThrougTag(5) == kSeedHashMask");

    std::printf("[hash] passed\n");
}

// ---------- WorldToScreenPoint ----------

void TestWorldToScreenPoint()
{
    // Identity view-proj, object at origin -> center of screen
    glm::mat4 identity(1.0f);
    er2::ScreenRect screen{0.0f, 0.0f, 1920.0f, 1080.0f};
    
    auto r = er2::WorldToScreenPoint(identity, screen, glm::vec3(0.0f, 0.0f, 0.0f));
    CHECK_FLOAT_EQ(r.x, 960.0f, 1.0f, "W2S origin -> center X");
    CHECK_FLOAT_EQ(r.y, 540.0f, 1.0f, "W2S origin -> center Y");
    CHECK(r.visible, "W2S origin visible");

    // Object behind camera (w <= 0) should not be visible
    // Use a simple projection where z maps to w
    glm::mat4 proj(0.0f);
    proj[0][0] = 1.0f;
    proj[1][1] = 1.0f;
    proj[2][3] = 1.0f;  // w = z
    proj[3][2] = 1.0f;

    auto r2 = er2::WorldToScreenPoint(proj, screen, glm::vec3(0.0f, 0.0f, -1.0f));
    CHECK(!r2.visible, "W2S behind camera not visible");

    std::printf("[w2s] passed\n");
}

// ---------- QuatRotateSIMD ----------

void TestQuatRotateSIMD()
{
    // Identity quaternion (0,0,0,1) should not change the vector
    __m128 qIdentity = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f); // w=1 in [3], but layout is x,y,z,w
    // Actually the quaternion layout used in er2: {x, y, z, w} in the SIMD register
    // Identity = (0, 0, 0, 1) -> x=0, y=0, z=0, w=1
    __m128 qId = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f); // _mm_set_ps is (w, z, y, x) -> stored as x,y,z,w
    __m128 v = _mm_set_ps(0.0f, 3.0f, 2.0f, 1.0f);    // (1, 2, 3, 0)

    __m128 result = er2::QuatRotateSIMD(qId, v);
    float out[4];
    _mm_storeu_ps(out, result);
    CHECK_FLOAT_EQ(out[0], 1.0f, 0.001f, "QuatRotate identity x");
    CHECK_FLOAT_EQ(out[1], 2.0f, 0.001f, "QuatRotate identity y");
    CHECK_FLOAT_EQ(out[2], 3.0f, 0.001f, "QuatRotate identity z");

    // 90-degree rotation around Z axis: q = (0, 0, sin(45), cos(45)) = (0, 0, 0.7071, 0.7071)
    float s = 0.70710678f;
    __m128 qZ90 = _mm_set_ps(s, s, 0.0f, 0.0f); // x=0, y=0, z=sin45, w=cos45
    __m128 vx = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f); // (1, 0, 0)

    __m128 rz = er2::QuatRotateSIMD(qZ90, vx);
    float outz[4];
    _mm_storeu_ps(outz, rz);
    // 90 deg around Z: (1,0,0) -> (0,1,0)
    CHECK_FLOAT_EQ(outz[0], 0.0f, 0.01f, "QuatRotate Z90 x");
    CHECK_FLOAT_EQ(outz[1], 1.0f, 0.01f, "QuatRotate Z90 y");
    CHECK_FLOAT_EQ(outz[2], 0.0f, 0.01f, "QuatRotate Z90 z");

    std::printf("[quat] passed\n");
}

int main()
{
    TestCalHashmaskThrougTag();
    TestWorldToScreenPoint();
    TestQuatRotateSIMD();

    std::printf("\nResults: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed > 0 ? 1 : 0;
}
