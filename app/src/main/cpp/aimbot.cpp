#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <cmath>
#include <unistd.h>
#include <pthread.h>
#include <cstdint>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "s2aim", __VA_ARGS__)

struct Vec3 { float x, y, z; };

static bool       g_on     = false;
static bool       g_run    = true;
static float      g_smooth = 5.0f;
static float      g_fov    = 15.0f;
static uintptr_t  g_base   = 0;

// ============================================================
// TU WPISZ OFSETY Z DUMPA — Il2CppDumper na libil2cpp.so
// na razie placeholdery — aimbot nie ruszy bez prawdziwych
// ============================================================
namespace Off {
    constexpr uintptr_t GAME_STATE = 0x0;   // <-- wpisz RVA z dump.cs
    constexpr uintptr_t LOCAL      = 0xA0;
    constexpr uintptr_t LIST       = 0xA8;
    constexpr uintptr_t COUNT      = 0xB0;
    constexpr uintptr_t P_HEALTH   = 0x18;
    constexpr uintptr_t P_TEAM     = 0x1C;
    constexpr uintptr_t P_POS      = 0x20;
    constexpr uintptr_t P_VIEW     = 0x2C;
}

static inline Vec3  rdVec(uintptr_t p, uintptr_t o){ return *(Vec3*)(p+o); }
static inline void  wrVec(uintptr_t p, uintptr_t o, Vec3 v){ *(Vec3*)(p+o) = v; }
static inline int   rdInt(uintptr_t p, uintptr_t o){ return *(int*)(p+o); }

static Vec3 CalcAngle(Vec3 s, Vec3 d) {
    Vec3 o;
    float dx = d.x - s.x, dy = d.y - s.y, dz = d.z - s.z;
    float hyp = sqrtf(dx*dx + dy*dy);
    o.x = atan2f(-dz, hyp) * 57.2957795f;
    o.y = atan2f(dy,  dx)  * 57.2957795f;
    o.z = 0.0f;
    return o;
}

static void aimTick() {
    if (!g_base || !g_on) return;

    // jeśli offset GAME_STATE = 0 — nic nie rób (jeszcze nie dumpnięte)
    if (Off::GAME_STATE == 0x0) return;

    uintptr_t game = *(uintptr_t*)(g_base + Off::GAME_STATE);
    if (!game) return;

    uintptr_t local = *(uintptr_t*)(game + Off::LOCAL);
    uintptr_t list  = *(uintptr_t*)(game + Off::LIST);
    int       count = *(int*)    (game + Off::COUNT);
    if (!local || !list || count <= 0) return;

    int myTeam = rdInt(local, Off::P_TEAM);
    Vec3 eye   = rdVec(local, Off::P_POS);
    Vec3 cur   = rdVec(local, Off::P_VIEW);

    uintptr_t best = 0;
    float bestFov  = g_fov;

    for (int i = 0; i < count; i++) {
        uintptr_t ent = *(uintptr_t*)(list + i * 8);
        if (!ent || ent == local) continue;
        if (rdInt(ent, Off::P_HEALTH) <= 0) continue;
        if (rdInt(ent, Off::P_TEAM) == myTeam) continue;

        Vec3 pos  = rdVec(ent, Off::P_POS);
        Vec3 want = CalcAngle(eye, pos);

        float dy = fabsf(want.y - cur.y);
        float dp = fabsf(want.x - cur.x);
        float ang = sqrtf(dy*dy + dp*dp);
        if (ang < bestFov) { bestFov = ang; best = ent; }
    }

    if (!best) return;

    Vec3 want = CalcAngle(eye, rdVec(best, Off::P_POS));
    cur.x += (want.x - cur.x) / g_smooth;
    cur.y += (want.y - cur.y) / g_smooth;

    if (cur.x >  89.0f) cur.x =  89.0f;
    if (cur.x < -89.0f) cur.x = -89.0f;

    wrVec(local, Off::P_VIEW, cur);
}

static void* aimThread(void*) {
    g_base = (uintptr_t)dlopen("libil2cpp.so", RTLD_NOW);
    LOG("libil2cpp base = %p", (void*)g_base);

    while (g_run) {
        aimTick();
        usleep(8000); // ~125 Hz
    }
    return nullptr;
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void*) {
    LOG("JNI_OnLoad — lib załadowana");
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_fsociety_s2aim_MainActivity_nativeVersion(JNIEnv* e, jobject) {
    return e->NewStringUTF("0.2 — aimbot live");
}

extern "C" JNIEXPORT void JNICALL
Java_com_fsociety_s2aim_MainActivity_nativeStart(JNIEnv*, jobject) {
    static bool started = false;
    if (started) return;
    started = true;

    pthread_t t;
    pthread_create(&t, nullptr, aimThread, nullptr);
    LOG("aim thread started");
}

extern "C" JNIEXPORT void JNICALL
Java_com_fsociety_s2aim_MainActivity_nativeStop(JNIEnv*, jobject) {
    g_run = false;
    g_on  = false;
    LOG("aim thread stopping");
}

extern "C" JNIEXPORT void JNICALL
Java_com_fsociety_s2aim_MainActivity_nativeSetAim(JNIEnv*, jobject, jboolean on) {
    g_on = (bool)on;
    LOG("aim = %d", (int)g_on);
}

extern "C" JNIEXPORT void JNICALL
Java_com_fsociety_s2aim_MainActivity_nativeSetSmooth(JNIEnv*, jobject, jfloat s) {
    g_smooth = (s < 1.0f) ? 1.0f : s;
}

extern "C" JNIEXPORT void JNICALL
Java_com_fsociety_s2aim_MainActivity_nativeSetFov(JNIEnv*, jobject, jfloat f) {
    g_fov = (f < 1.0f) ? 1.0f : f;
}
