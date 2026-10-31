// slime-chunk.cpp : スライムチャンク ファインダー (Java版) のエントリ ポイントを定義します。
//
// 【組み込み方】
//   Visual Studio の「デスクトップ アプリケーション」テンプレートで作成した
//   slime-chunk プロジェクトの slime-chunk.cpp を、このファイルの内容に置き換えるだけで動きます。
//   メニューはコードで作るため、リソース(.rc)の編集は不要です。
//   (文字コードは「UTF-8 (BOM 付き)」で保存されています)

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"
#include "slime-chunk.h"
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <wctype.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define MAX_LOADSTRING 100
#define APP_VERSION L"1.5.0"

// このアプリ専用のコマンドID (resource.h の値と重ならないよう 5000 番台を使います)
enum {
    IDM_FILE_SETPATH = 5001, IDM_FILE_SAVENOW, IDM_FILE_OPENDIR, IDM_FILE_AUTOSAVE, IDM_FILE_CLEARHIST, IDM_FILE_CLEARRES, IDM_FILE_SAVECSV,
    IDM_SIZE_1 = 5011, IDM_SIZE_2, IDM_SIZE_3, IDM_SIZE_4, IDM_SIZE_DENSE,
    IDM_APPVER = 5021, IDM_SET_RESET, IDM_SET_DARK, IDM_SET_LANG,
    IDM_HELP_USAGE = 5031, IDM_HELP_ABOUTSLIME,
    IDM_TOOL_CONVERT = 5041, IDM_TOOL_EXPORT, IDM_TOOL_IMPORT, IDM_TOOL_BATCH, IDM_TOOL_EXCLUDE,
    ID_SEED = 5101, ID_VER, ID_X, ID_Z, ID_R, ID_DK, ID_DM, ID_ISO, ID_GO, ID_STOP, ID_PROG, ID_STATUS,
    ID_LIST, ID_COPYXZ, ID_COPYTP, ID_OPEN, ID_COPYALL, ID_NOTE, ID_PATTERN, ID_SAVEPATH, ID_CHUNKS, ID_HIST, ID_HISTCLEAR, ID_CENTER,
    ID_FAVADD, ID_FAVVIEW, ID_PREVIEW, ID_CVBX, ID_CVBZ, ID_CVCX, ID_CVCZ,
    ID_NEAREST, ID_EXUSE, ID_EXX, ID_EXZ, ID_EXR, ID_EXNOTE, ID_LBL0 = 5200
};
// v1.5.0: メイン画面のサイズ選択ボタン / 半径プリセットボタン
enum { ID_SEG0 = 5301, ID_RP0 = 5311 };
#define TIMER_ID 1
#define WM_DONE (WM_APP + 1)

// =====================================================================
//  検索エンジン部分 (画面に依存しない)
// =====================================================================
typedef int64_t I64;
typedef int32_t I32;
static I64 SEED = 0;

// Java版のスライムチャンク判定 (バージョンによらず共通)
static inline int slimeAt(I32 x, I32 z)
{
    I64 s = SEED + (I64)(I32)((uint32_t)x * (uint32_t)x * 0x4c1906u) + (I64)(I32)((uint32_t)x * 0x5ac0dbu)
          + (I64)(I32)((uint32_t)z * (uint32_t)z) * 0x4307a7LL + (I64)(I32)((uint32_t)z * 0x5f24fu);
    s ^= 0x3ad8025fLL;
    uint64_t r = ((uint64_t)s ^ 0x5DEECE66DULL) & ((1ULL << 48) - 1);
    for (;;) {
        r = (r * 0x5DEECE66DULL + 0xBULL) & ((1ULL << 48) - 1);
        I32 bits = (I32)(r >> 17);
        I32 val = bits % 10;
        if ((I32)((uint32_t)bits - (uint32_t)val + 9u) >= 0) return val == 0;
    }
}

// K×K の外周1チャンクにスライムチャンクが無いか
static int isolatedSquare(I32 cx, I32 cz, int K)
{
    for (int a = -1; a <= K; a++)
        for (int b = -1; b <= K; b++) {
            if (a >= 0 && a < K && b >= 0 && b < K) continue;
            if (slimeAt(cx + a, cz + b)) return 0;
        }
    return 1;
}

struct Params {
    long long X, Z, R;   // 検索の中心と半径(ブロック)
    int K;               // 範囲の一辺 (チャンク)
    int thr;             // 範囲内に必要なスライムチャンク数 (K*K なら全部)
    int dense;           // 1=密集モード
    int onlyIso;         // 1=外周もきれいなものだけ
};
struct Res { I32 cx, cz; int count; int iso; double dist; double key; };

static Params P;
struct ExclZone { long long X, Z, R; wchar_t note[40]; };
static const int EXCL_MAX = 100;
static ExclZone g_excl[EXCL_MAX];
static int g_nexcl = 0;
static int g_useExclude = 0;
static bool isExcluded(double bx, double bz)
{
    if (!g_useExclude) return false;
    for (int i = 0; i < g_nexcl; i++) {
        double dx = bx - (double)g_excl[i].X, dz = bz - (double)g_excl[i].Z;
        if (dx * dx + dz * dz <= (double)g_excl[i].R * (double)g_excl[i].R) return true;
    }
    return false;
}
static const int KEEP = 5000;       // 保持する結果の最大数 (近い順 / 密集順)
static Res* g_heap = NULL; static int g_heapN = 0;
static Res* g_res = NULL;  static int g_nres = 0;
static CRITICAL_SECTION g_cs;
static bool g_csInit = false;
static volatile double g_worst = 1e300;
static volatile LONG g_full = 0, g_totalHits = 0, g_exactHits = 0, g_done = 0, g_nbands = 1, g_nextBand = 0, g_stop = 0;
static bool g_lastNearest = false;
static long long g_x0, g_z0, g_W, g_H;
static const int BAND = 128;

static void siftUp(int i)
{
    while (i > 0) {
        int p = (i - 1) / 2;
        if (g_heap[p].key >= g_heap[i].key) break;
        Res t = g_heap[p]; g_heap[p] = g_heap[i]; g_heap[i] = t; i = p;
    }
}
static void siftDown(int i)
{
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        if (l < g_heapN && g_heap[l].key > g_heap[m].key) m = l;
        if (r < g_heapN && g_heap[r].key > g_heap[m].key) m = r;
        if (m == i) break;
        Res t = g_heap[m]; g_heap[m] = g_heap[i]; g_heap[i] = t; i = m;
    }
}
static void heapPush(const Res& r)
{
    EnterCriticalSection(&g_cs);
    if (g_heapN < KEEP) {
        g_heap[g_heapN] = r; siftUp(g_heapN); g_heapN++;
        if (g_heapN == KEEP) { g_worst = g_heap[0].key; g_full = 1; }
    } else if (r.key < g_heap[0].key) {
        g_heap[0] = r; siftDown(0); g_worst = g_heap[0].key;
    }
    LeaveCriticalSection(&g_cs);
}

static void onHit(I32 cx, I32 cz, int count)
{
    const int K = P.K;
    const bool exact = (!P.dense && count >= K * K);           // K×K が全部そろっている
    if (!P.dense && P.onlyIso && !exact) return;
    double bx = cx * 16.0 + K * 8.0, bz = cz * 16.0 + K * 8.0;
    if (isExcluded(bx, bz)) return;
    double dist = hypot(bx - (double)P.X, bz - (double)P.Z);
    double key = -(double)count * 1e9 + dist;                   // 個数が多い順 → 近い順
    int iso = 0;
    if (exact && P.onlyIso) { iso = isolatedSquare(cx, cz, K); if (!iso) return; }
    InterlockedIncrement(&g_totalHits);
    if (exact) InterlockedIncrement(&g_exactHits);
    if (g_full && key >= g_worst) return;
    if (exact && !P.onlyIso) iso = isolatedSquare(cx, cz, K);
    Res r; r.cx = cx; r.cz = cz; r.count = count; r.iso = iso; r.dist = dist; r.key = key;
    heapPush(r);
}

static DWORD WINAPI bandWorker(LPVOID)
{
    const int K = P.K;
    const long long W = g_W, H = g_H;
    const long long Hwin = H - K + 1, Wwin = W - K + 1;
    unsigned char* g = (unsigned char*)malloc((size_t)W * (BAND + K - 1));
    short* col = (short*)malloc((size_t)W * sizeof(short));
    if (!g || !col) { free(g); free(col); return 1; }
    for (;;) {
        LONG bi = InterlockedIncrement(&g_nextBand) - 1;
        if (bi >= g_nbands || g_stop) break;
        long long r0 = (long long)bi * BAND;
        long long rows = BAND; if (r0 + rows > Hwin) rows = Hwin - r0;
        long long nr = rows + K - 1;
        for (long long j = 0; j < nr; j++)
            for (long long i = 0; i < W; i++)
                g[j * W + i] = (unsigned char)slimeAt((I32)(g_x0 + i), (I32)(g_z0 + r0 + j));
        for (long long i = 0; i < W; i++) col[i] = 0;
        for (int jj = 0; jj < K; jj++)
            for (long long i = 0; i < W; i++) col[i] = (short)(col[i] + g[(long long)jj * W + i]);
        for (long long j = 0; j < rows; j++) {
            if (j > 0)
                for (long long i = 0; i < W; i++)
                    col[i] = (short)(col[i] + g[(j + K - 1) * W + i] - g[(j - 1) * W + i]);
            int s = 0;
            for (int i = 0; i < K; i++) s += col[i];
            for (long long i = 0; i < Wwin; i++) {
                if (i > 0) s += col[i + K - 1] - col[i - 1];
                if (s >= P.thr) onHit((I32)(g_x0 + i), (I32)(g_z0 + r0 + j), s);
            }
        }
        InterlockedIncrement(&g_done);
    }
    free(g); free(col);
    return 0;
}

static int cmpKey(const void* a, const void* b)
{
    double d = ((const Res*)a)->key - ((const Res*)b)->key;
    return d < 0 ? -1 : (d > 0 ? 1 : 0);
}

// 検索本体 (同期処理)。結果は g_res / g_nres, 全ヒット数は g_totalHits。
static void searchCore()
{
    if (!g_csInit) { InitializeCriticalSection(&g_cs); g_csInit = true; }
    if (!g_heap) g_heap = (Res*)malloc(sizeof(Res) * KEEP);
    if (!g_res)  g_res  = (Res*)malloc(sizeof(Res) * KEEP);
    g_heapN = 0; g_full = 0; g_worst = 1e300; g_totalHits = 0; g_exactHits = 0; g_done = 0; g_nextBand = 0; g_nres = 0;

    const int K = P.K;
    I32 ccx = (I32)floor((double)P.X / 16.0), ccz = (I32)floor((double)P.Z / 16.0);
    long long rc = (P.R + 15) / 16; if (rc < K + 1) rc = K + 1;
    g_x0 = ccx - rc; g_z0 = ccz - rc; g_W = 2 * rc + 1; g_H = 2 * rc + 1;
    long long Hwin = g_H - K + 1;
    g_nbands = (LONG)((Hwin + BAND - 1) / BAND);

    SYSTEM_INFO si; GetSystemInfo(&si);
    int nt = (int)si.dwNumberOfProcessors; if (nt < 1) nt = 1; if (nt > 32) nt = 32; if (nt > g_nbands) nt = (int)g_nbands;
    HANDLE th[32]; int n = 0;
    for (int i = 0; i < nt; i++) { th[n] = CreateThread(NULL, 0, bandWorker, NULL, 0, NULL); if (th[n]) n++; }
    if (n) WaitForMultipleObjects(n, th, TRUE, INFINITE);
    for (int i = 0; i < n; i++) CloseHandle(th[i]);

    qsort(g_heap, g_heapN, sizeof(Res), cmpKey);
    if (!P.dense && g_exactHits > 0) {
        // 完全にそろった場所があるときは、それだけを近い順に残す
        int m = 0;
        for (int i = 0; i < g_heapN; i++) if (g_heap[i].count >= K * K) g_res[m++] = g_heap[i];
        g_nres = m;
    } else {
        // 密集モード / 完全一致が無かったときの「惜しい場所」: 重なっている範囲は、最も良いものだけ残す
        int m = 0;
        for (int i = 0; i < g_heapN; i++) {
            bool ok = true;
            for (int j = 0; j < m; j++)
                if (abs(g_heap[i].cx - g_res[j].cx) < K && abs(g_heap[i].cz - g_res[j].cz) < K) { ok = false; break; }
            if (ok) g_res[m++] = g_heap[i];
        }
        g_nres = m;
    }
}

// 「最も近い1件だけを一瞬で探す」モード用: 中心から外側へリング状に広げながら探し、
// 1件見つかった時点で、それより近い候補がもう存在しないと分かった時点で打ち切る。
static volatile LONG g_nearRing = 0, g_nearMaxRing = 1, g_nearFound = 0;
static bool nearestCheck(I32 cx, I32 cz, double& outDist, int& outCount, int& outIso)
{
    const int K = P.K;
    int c = 0;
    for (int a = 0; a < K; a++) for (int b = 0; b < K; b++) if (slimeAt(cx + a, cz + b)) c++;
    if (c < P.thr) return false;
    double bx = cx * 16.0 + K * 8.0, bz = cz * 16.0 + K * 8.0;
    if (isExcluded(bx, bz)) return false;
    int iso = 0;
    bool exact = (!P.dense && c >= K * K);
    if (!P.dense && P.onlyIso && !exact) return false;
    if (exact && P.onlyIso) { iso = isolatedSquare(cx, cz, K); if (!iso) return false; }
    else if (exact) iso = isolatedSquare(cx, cz, K);
    outDist = hypot(bx - (double)P.X, bz - (double)P.Z); outCount = c; outIso = iso;
    return true;
}
static void nearestOnlySearch()
{
    if (!g_res) g_res = (Res*)malloc(sizeof(Res) * KEEP);
    g_nres = 0; g_totalHits = 0; g_exactHits = 0;
    const int K = P.K;
    I32 ccx = (I32)floor((double)P.X / 16.0), ccz = (I32)floor((double)P.Z / 16.0);
    long long maxR = (P.R + 15) / 16; if (maxR < K + 1) maxR = K + 1;
    g_nearMaxRing = (LONG)maxR; g_nearRing = 0; g_nearFound = 0;
    bool found = false; double bestDist = 1e300; Res best;
    for (long long r = 0; r <= maxR; r++) {
        if (g_stop) break;
        g_nearRing = (LONG)r;
        if (found) {
            // このリング上のどの候補も、中心チャンクの縁までの距離 (r-1)*16 ブロック以上離れている。
            // それが既に見つけたベストより遠ければ、これ以上探しても更新されないので打ち切る。
            double minPossible = (double)(r > 0 ? (r - 1) : 0) * 16.0;
            if (minPossible > bestDist) break;
        }
        auto tryPos = [&](I32 cx, I32 cz) {
            double d; int c, iso;
            if (nearestCheck(cx, cz, d, c, iso)) {
                InterlockedIncrement(&g_totalHits);
                bool exact = (!P.dense && c >= K * K);
                if (P.dense || exact) InterlockedIncrement(&g_exactHits);
                if (!found || d < bestDist) {
                    found = true; bestDist = d;
                    best.cx = cx; best.cz = cz; best.count = c; best.iso = iso; best.dist = d;
                }
            }
        };
        if (r == 0) tryPos(ccx, ccz);
        else {
            for (long long dx = -r; dx <= r; dx++) { tryPos((I32)(ccx + dx), (I32)(ccz - r)); tryPos((I32)(ccx + dx), (I32)(ccz + r)); }
            for (long long dz = -r + 1; dz <= r - 1; dz++) { tryPos((I32)(ccx - r), (I32)(ccz + dz)); tryPos((I32)(ccx + r), (I32)(ccz + dz)); }
        }
    }
    if (found) { g_res[0] = best; g_nres = 1; }
    g_nearFound = found ? 1 : 0;
}

#ifndef SLIME_TEST
// =====================================================================
//  画面 (Win32)
// =====================================================================
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK TextWndProc(HWND, UINT, WPARAM, LPARAM);

static HWND hMain, hSeed, hVer, hX, hZ, hR, hDK, hDM, hIso, hGo, hStop, hProg, hStatus, hList;
static HWND hCopyXZ, hCopyTP, hOpen, hCopyAll, hNote, hPattern, hSavePath, hText, hTextEdit, hHist, hHistClear, hCenter;
static HWND hFavAdd, hFavView, hPreview;
static HWND hConv, hCvBX, hCvBZ, hCvOut1, hCvCX, hCvCZ, hCvOut2;
static HWND hNearest;
static HWND hSeg[5], hRp[4], hLblSeg, hLblRad, hTip;      // v1.5.0
static HWND g_hot = NULL;                                  // マウスが乗っているボタン
static int g_sortCol = -1; static bool g_sortAsc = true;   // 結果一覧の並べ替え状態
static const long long RP_VAL[4] = { 10000, 50000, 100000, 500000 };
static HWND hExcl, hExUse, hExX, hExZ, hExR, hExNote, hExAdd, hExRemove, hExList;
static HWND hExInfo, hExLblX, hExLblZ, hExLblR, hExLblNote;
static LRESULT CALLBACK ExclWndProc(HWND, UINT, WPARAM, LPARAM);
static void showExcludeList();
static void loadExclude();
static void saveExclude();
static HWND hBatch, hBatchInfo, hBatchSeeds, hBatchGo, hBatchStop, hBatchProg, hBatchStatus, hBatchList, hBatchCopyXZ, hBatchOpen, hBatchCopyAll;
struct BatchHit { wchar_t seed[40]; I32 cx, cz; int count, iso; double dist; };
static BatchHit* g_batch = NULL; static int g_nbatch = 0, g_batchCap = 0, g_batchShown = 0;
static volatile LONG g_batchStop = 0, g_batchRunning = 0, g_batchDoneSeeds = 0, g_batchTotalSeeds = 1;
static wchar_t g_batchVerKey[16], g_batchVerLabel[64];
#define WM_BATCH_DONE (WM_APP + 2)
static HWND hCvHdr1, hCvLblBX, hCvLblBZ, hCvHdr2, hCvLblCX, hCvLblCZ;
static LRESULT CALLBACK ConvWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK PreviewWndProc(HWND, UINT, WPARAM, LPARAM);
static bool g_viewFav = false;
static HWND hLbl[8], hChunks;
static HFONT hFont, hFontBold, hFontTitle, hFontSub;
static RECT g_card1, g_card2;
static HBRUSH g_brMint, g_brWhite, g_brOff;

// ---- 配色 (やわらかいスライムグリーン系) ----
// 色はライト/ダーク切り替えのため変数にしてあります (applyTheme が値を入れます)
static COLORREF C_MINT, C_WHITE, C_OFF, C_LINE, C_SHADOW, C_INK, C_MUTED, C_DEEP, C_TEAL, C_SLIME, C_ROW, C_SEL, C_FAV;
static bool g_dark = false;
// ---------------------------------------------------------------------
//  言語 (日本語 / English)
// ---------------------------------------------------------------------
static int g_lang = 0;   // 0 = 日本語, 1 = English
static inline const wchar_t* T(const wchar_t* ja, const wchar_t* en) { return g_lang == 0 ? ja : en; }

static void applyTheme(bool dark)
{
    g_dark = dark;
    if (!dark) {
        C_MINT = RGB(236, 245, 224); C_WHITE = RGB(255, 255, 255); C_OFF = RGB(240, 244, 236);
        C_LINE = RGB(207, 224, 190); C_SHADOW = RGB(222, 234, 210); C_INK = RGB(48, 62, 46);
        C_MUTED = RGB(110, 130, 104); C_DEEP = RGB(33, 105, 58); C_TEAL = RGB(16, 118, 122);
        C_SLIME = RGB(110, 201, 74); C_ROW = RGB(244, 250, 238); C_SEL = RGB(200, 236, 168);
        C_FAV = RGB(214, 158, 46);
    } else {
        C_MINT = RGB(21, 27, 19); C_WHITE = RGB(31, 39, 27); C_OFF = RGB(38, 47, 34);
        C_LINE = RGB(58, 74, 50); C_SHADOW = RGB(14, 18, 12); C_INK = RGB(224, 232, 214);
        C_MUTED = RGB(152, 168, 142); C_DEEP = RGB(142, 224, 122); C_TEAL = RGB(104, 214, 208);
        C_SLIME = RGB(122, 214, 88); C_ROW = RGB(38, 47, 34); C_SEL = RGB(58, 92, 42);
        C_FAV = RGB(232, 176, 62);
    }
}
// ダークモード時はタイトルバーも暗くする (Windows 10 1809 以降。非対応の環境では何も起きません)
static void applyTitleBar()
{
    if (!hMain) return;
    typedef HRESULT (WINAPI* DwmSetFn)(HWND, DWORD, LPCVOID, DWORD);
    HMODULE m = LoadLibraryW(L"dwmapi.dll"); if (!m) return;
    DwmSetFn f = (DwmSetFn)GetProcAddress(m, "DwmSetWindowAttribute");
    if (f) { BOOL on = g_dark ? TRUE : FALSE; f(hMain, 20, &on, sizeof on); f(hMain, 19, &on, sizeof on); }
    FreeLibrary(m);
}
static HMENU hMenu;
static int dpi = 96;
#define S(v) MulDiv((v), dpi, 96)

static const wchar_t* verNames[] = {
    L"Java 26.3", L"Java 26.2", L"Java 26.1", L"Java 1.21.9 - 1.21.11", L"Java 1.21.6 - 1.21.8", L"Java 1.21.5",
    L"Java 1.21.4", L"Java 1.21.2 - 1.21.3", L"Java 1.21 - 1.21.1", L"Java 1.20", L"Java 1.19.3 - 1.19.4",
    L"Java 1.19 - 1.19.2", L"Java 1.18", L"Java 1.17", L"Java 1.16", L"Java 1.15", L"Java 1.14", L"Java 1.13",
    L"Java 1.12", L"Java 1.11", L"Java 1.10", L"Java 1.9", L"Java 1.8", L"Java 1.7（リンクは1.8扱い）" };
static const wchar_t* verKeys[] = {
    L"26_3", L"26_2", L"26_1", L"1_21_9", L"1_21_6", L"1_21_5", L"1_21_4", L"1_21_2", L"1_21", L"1_20",
    L"1_19_3", L"1_19", L"1_18", L"1_17", L"1_16", L"1_15", L"1_14", L"1_13", L"1_12", L"1_11", L"1_10",
    L"1_9", L"1_8", L"1_8" };
#define NVER 24
static const wchar_t* verDisplayName(int i)
{
    if (i == NVER - 1) return T(L"Java 1.7（リンクは1.8扱い）", L"Java 1.7 (linked as 1.8)");
    return verNames[i];
}

static wchar_t g_seedStr[64], g_verKey[16], g_verLabel[64];
static DWORD g_t0;
static int g_shown = 0, g_running = 0;
static int g_mode = 2;                  // 0=1x1 1=2x2 2=3x3 3=4x4 4=密集
static int g_searchVer = 0;             // 検索したときに選んでいたバージョン番号
static int g_autoSave = 0;
static wchar_t g_savePath[MAX_PATH * 2] = L"";
#define MAX_SHOW 500

static const wchar_t* NOTE_TEXT_JA =
    L"※バージョンの指定はChunkbaseの表記に合わせています。実際にChunkbaseを開いて、シードとバージョンが正しく反映されているか確認してください。";
static const wchar_t* NOTE_TEXT_EN =
    L"Note: version labels follow Chunkbase's own naming. Please open Chunkbase and check the seed and version are reflected correctly.";
#define NOTE_TEXT T(NOTE_TEXT_JA, NOTE_TEXT_EN)

// ---------------------------------------------------------------------
//  文字列バッファ
// ---------------------------------------------------------------------
struct WBuf { wchar_t* p; size_t n, cap; };
static void wappend(WBuf& b, const wchar_t* fmt, ...)
{
    wchar_t tmp[768];
    va_list ap; va_start(ap, fmt);
    _vsnwprintf(tmp, 767, fmt, ap);
    va_end(ap);
    tmp[767] = 0;
    size_t l = wcslen(tmp);
    if (b.n + l + 1 > b.cap) {
        b.cap = (b.cap ? b.cap * 2 : 4096); while (b.cap < b.n + l + 1) b.cap *= 2;
        b.p = (wchar_t*)realloc(b.p, b.cap * sizeof(wchar_t));
    }
    memcpy(b.p + b.n, tmp, (l + 1) * sizeof(wchar_t)); b.n += l;
}

static void setStatus(const wchar_t* t);
static void patternText(wchar_t* out, size_t n, int mode, int K, int thr)
{
    if (mode == 4) _snwprintf(out, n, T(L"密集（%d×%d の範囲に %d個以上）", L"Dense (%d\u00d7%d box, %d+ chunks)"), K, K, thr);
    else if (K == 4) _snwprintf(out, n, T(L"4×4（とてもまれ。なければ惜しい場所を表示）", L"4\u00d74 (very rare \u2014 shows near-misses if none found)"));
    else if (K == 3) _snwprintf(out, n, T(L"3×3（なければ惜しい場所も表示）", L"3\u00d73 (shows near-misses if none found)"));
    else _snwprintf(out, n, L"%d\u00d7%d", K, K);
    out[n - 1] = 0;
}

static void centerOf(const Res& r, long long* bx, long long* bz)
{
    *bx = (long long)r.cx * 16 + P.K * 8;
    *bz = (long long)r.cz * 16 + P.K * 8;
}

static bool isExact(const Res& r) { return !P.dense && r.count >= P.K * P.K; }

// 結果のレポート文 (クリップボード/ファイル共通)
static void buildReport(WBuf& b, int maxItems)
{
    wchar_t pat[128];
    patternText(pat, 128, P.dense ? 4 : P.K - 1, P.K, P.thr);
    wappend(b, L"スライムチャンク検索結果\r\n");
    wappend(b, L"シード: %ls（%ls）\r\n", g_seedStr, g_verLabel);
    wappend(b, L"検索パターン: %ls\r\n", pat);
    wappend(b, L"探索: 中心(%lld, %lld) 半径%lldブロック\r\n", P.X, P.Z, P.R);
    {
        int cnt = maxItems < g_nres ? maxItems : g_nres;
        if (P.dense)
            wappend(b, L"ヒット数（全体）: %ld / 保存件数: %d（密集度が高い順）\r\n\r\n", (long)g_totalHits, cnt);
        else if (g_exactHits > 0)
            wappend(b, L"完全一致: %ld か所 / 保存件数: %d（中心に近い順）\r\n\r\n", (long)g_exactHits, cnt);
        else
            wappend(b, L"完全一致: 0 か所（見つかりませんでした）/ 惜しい候補: %d か所（個数が多い順）\r\n\r\n", cnt);
    }
    int n = g_nres < maxItems ? g_nres : maxItems;
    for (int i = 0; i < n; i++) {
        long long bx, bz; centerOf(g_res[i], &bx, &bz);
        if (!isExact(g_res[i]))
            wappend(b, L"%d. X %lld, Z %lld（%d×%d の範囲に %d個、距離約%.0f）\r\n", i + 1, bx, bz, P.K, P.K, g_res[i].count, g_res[i].dist);
        else
            wappend(b, L"%d. X %lld, Z %lld（%ls、距離約%.0f）\r\n", i + 1, bx, bz, g_res[i].iso ? L"外周もきれい" : L"隣接あり", g_res[i].dist);
        wappend(b, L"    チャンク X %d〜%d / Z %d〜%d　ブロック X %d〜%d / Z %d〜%d\r\n",
                g_res[i].cx, g_res[i].cx + P.K - 1, g_res[i].cz, g_res[i].cz + P.K - 1,
                g_res[i].cx * 16, (g_res[i].cx + P.K) * 16 - 1, g_res[i].cz * 16, (g_res[i].cz + P.K) * 16 - 1);
    }
}

// CSVの1フィールドを "..." で囲む (中に " があれば "" に、改行やカンマがあっても安全)
static void csvField(WBuf& b, const wchar_t* v)
{
    wappend(b, L"\"");
    for (const wchar_t* p = v; *p; p++) { if (*p == L'"') wappend(b, L"\"\""); else wappend(b, L"%c", *p); }
    wappend(b, L"\"");
}
static void buildCSV(WBuf& b, int maxItems)
{
    wappend(b, L"番号,中心X,中心Z,内容,中心からの距離,チャンクX開始,チャンクX終了,チャンクZ開始,チャンクZ終了\r\n");
    int n = g_nres < maxItems ? g_nres : maxItems;
    for (int i = 0; i < n; i++) {
        long long bx, bz; centerOf(g_res[i], &bx, &bz);
        wchar_t content[80];
        if (!isExact(g_res[i])) _snwprintf(content, 80, L"%d×%d の範囲に %d個", P.K, P.K, g_res[i].count);
        else _snwprintf(content, 80, L"%ls", g_res[i].iso ? L"外周もきれい" : L"隣接あり");
        content[79] = 0;
        wappend(b, L"%d,%lld,%lld,", i + 1, bx, bz);
        csvField(b, content);
        wappend(b, L",%.0f,%d,%d,%d,%d\r\n", g_res[i].dist, g_res[i].cx, g_res[i].cx + P.K - 1, g_res[i].cz, g_res[i].cz + P.K - 1);
    }
}
static bool writeUtf8File(const wchar_t* path, const wchar_t* text)
{
    int need = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
    char* u8 = (char*)malloc(need + 4);
    WideCharToMultiByte(CP_UTF8, 0, text, -1, u8, need, NULL, NULL);
    HANDLE f = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    bool ok = false;
    if (f != INVALID_HANDLE_VALUE) {
        DWORD w; const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
        WriteFile(f, bom, 3, &w, NULL);
        ok = WriteFile(f, u8, need - 1, &w, NULL) != 0;
        CloseHandle(f);
    }
    free(u8);
    return ok;
}
static bool saveToFile(const wchar_t* path)
{
    WBuf b = { 0, 0, 0 };
    buildReport(b, KEEP);
    bool ok = writeUtf8File(path, b.p);
    free(b.p);
    return ok;
}
static void doSaveCSV()
{
    if (g_nres == 0) { MessageBoxW(hMain, L"保存する検索結果がありません。先に検索してください。", L"CSVで保存", MB_OK | MB_ICONINFORMATION); return; }
    wchar_t f[MAX_PATH * 2] = L"slime_result.csv";
    OPENFILENAMEW o; ZeroMemory(&o, sizeof o);
    o.lStructSize = sizeof o; o.hwndOwner = hMain;
    o.lpstrFilter = L"CSVファイル (*.csv)\0*.csv\0すべてのファイル (*.*)\0*.*\0";
    o.lpstrFile = f; o.nMaxFile = MAX_PATH * 2; o.lpstrDefExt = L"csv";
    o.lpstrTitle = L"結果をCSVで保存";
    o.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetSaveFileNameW(&o)) return;
    WBuf b = { 0, 0, 0 }; buildCSV(b, KEEP);
    bool ok = writeUtf8File(f, b.p); free(b.p);
    wchar_t m[MAX_PATH * 2 + 40]; _snwprintf(m, MAX_PATH * 2 + 39, ok ? L"CSVを保存しました：%ls" : L"保存に失敗しました：%ls", f); m[MAX_PATH * 2 + 39] = 0;
    setStatus(m);
}

static void copyText(const wchar_t* t)
{
    size_t n = wcslen(t) + 1;
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, n * sizeof(wchar_t));
    if (!h) return;
    memcpy(GlobalLock(h), t, n * sizeof(wchar_t)); GlobalUnlock(h);
    if (OpenClipboard(hMain)) { EmptyClipboard(); SetClipboardData(CF_UNICODETEXT, h); CloseClipboard(); }
    else GlobalFree(h);
}
static void setStatus(const wchar_t* t) { SetWindowTextW(hStatus, t); }

// ---------------------------------------------------------------------
//  設定の保存 (%APPDATA%\SlimeChunkFinder\settings.ini)
// ---------------------------------------------------------------------
static void iniPath(wchar_t* out, size_t n)
{
    wchar_t dir[MAX_PATH] = L"";
    GetEnvironmentVariableW(L"APPDATA", dir, MAX_PATH);
    wcscat(dir, L"\\SlimeChunkFinder");
    CreateDirectoryW(dir, NULL);
    _snwprintf(out, n, L"%ls\\settings.ini", dir); out[n - 1] = 0;
}
static void iniGet(const wchar_t* ini, const wchar_t* key, const wchar_t* def, wchar_t* out, DWORD n)
{
    GetPrivateProfileStringW(L"main", key, def, out, n, ini);
}
static void iniSet(const wchar_t* ini, const wchar_t* key, const wchar_t* v) { WritePrivateProfileStringW(L"main", key, v, ini); }

static void setMode(int m);
static void updateSavePathLabel();
static void updateChunks();
static void loadHistory();
static void refillHistCombo();
static void setStatus(const wchar_t* t);
static void loadFavorites();
static void fillFavList();
static void setListColumnsForView(bool fav);
static void setFavButtonLabel();
static HMENU buildMenu();
static void showConverter();
static void applyLanguage();
static void showBatchSearch();
static LRESULT CALLBACK BatchWndProc(HWND, UINT, WPARAM, LPARAM);
static bool g_startMax = false;
static bool g_hasWinRect = false;
static RECT g_winRect;

static void loadSettings()
{
    wchar_t ini[MAX_PATH * 2], b[MAX_PATH * 2];
    iniPath(ini, MAX_PATH * 2);
    iniGet(ini, L"Seed", L"70392165441118850", b, 64); SetWindowTextW(hSeed, b);
    iniGet(ini, L"X", L"0", b, 32); SetWindowTextW(hX, b);
    iniGet(ini, L"Z", L"0", b, 32); SetWindowTextW(hZ, b);
    iniGet(ini, L"Radius", L"100000", b, 32); SetWindowTextW(hR, b);
    iniGet(ini, L"DenseK", L"5", b, 32); SetWindowTextW(hDK, b);
    iniGet(ini, L"DenseM", L"12", b, 32); SetWindowTextW(hDM, b);
    iniGet(ini, L"Version", L"0", b, 32); int vi = _wtoi(b); if (vi < 0 || vi >= NVER) vi = 0;
    SendMessage(hVer, CB_SETCURSEL, vi, 0);
    iniGet(ini, L"OnlyIso", L"0", b, 8); SendMessage(hIso, BM_SETCHECK, _wtoi(b) ? BST_CHECKED : BST_UNCHECKED, 0);
    iniGet(ini, L"AutoSave", L"0", b, 8); g_autoSave = _wtoi(b) ? 1 : 0;
    CheckMenuItem(hMenu, IDM_FILE_AUTOSAVE, MF_BYCOMMAND | (g_autoSave ? MF_CHECKED : MF_UNCHECKED));
    iniGet(ini, L"SavePath", L"", g_savePath, MAX_PATH * 2);
    iniGet(ini, L"Mode2", L"2", b, 8); int m = _wtoi(b); if (m < 0 || m > 4) m = 2;
    setMode(m);
    updateSavePathLabel();
    updateChunks();
    loadHistory();
    refillHistCombo();
    // 前回のウィンドウの位置と大きさ (実際にウィンドウへ反映するのは InitInstance 側。
    // ここではまだウィンドウが作られている最中 (WM_CREATE) なので、位置操作はしない)
    wchar_t w[4][16];
    iniGet(ini, L"WinL", L"", w[0], 16); iniGet(ini, L"WinT", L"", w[1], 16); iniGet(ini, L"WinW", L"", w[2], 16); iniGet(ini, L"WinH", L"", w[3], 16);
    g_hasWinRect = false;
    if (w[0][0] && w[2][0]) {
        RECT rc; rc.left = _wtoi(w[0]); rc.top = _wtoi(w[1]); rc.right = rc.left + _wtoi(w[2]); rc.bottom = rc.top + _wtoi(w[3]);
        if (rc.right - rc.left >= 600 && rc.bottom - rc.top >= 500 && rc.right - rc.left <= 8000 && rc.bottom - rc.top <= 8000
            && MonitorFromRect(&rc, MONITOR_DEFAULTTONULL)) {
            g_winRect = rc; g_hasWinRect = true;
        }
    }
    iniGet(ini, L"WinMax", L"0", b, 8); g_startMax = _wtoi(b) != 0;
    iniGet(ini, L"Dark", L"0", b, 8); applyTheme(_wtoi(b) != 0);
    iniGet(ini, L"Lang", L"0", b, 8); g_lang = _wtoi(b) != 0 ? 1 : 0;
    if (hMenu) CheckMenuItem(hMenu, IDM_SET_DARK, MF_BYCOMMAND | (g_dark ? MF_CHECKED : MF_UNCHECKED));
}
static void saveSettings()
{
    wchar_t ini[MAX_PATH * 2], b[MAX_PATH * 2];
    iniPath(ini, MAX_PATH * 2);
    GetWindowTextW(hSeed, b, 64); iniSet(ini, L"Seed", b);
    GetWindowTextW(hX, b, 32); iniSet(ini, L"X", b);
    GetWindowTextW(hZ, b, 32); iniSet(ini, L"Z", b);
    GetWindowTextW(hR, b, 32); iniSet(ini, L"Radius", b);
    GetWindowTextW(hDK, b, 32); iniSet(ini, L"DenseK", b);
    GetWindowTextW(hDM, b, 32); iniSet(ini, L"DenseM", b);
    _snwprintf(b, 32, L"%d", (int)SendMessage(hVer, CB_GETCURSEL, 0, 0)); iniSet(ini, L"Version", b);
    iniSet(ini, L"OnlyIso", SendMessage(hIso, BM_GETCHECK, 0, 0) == BST_CHECKED ? L"1" : L"0");
    iniSet(ini, L"AutoSave", g_autoSave ? L"1" : L"0");
    iniSet(ini, L"SavePath", g_savePath);
    _snwprintf(b, 32, L"%d", g_mode); iniSet(ini, L"Mode2", b);
    WINDOWPLACEMENT wp; ZeroMemory(&wp, sizeof wp); wp.length = sizeof wp;
    if (GetWindowPlacement(hMain, &wp)) {
        _snwprintf(b, 32, L"%ld", wp.rcNormalPosition.left); iniSet(ini, L"WinL", b);
        _snwprintf(b, 32, L"%ld", wp.rcNormalPosition.top); iniSet(ini, L"WinT", b);
        _snwprintf(b, 32, L"%ld", wp.rcNormalPosition.right - wp.rcNormalPosition.left); iniSet(ini, L"WinW", b);
        _snwprintf(b, 32, L"%ld", wp.rcNormalPosition.bottom - wp.rcNormalPosition.top); iniSet(ini, L"WinH", b);
        iniSet(ini, L"WinMax", wp.showCmd == SW_SHOWMAXIMIZED ? L"1" : L"0");
    }
    iniSet(ini, L"Dark", g_dark ? L"1" : L"0");
    iniSet(ini, L"Lang", g_lang ? L"1" : L"0");
}
// 設定ファイルをそのままエクスポート／インポートします
static void exportSettings()
{
    saveSettings();
    wchar_t src[MAX_PATH * 2]; iniPath(src, MAX_PATH * 2);
    wchar_t f[MAX_PATH * 2] = L"slime-chunk-settings.ini";
    OPENFILENAMEW o; ZeroMemory(&o, sizeof o);
    o.lStructSize = sizeof o; o.hwndOwner = hMain;
    o.lpstrFilter = L"設定ファイル (*.ini)\0*.ini\0すべてのファイル (*.*)\0*.*\0";
    o.lpstrFile = f; o.nMaxFile = MAX_PATH * 2; o.lpstrDefExt = L"ini";
    o.lpstrTitle = L"設定をエクスポート";
    o.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetSaveFileNameW(&o)) return;
    bool ok = CopyFileW(src, f, FALSE) != 0;
    wchar_t m[MAX_PATH * 2 + 40]; _snwprintf(m, MAX_PATH * 2 + 39, ok ? L"設定をエクスポートしました：%ls" : L"エクスポートに失敗しました：%ls", f); m[MAX_PATH * 2 + 39] = 0;
    setStatus(m);
}
static void importSettings()
{
    if (g_running) return;
    wchar_t f[MAX_PATH * 2] = L"";
    OPENFILENAMEW o; ZeroMemory(&o, sizeof o);
    o.lStructSize = sizeof o; o.hwndOwner = hMain;
    o.lpstrFilter = L"設定ファイル (*.ini)\0*.ini\0すべてのファイル (*.*)\0*.*\0";
    o.lpstrFile = f; o.nMaxFile = MAX_PATH * 2;
    o.lpstrTitle = L"設定をインポート";
    o.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetOpenFileNameW(&o)) return;
    if (MessageBoxW(hMain, L"今の設定・履歴・お気に入りを、選んだファイルの内容で置き換えます。よろしいですか？", L"設定のインポート", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
    wchar_t dst[MAX_PATH * 2]; iniPath(dst, MAX_PATH * 2);
    if (!CopyFileW(f, dst, FALSE)) { MessageBoxW(hMain, L"読み込みに失敗しました。", L"設定のインポート", MB_OK | MB_ICONERROR); return; }
    loadSettings(); loadHistory(); refillHistCombo(); loadFavorites();
    if (g_viewFav) fillFavList(); else ListView_DeleteAllItems(hList);
    if (hMenu) CheckMenuItem(hMenu, IDM_SET_DARK, MF_BYCOMMAND | (g_dark ? MF_CHECKED : MF_UNCHECKED));
    InvalidateRect(hMain, NULL, TRUE);
    setStatus(L"設定をインポートしました。");
}

// ---------------------------------------------------------------------
//  入力の読み取り
// ---------------------------------------------------------------------
static int parseSeed(const wchar_t* s, I64* out)
{
    while (*s == L' ') s++;
    size_t n = wcslen(s);
    while (n > 0 && s[n - 1] == L' ') n--;
    if (n == 0) return 0;
    wchar_t buf[64]; if (n >= 64) n = 63; wcsncpy(buf, s, n); buf[n] = 0;
    int numeric = 1; size_t i = 0; if (buf[0] == L'-') i = 1;
    if (i >= n) numeric = 0;
    for (size_t k = i; k < n; k++) if (buf[k] < L'0' || buf[k] > L'9') { numeric = 0; break; }
    if (numeric) {
        errno = 0; wchar_t* e; long long v = _wcstoi64(buf, &e, 10);
        if (errno != ERANGE) { *out = v; return 1; }
    }
    int32_t h = 0;   // Javaの String.hashCode と同じ (文字列シードの扱い)
    for (size_t k = 0; k < n; k++) h = (int32_t)((uint32_t)h * 31u + (uint32_t)buf[k]);
    *out = h; return 1;
}
static long long getNum(HWND h, long long def)
{
    wchar_t b[64]; GetWindowTextW(h, b, 64);
    if (!b[0]) return def;
    return _wtoi64(b);
}

// ---------------------------------------------------------------------
//  画面の状態
// ---------------------------------------------------------------------
static void setRunning(int r)
{
    g_running = r;
    EnableWindow(hGo, !r); EnableWindow(hStop, r);
    EnableWindow(hSeed, !r); EnableWindow(hVer, !r); EnableWindow(hX, !r); EnableWindow(hZ, !r); EnableWindow(hR, !r);
    EnableWindow(hIso, !r && g_mode != 4);
    EnableWindow(hDK, !r && g_mode == 4); EnableWindow(hDM, !r && g_mode == 4);
    EnableWindow(hNearest, !r);
    for (int i = 0; i < 5; i++) if (hSeg[i]) EnableWindow(hSeg[i], !r);
    for (int i = 0; i < 4; i++) if (hRp[i]) EnableWindow(hRp[i], !r);
    EnableWindow(hHist, !r); EnableWindow(hHistClear, !r);
    for (int id = IDM_SIZE_1; id <= IDM_SIZE_DENSE; id++) EnableMenuItem(hMenu, id, MF_BYCOMMAND | (r ? MF_GRAYED : MF_ENABLED));
}

static void setMode(int m)
{
    g_mode = m;
    CheckMenuRadioItem(hMenu, IDM_SIZE_1, IDM_SIZE_DENSE, IDM_SIZE_1 + m, MF_BYCOMMAND);
    wchar_t pat[128], b[200];
    if (m == 4) {
        int K = (int)getNum(hDK, 5), M = (int)getNum(hDM, 12);
        patternText(pat, 128, 4, K, M);
    } else patternText(pat, 128, m, m + 1, 0);
    _snwprintf(b, 200, T(L"検索パターン：%ls", L"Pattern: %ls"), pat); b[199] = 0;
    SetWindowTextW(hPattern, b);
    EnableWindow(hIso, !g_running && m != 4);
    EnableWindow(hDK, !g_running && m == 4); EnableWindow(hDM, !g_running && m == 4);
    for (int i = 0; i < 5; i++) if (hSeg[i]) InvalidateRect(hSeg[i], NULL, FALSE);
}

static void updateSavePathLabel()
{
    wchar_t b[MAX_PATH * 2 + 64];
    if (g_savePath[0]) _snwprintf(b, MAX_PATH * 2 + 63, T(L"保存先：%ls%ls", L"Save to: %ls%ls"), g_savePath, g_autoSave ? T(L"　（検索後に自動保存）", L"  (auto-saved after search)") : L"");
    else _snwprintf(b, MAX_PATH * 2 + 63, T(L"保存先：未設定（「ファイル」メニューから設定できます）", L"Save to: not set (set it from the File menu)"));
    b[MAX_PATH * 2 + 63] = 0;
    SetWindowTextW(hSavePath, b);
}

static bool chooseSavePath()
{
    wchar_t f[MAX_PATH * 2];
    wcscpy(f, g_savePath[0] ? g_savePath : L"slime_result.txt");
    OPENFILENAMEW o; ZeroMemory(&o, sizeof o);
    o.lStructSize = sizeof o; o.hwndOwner = hMain;
    o.lpstrFilter = L"テキストファイル (*.txt)\0*.txt\0すべてのファイル (*.*)\0*.*\0";
    o.lpstrFile = f; o.nMaxFile = MAX_PATH * 2; o.lpstrDefExt = L"txt";
    o.lpstrTitle = L"検索結果の保存先を指定";
    o.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetSaveFileNameW(&o)) return false;
    wcscpy(g_savePath, f);
    updateSavePathLabel();
    return true;
}

static void doSave(bool interactive)
{
    if (g_nres == 0) { if (interactive) MessageBoxW(hMain, L"保存する検索結果がありません。先に検索してください。", L"保存", MB_OK | MB_ICONINFORMATION); return; }
    if (!g_savePath[0] && !(interactive && chooseSavePath())) return;
    wchar_t b[MAX_PATH * 2 + 64];
    if (saveToFile(g_savePath)) _snwprintf(b, MAX_PATH * 2 + 63, L"結果を保存しました：%ls", g_savePath);
    else _snwprintf(b, MAX_PATH * 2 + 63, L"保存に失敗しました：%ls", g_savePath);
    b[MAX_PATH * 2 + 63] = 0;
    setStatus(b);
}

// ---------------------------------------------------------------------
//  検索の開始と完了
// ---------------------------------------------------------------------
static DWORD WINAPI searchThread(LPVOID)
{
    searchCore();
    PostMessageW(hMain, WM_DONE, 0, 0);
    return 0;
}
static DWORD WINAPI nearestOnlyThread(LPVOID)
{
    nearestOnlySearch();
    PostMessageW(hMain, WM_DONE, 0, 0);
    return 0;
}

// メイン画面の入力から検索パラメータを組み立てる (通常検索と複数シード検索の両方で使う)
static bool buildParamsFromUI(Params& p, wchar_t* err, size_t errN)
{
    p.dense = (g_mode == 4);
    if (p.dense) {
        p.K = (int)getNum(hDK, 5); p.thr = (int)getNum(hDM, 12);
        if (p.K < 2 || p.K > 16) { wcsncpy(err, T(L"「範囲の一辺」は 2〜16 の数字で指定してください。", L"\"Box size\" must be a number from 2 to 16."), errN - 1); err[errN - 1] = 0; return false; }
        if (p.thr < 1 || p.thr > p.K * p.K) {
            _snwprintf(err, errN, T(L"「最小個数」は 1〜%d の数字で指定してください。", L"\"Min count\" must be a number from 1 to %d."), p.K * p.K);
            err[errN - 1] = 0; return false;
        }
    } else p.K = g_mode + 1;
    p.X = getNum(hX, 0); p.Z = getNum(hZ, 0); p.R = getNum(hR, 100000);
    if (p.R < 64) p.R = 64; if (p.R > 1000000) p.R = 1000000;
    p.onlyIso = (!p.dense && SendMessage(hIso, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (!p.dense) {
        int full = p.K * p.K, slack = (p.K == 3) ? 2 : (p.K == 4) ? 6 : 0;
        p.thr = p.onlyIso ? full : full - slack;
    }
    return true;
}
static void startSearch()
{
    if (g_viewFav) { g_viewFav = false; setListColumnsForView(false); setFavButtonLabel(); }
    wchar_t sb[64]; GetWindowTextW(hSeed, sb, 64);
    I64 seed;
    if (!parseSeed(sb, &seed)) { MessageBoxW(hMain, T(L"シード値を入力してください。", L"Please enter a seed."), T(L"入力エラー", L"Input error"), MB_OK | MB_ICONWARNING); return; }
    Params p; wchar_t err[160];
    if (!buildParamsFromUI(p, err, 160)) { MessageBoxW(hMain, err, T(L"入力エラー", L"Input error"), MB_OK | MB_ICONWARNING); return; }
    P = p; SEED = seed;
    _snwprintf(g_seedStr, 64, L"%lld", (long long)seed);
    int vi = (int)SendMessage(hVer, CB_GETCURSEL, 0, 0); if (vi < 0) vi = 0;
    wcscpy(g_verKey, verKeys[vi]); wcscpy(g_verLabel, verNames[vi]); g_searchVer = vi;
    g_nres = 0; g_shown = 0; g_stop = 0; g_totalHits = 0; g_exactHits = 0; g_done = 0; g_nbands = 1;
    ListView_DeleteAllItems(hList); g_sortCol = -1;
    setRunning(1);
    g_t0 = GetTickCount();
    SendMessage(hProg, PBM_SETPOS, 0, 0);
    bool nearest = SendMessage(hNearest, BM_GETCHECK, 0, 0) == BST_CHECKED;
    g_lastNearest = nearest;
    if (nearest) {
        setStatus(T(L"最も近い1件を探しています…", L"Looking for the nearest spot\u2026"));
        SetTimer(hMain, TIMER_ID, 100, NULL);
        HANDLE th = CreateThread(NULL, 0, nearestOnlyThread, NULL, 0, NULL);
        if (th) CloseHandle(th);
    } else {
        setStatus(T(L"探索を開始しました…", L"Search started\u2026"));
        SetTimer(hMain, TIMER_ID, 150, NULL);
        HANDLE th = CreateThread(NULL, 0, searchThread, NULL, 0, NULL);
        if (th) CloseHandle(th);
    }
}

// ---- 結果一覧の並べ替え (列の見出しをクリック) ----
static int cmpSort(const void* a, const void* b)
{
    const Res* x = (const Res*)a; const Res* y = (const Res*)b; double d;
    switch (g_sortCol) {
    case 1: d = (double)x->cx - (double)y->cx; break;
    case 2: d = (double)x->cz - (double)y->cz; break;
    case 3: d = (double)y->count - (double)x->count; if (d == 0) d = (double)y->iso - (double)x->iso; break;
    default: d = x->dist - y->dist; break;
    }
    if (d == 0) d = x->dist - y->dist;
    if (!g_sortAsc) d = -d;
    return d < 0 ? -1 : d > 0 ? 1 : 0;
}
static void setSortArrow(int col, bool asc)
{
    HWND hh = ListView_GetHeader(hList); if (!hh) return;
    for (int i = 0; i < 6; i++) {
        HDITEMW h; ZeroMemory(&h, sizeof h); h.mask = HDI_FORMAT;
        if (!Header_GetItem(hh, i, &h)) continue;
        h.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
        if (i == col) h.fmt |= asc ? HDF_SORTUP : HDF_SORTDOWN;
        Header_SetItem(hh, i, &h);
    }
}

static void fillList()
{
    if (g_sortCol < 0) setSortArrow(-1, true);
    g_shown = g_nres < MAX_SHOW ? g_nres : MAX_SHOW;
    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    for (int i = 0; i < g_shown; i++) {
        wchar_t t[160]; long long bx, bz; centerOf(g_res[i], &bx, &bz);
        LVITEMW it; ZeroMemory(&it, sizeof it);
        it.mask = LVIF_TEXT; it.iItem = i;
        _snwprintf(t, 160, L"%d", i + 1); it.pszText = t;
        ListView_InsertItem(hList, &it);
        _snwprintf(t, 160, L"%lld", bx); ListView_SetItemText(hList, i, 1, t);
        _snwprintf(t, 160, L"%lld", bz); ListView_SetItemText(hList, i, 2, t);
        if (P.dense) _snwprintf(t, 160, T(L"%d×%d の範囲に %d個", L"%d\u00d7%d box, %d chunks"), P.K, P.K, g_res[i].count);
        else if (!isExact(g_res[i])) _snwprintf(t, 160, T(L"惜しい！ %d×%d 内に %d/%d個", L"Close! %d\u00d7%d has %d/%d"), P.K, P.K, g_res[i].count, P.K * P.K);
        else _snwprintf(t, 160, L"%ls", g_res[i].iso ? T(L"外周もきれい", L"Clean border") : T(L"隣にスライムチャンクあり", L"Adjacent chunk nearby"));
        ListView_SetItemText(hList, i, 3, t);
        _snwprintf(t, 160, T(L"約 %.0f", L"~%.0f"), g_res[i].dist); ListView_SetItemText(hList, i, 4, t);
        _snwprintf(t, 160, L"X %d\u301c%d / Z %d\u301c%d", g_res[i].cx, g_res[i].cx + P.K - 1, g_res[i].cz, g_res[i].cz + P.K - 1);
        ListView_SetItemText(hList, i, 5, t);
    }
    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    if (g_shown > 0) ListView_SetItemState(hList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

static void onColumnClick(int col)
{
    if (g_viewFav || g_running || g_shown < 2 || col < 0 || col > 5) return;
    if (col == g_sortCol) g_sortAsc = !g_sortAsc; else { g_sortCol = col; g_sortAsc = true; }
    qsort(g_res, g_shown, sizeof(Res), cmpSort);
    ListView_DeleteAllItems(hList);
    fillList();
    setSortArrow(g_sortCol, g_sortAsc);
}

static int selIndex()
{
    int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (i < 0 || i >= g_shown) { MessageBoxW(hMain, L"一覧から場所を選んでください。", L"選択してください", MB_OK | MB_ICONINFORMATION); return -1; }
    return i;
}

// ---------------------------------------------------------------------
//  レイアウト
// ---------------------------------------------------------------------
// 半径から「調べるチャンク数」を計算して表示する
static void fmtCommas(long long v, wchar_t* out, size_t n)
{
    wchar_t t[40]; _snwprintf(t, 40, L"%lld", v); t[39] = 0;
    size_t l = wcslen(t), o = 0;
    for (size_t i = 0; i < l && o + 2 < n; i++) {
        out[o++] = t[i];
        size_t rest = l - i - 1;
        if (rest > 0 && rest % 3 == 0) out[o++] = L',';
    }
    out[o] = 0;
}
static void updateChunks()
{
    if (!hChunks || !hR) return;
    long long R = getNum(hR, 100000);
    if (R < 64) R = 64; if (R > 1000000) R = 1000000;
    long long rc = (R + 15) / 16;
    long long n = (2 * rc + 1) * (2 * rc + 1);
    wchar_t c[48], ap[48], b[200];
    fmtCommas(n, c, 48);
    if (g_lang == 0) {
        if (n >= 100000000LL) _snwprintf(ap, 48, L"（約%.1f億）", n / 1e8);
        else if (n >= 10000LL) _snwprintf(ap, 48, L"（約%lld万）", n / 10000);
        else ap[0] = 0;
    } else {
        if (n >= 1000000000LL) _snwprintf(ap, 48, L"(~%.1fB)", n / 1e9);
        else if (n >= 1000000LL) _snwprintf(ap, 48, L"(~%.1fM)", n / 1e6);
        else if (n >= 1000LL) _snwprintf(ap, 48, L"(~%lldK)", n / 1000);
        else ap[0] = 0;
    }
    ap[47] = 0;
    _snwprintf(b, 200, T(L"調べるチャンク数：%ls %ls%ls", L"Chunks to scan: %ls %ls%ls"), c, ap,
        n >= 1000000000LL ? T(L"　※かなり時間がかかります", L"  \u2014 this may take a while") : L"");
    b[199] = 0;
    SetWindowTextW(hChunks, b);
}

// ---------------------------------------------------------------------
//  検索履歴 (%APPDATA%\SlimeChunkFinder\settings.ini の [history])
// ---------------------------------------------------------------------
struct HistEntry { wchar_t stamp[16]; wchar_t seed[40]; int ver, mode, K, M, iso; long long X, Z, R; long hits; int secs; };
static const int HIST_MAX = 30;
static HistEntry g_hist[HIST_MAX];
static int g_nhist = 0;

static void histToString(const HistEntry& h, wchar_t* out, size_t n)
{
    _snwprintf(out, n, L"%ls|%ls|%d|%d|%d|%d|%d|%lld|%lld|%lld|%ld|%d", h.stamp, h.seed, h.ver, h.mode, h.K, h.M, h.iso, h.X, h.Z, h.R, h.hits, h.secs);
    out[n - 1] = 0;
}
static bool histFromString(const wchar_t* s, HistEntry& h)
{
    wchar_t buf[600]; wcsncpy(buf, s, 599); buf[599] = 0;
    wchar_t* f[12]; int n = 0; wchar_t* p = buf; f[n++] = p;
    while (*p && n < 12) { if (*p == L'|') { *p = 0; f[n++] = p + 1; } p++; }
    if (n < 12) return false;
    ZeroMemory(&h, sizeof h);
    wcsncpy(h.stamp, f[0], 15); wcsncpy(h.seed, f[1], 39);
    h.ver = _wtoi(f[2]); h.mode = _wtoi(f[3]); h.K = _wtoi(f[4]); h.M = _wtoi(f[5]); h.iso = _wtoi(f[6]);
    h.X = _wtoi64(f[7]); h.Z = _wtoi64(f[8]); h.R = _wtoi64(f[9]); h.hits = _wtol(f[10]); h.secs = _wtoi(f[11]);
    if (h.ver < 0 || h.ver >= NVER || h.mode < 0 || h.mode > 4) return false;
    return true;
}
static void saveHistory()
{
    wchar_t ini[MAX_PATH * 2], b[640], k[16];
    iniPath(ini, MAX_PATH * 2);
    WritePrivateProfileStringW(L"history", NULL, NULL, ini);           // いったんこの項目を空にする
    if (g_nhist == 0) return;
    _snwprintf(b, 16, L"%d", g_nhist); WritePrivateProfileStringW(L"history", L"Count", b, ini);
    for (int i = 0; i < g_nhist; i++) {
        _snwprintf(k, 16, L"E%d", i);
        histToString(g_hist[i], b, 640);
        WritePrivateProfileStringW(L"history", k, b, ini);
    }
}
static void loadHistory()
{
    wchar_t ini[MAX_PATH * 2], b[640], k[16];
    iniPath(ini, MAX_PATH * 2);
    g_nhist = 0;
    int cnt = (int)GetPrivateProfileIntW(L"history", L"Count", 0, ini);
    if (cnt > HIST_MAX) cnt = HIST_MAX;
    for (int i = 0; i < cnt; i++) {
        _snwprintf(k, 16, L"E%d", i);
        GetPrivateProfileStringW(L"history", k, L"", b, 640, ini);
        HistEntry h;
        if (b[0] && histFromString(b, h)) g_hist[g_nhist++] = h;
    }
}
static void histLabel(const HistEntry& h, wchar_t* out, size_t n)
{
    wchar_t pat[48], sd[24], rr[32];
    if (h.mode == 4) _snwprintf(pat, 48, L"密集 %d×%d/%d個", h.K, h.K, h.M); else _snwprintf(pat, 48, L"%d×%d", h.mode + 1, h.mode + 1);
    pat[47] = 0;
    if (wcslen(h.seed) > 14) { wcsncpy(sd, h.seed, 12); sd[12] = L'…'; sd[13] = 0; } else wcscpy(sd, h.seed);
    fmtCommas(h.R, rr, 32);
    _snwprintf(out, n, L"%ls　%ls　シード %ls　中心(%lld, %lld)　半径 %ls　→ %ld か所", h.stamp, pat, sd, h.X, h.Z, rr, h.hits);
    out[n - 1] = 0;
}
static void refillHistCombo()
{
    if (!hHist) return;
    SendMessage(hHist, CB_RESETCONTENT, 0, 0);
    SendMessageW(hHist, CB_ADDSTRING, 0, (LPARAM)(g_nhist ? T(L"── 検索履歴（選ぶと条件を呼び出します）──", L"── Search history (pick to restore) ──") : T(L"── 検索履歴はまだありません ──", L"── No search history yet ──")));
    for (int i = 0; i < g_nhist; i++) { wchar_t t[300]; histLabel(g_hist[i], t, 300); SendMessageW(hHist, CB_ADDSTRING, 0, (LPARAM)t); }
    SendMessage(hHist, CB_SETCURSEL, 0, 0);
    SendMessage(hHist, CB_SETDROPPEDWIDTH, S(780), 0);
}
static void addHistory(int secs, long hits)
{
    HistEntry h; ZeroMemory(&h, sizeof h);
    SYSTEMTIME st; GetLocalTime(&st);
    _snwprintf(h.stamp, 15, L"%02d/%02d %02d:%02d", st.wMonth, st.wDay, st.wHour, st.wMinute);
    wcsncpy(h.seed, g_seedStr, 39); for (wchar_t* q = h.seed; *q; q++) if (*q == L'|') *q = L'/';
    h.ver = g_searchVer; h.mode = P.dense ? 4 : P.K - 1;
    if (P.dense) { h.K = P.K; h.M = P.thr; } else { h.K = (int)getNum(hDK, 5); h.M = (int)getNum(hDM, 12); }
    h.iso = P.onlyIso; h.X = P.X; h.Z = P.Z; h.R = P.R; h.hits = hits; h.secs = secs;
    bool same = g_nhist > 0 && !wcscmp(g_hist[0].seed, h.seed) && g_hist[0].ver == h.ver && g_hist[0].mode == h.mode &&
        g_hist[0].K == h.K && g_hist[0].M == h.M && g_hist[0].iso == h.iso && g_hist[0].X == h.X && g_hist[0].Z == h.Z && g_hist[0].R == h.R;
    if (same) g_hist[0] = h;
    else {
        if (g_nhist < HIST_MAX) g_nhist++;
        memmove(&g_hist[1], &g_hist[0], sizeof(HistEntry) * (g_nhist - 1));
        g_hist[0] = h;
    }
    saveHistory();
    refillHistCombo();
}
static void restoreHistory()
{
    if (g_running) return;
    int sel = (int)SendMessage(hHist, CB_GETCURSEL, 0, 0);
    if (sel <= 0 || sel > g_nhist) return;
    const HistEntry h = g_hist[sel - 1];
    wchar_t b[40];
    SetWindowTextW(hSeed, h.seed);
    SendMessage(hVer, CB_SETCURSEL, h.ver, 0);
    _snwprintf(b, 40, L"%lld", h.X); SetWindowTextW(hX, b);
    _snwprintf(b, 40, L"%lld", h.Z); SetWindowTextW(hZ, b);
    _snwprintf(b, 40, L"%lld", h.R); SetWindowTextW(hR, b);
    _snwprintf(b, 40, L"%d", h.K); SetWindowTextW(hDK, b);
    _snwprintf(b, 40, L"%d", h.M); SetWindowTextW(hDM, b);
    SendMessage(hIso, BM_SETCHECK, h.iso ? BST_CHECKED : BST_UNCHECKED, 0);
    setMode(h.mode);
    updateChunks();
    SendMessage(hHist, CB_SETCURSEL, 0, 0);
    setStatus(L"履歴から条件を呼び出しました。「探す」で同じ検索をやり直せます。");
}
static void clearHistory()
{
    if (g_running) return;
    if (g_nhist == 0) { MessageBoxW(hMain, T(L"検索履歴はまだありません。", L"There is no search history yet."), T(L"検索履歴の初期化", L"Clear search history"), MB_OK | MB_ICONINFORMATION); return; }
    wchar_t m[200]; _snwprintf(m, 200, L"検索履歴（%d件）をすべて消します。よろしいですか？\r\n（いま表示している結果は消えません）", g_nhist); m[199] = 0;
    if (MessageBoxW(hMain, m, L"検索履歴の初期化", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
    g_nhist = 0; saveHistory(); refillHistCombo();
    setStatus(L"検索履歴を初期化しました。");
}
static void clearResults()
{
    if (g_running) return;
    ListView_DeleteAllItems(hList); g_nres = 0; g_shown = 0;
    SendMessage(hProg, PBM_SETPOS, 0, 0);
    setStatus(L"表示中の結果をクリアしました。");
}
static void resetInputs()
{
    if (g_running) return;
    SetWindowTextW(hX, L"0"); SetWindowTextW(hZ, L"0"); SetWindowTextW(hR, L"100000");
    SetWindowTextW(hDK, L"5"); SetWindowTextW(hDM, L"12");
    SendMessage(hIso, BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessage(hVer, CB_SETCURSEL, 0, 0);
    setMode(2); updateChunks();
    setStatus(L"入力を初期値に戻しました（シード値はそのままです）。");
}

// ---------------------------------------------------------------------
//  お気に入り (%APPDATA%\SlimeChunkFinder\settings.ini の [favorites])
// ---------------------------------------------------------------------
struct FavEntry { wchar_t stamp[16]; wchar_t seed[40]; int ver; wchar_t pat[40]; long long X, Z; };
static const int FAV_MAX = 300;
static FavEntry g_fav[FAV_MAX];
static int g_nfav = 0;
static int g_favSel = -1;   // 選んでいるお気に入りの座標 (プレビュー用、検索結果でなくお気に入り自身)

static void favToString(const FavEntry& f, wchar_t* out, size_t n)
{
    wchar_t pat[80]; wcscpy(pat, f.pat); for (wchar_t* q = pat; *q; q++) if (*q == L'|') *q = L'/';
    _snwprintf(out, n, L"%ls|%ls|%d|%ls|%lld|%lld", f.stamp, f.seed, f.ver, pat, f.X, f.Z);
    out[n - 1] = 0;
}
static bool favFromString(const wchar_t* s, FavEntry& f)
{
    wchar_t buf[400]; wcsncpy(buf, s, 399); buf[399] = 0;
    wchar_t* fl[6]; int n = 0; wchar_t* p = buf; fl[n++] = p;
    while (*p && n < 6) { if (*p == L'|') { *p = 0; fl[n++] = p + 1; } p++; }
    if (n < 6) return false;
    ZeroMemory(&f, sizeof f);
    wcsncpy(f.stamp, fl[0], 15); wcsncpy(f.seed, fl[1], 39); f.ver = _wtoi(fl[2]); wcsncpy(f.pat, fl[3], 39);
    f.X = _wtoi64(fl[4]); f.Z = _wtoi64(fl[5]);
    return f.ver >= 0 && f.ver < NVER;
}
static void saveFavorites()
{
    wchar_t ini[MAX_PATH * 2], b[420], k[16];
    iniPath(ini, MAX_PATH * 2);
    WritePrivateProfileStringW(L"favorites", NULL, NULL, ini);
    if (g_nfav == 0) return;
    _snwprintf(b, 16, L"%d", g_nfav); WritePrivateProfileStringW(L"favorites", L"Count", b, ini);
    for (int i = 0; i < g_nfav; i++) { _snwprintf(k, 16, L"F%d", i); favToString(g_fav[i], b, 420); WritePrivateProfileStringW(L"favorites", k, b, ini); }
}
static void loadFavorites()
{
    wchar_t ini[MAX_PATH * 2], b[420], k[16];
    iniPath(ini, MAX_PATH * 2);
    g_nfav = 0;
    int cnt = (int)GetPrivateProfileIntW(L"favorites", L"Count", 0, ini);
    if (cnt > FAV_MAX) cnt = FAV_MAX;
    for (int i = 0; i < cnt; i++) {
        _snwprintf(k, 16, L"F%d", i); GetPrivateProfileStringW(L"favorites", k, L"", b, 420, ini);
        FavEntry f; if (b[0] && favFromString(b, f)) g_fav[g_nfav++] = f;
    }
}
static void setFavButtonLabel()
{
    if (!hFavAdd) return;
    SetWindowTextW(hFavAdd, g_viewFav ? T(L"🗑 選択を削除", L"\U0001F5D1 Delete selected") : T(L"☆ お気に入りに追加", L"\u2606 Add to favorites"));
    SetWindowTextW(hFavView, g_viewFav ? T(L"← 検索結果に戻る", L"\u2190 Back to results") : T(L"★ お気に入りを見る", L"\u2605 View favorites"));
}
static void fillFavList()
{
    ListView_DeleteAllItems(hList); g_sortCol = -1; setSortArrow(-1, true);
    g_shown = g_nfav < MAX_SHOW ? g_nfav : MAX_SHOW;
    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    for (int i = 0; i < g_shown; i++) {
        wchar_t t[160];
        LVITEMW it; ZeroMemory(&it, sizeof it);
        it.mask = LVIF_TEXT; it.iItem = i;
        _snwprintf(t, 160, L"%d", i + 1); it.pszText = t;
        ListView_InsertItem(hList, &it);
        _snwprintf(t, 160, L"%lld", g_fav[i].X); ListView_SetItemText(hList, i, 1, t);
        _snwprintf(t, 160, L"%lld", g_fav[i].Z); ListView_SetItemText(hList, i, 2, t);
        ListView_SetItemText(hList, i, 3, g_fav[i].pat);
        ListView_SetItemText(hList, i, 4, g_fav[i].stamp);
        wchar_t sd[24]; if (wcslen(g_fav[i].seed) > 16) { wcsncpy(sd, g_fav[i].seed, 14); sd[14] = L'…'; sd[15] = 0; } else wcscpy(sd, g_fav[i].seed);
        _snwprintf(t, 160, L"%ls / %ls", sd, verNames[g_fav[i].ver]); ListView_SetItemText(hList, i, 5, t);
    }
    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    if (g_shown > 0) ListView_SetItemState(hList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    wchar_t b[80]; _snwprintf(b, 80, g_nfav ? L"お気に入り：%d件" : L"お気に入りはまだありません。", g_nfav); b[79] = 0; setStatus(b);
}
static void setListColumnsForView(bool fav)
{
    LVCOLUMNW c; ZeroMemory(&c, sizeof c); c.mask = LVCF_TEXT;
    wchar_t t1[32], t2[32], t3[32], t4[32], t5[32];
    wcscpy(t1, T(L"中心 X", L"Center X"));
    wcscpy(t2, T(L"中心 Z", L"Center Z"));
    wcscpy(t3, fav ? T(L"パターン", L"Pattern") : T(L"内容", L"Result"));
    wcscpy(t4, fav ? T(L"追加日時", L"Added") : T(L"中心からの距離", L"Distance"));
    wcscpy(t5, fav ? T(L"シード / バージョン", L"Seed / Version") : T(L"チャンク範囲", L"Chunk range"));
    c.pszText = t1; ListView_SetColumn(hList, 1, &c);
    c.pszText = t2; ListView_SetColumn(hList, 2, &c);
    c.pszText = t3; ListView_SetColumn(hList, 3, &c);
    c.pszText = t4; ListView_SetColumn(hList, 4, &c);
    c.pszText = t5; ListView_SetColumn(hList, 5, &c);
}
static void applyLanguage()
{
    SetWindowTextW(hMain, T(L"スライムチャンク ファインダー", L"Slime Chunk Finder"));
    SetWindowTextW(hLbl[0], T(L"シード値", L"Seed"));
    SetWindowTextW(hLbl[1], T(L"Javaバージョン", L"Java Version"));
    SetWindowTextW(hLbl[2], T(L"中心 X", L"Center X"));
    SetWindowTextW(hLbl[3], T(L"中心 Z", L"Center Z"));
    SetWindowTextW(hLbl[4], T(L"半径（ブロック）", L"Radius (blocks)"));
    SetWindowTextW(hLbl[5], T(L"範囲の一辺", L"Box size"));
    SetWindowTextW(hLbl[6], T(L"最小個数", L"Min count"));
    SetWindowTextW(hLbl[7], T(L"検索履歴", L"Search history"));
    SetWindowTextW(hIso, T(L"外周1チャンクにスライムチャンクがないものだけ表示", L"Show only spots with a clean 1-chunk border"));
    SetWindowTextW(hNearest, T(L"最も近い1件だけを一瞬で探す", L"Instantly find only the single nearest spot"));
    SetWindowTextW(hLblSeg, T(L"サイズ", L"Size"));
    SetWindowTextW(hLblRad, T(L"半径", L"Radius"));
    SetWindowTextW(hSeg[0], L"1\u00d71"); SetWindowTextW(hSeg[1], L"2\u00d72"); SetWindowTextW(hSeg[2], L"3\u00d73"); SetWindowTextW(hSeg[3], L"4\u00d74");
    SetWindowTextW(hSeg[4], T(L"密集", L"Dense"));
    SetWindowTextW(hRp[0], T(L"1万", L"10k")); SetWindowTextW(hRp[1], T(L"5万", L"50k"));
    SetWindowTextW(hRp[2], T(L"10万", L"100k")); SetWindowTextW(hRp[3], T(L"50万", L"500k"));
    SendMessageW(hSeed, EM_SETCUEBANNER, TRUE, (LPARAM)T(L"例：70392165441118850 や好きな文字列", L"e.g. 70392165441118850 or any text"));
    SetWindowTextW(hGo, T(L"探す", L"Search"));
    SetWindowTextW(hStop, T(L"止める", L"Stop"));
    SetWindowTextW(hHistClear, T(L"履歴を初期化", L"Clear history"));
    SetWindowTextW(hCopyXZ, T(L"座標をコピー", L"Copy coordinates"));
    SetWindowTextW(hCopyTP, T(L"/tp コマンドをコピー", L"Copy /tp command"));
    SetWindowTextW(hOpen, T(L"Chunkbaseで開く", L"Open in Chunkbase"));
    SetWindowTextW(hCenter, T(L"ここを中心にする", L"Set as center"));
    SetWindowTextW(hCopyAll, T(L"一覧をコピー", L"Copy list"));
    SetWindowTextW(hNote, NOTE_TEXT);
    setFavButtonLabel();
    setMode(g_mode);
    updateChunks();
    updateSavePathLabel();
    setListColumnsForView(g_viewFav);
    refillHistCombo();
    {   // バージョン選択欄: 1.7の表示だけ言語によって文言が変わるので、その項目だけ更新する
        int sel = (int)SendMessage(hVer, CB_GETCURSEL, 0, 0);
        SendMessage(hVer, CB_DELETESTRING, NVER - 1, 0);
        SendMessageW(hVer, CB_INSERTSTRING, NVER - 1, (LPARAM)verDisplayName(NVER - 1));
        if (sel >= 0) SendMessage(hVer, CB_SETCURSEL, sel, 0);
    }
    if (!g_running) setStatus(T(L"シード値を入れて「探す」を押してください。", L"Enter a seed and press Search."));
    if (hMenu) {
        HMENU old = hMenu;
        hMenu = buildMenu();
        SetMenu(hMain, hMenu);
        DestroyMenu(old);
        CheckMenuItem(hMenu, IDM_FILE_AUTOSAVE, MF_BYCOMMAND | (g_autoSave ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, IDM_SET_DARK, MF_BYCOMMAND | (g_dark ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuRadioItem(hMenu, IDM_SIZE_1, IDM_SIZE_DENSE, IDM_SIZE_1 + g_mode, MF_BYCOMMAND);
        DrawMenuBar(hMain);
    }
    InvalidateRect(hMain, NULL, TRUE);
    if (hPreview) InvalidateRect(hPreview, NULL, TRUE);
    applyTitleBar();
}
static void toggleFavView()
{
    if (g_running) return;
    g_viewFav = !g_viewFav;
    setListColumnsForView(g_viewFav);
    setFavButtonLabel();
    if (g_viewFav) fillFavList(); else fillList();
    InvalidateRect(hPreview, NULL, TRUE);
}
static void addFavoriteFromSelection()
{
    int i = selIndex(); if (i < 0) return;
    if (g_nfav >= FAV_MAX) { MessageBoxW(hMain, L"お気に入りは300件までです。いくつか削除してから追加してください。", L"お気に入り", MB_OK | MB_ICONWARNING); return; }
    FavEntry f; ZeroMemory(&f, sizeof f);
    SYSTEMTIME st; GetLocalTime(&st); _snwprintf(f.stamp, 15, L"%02d/%02d %02d:%02d", st.wMonth, st.wDay, st.wHour, st.wMinute);
    wcsncpy(f.seed, g_seedStr, 39); f.ver = g_searchVer;
    long long bx, bz; centerOf(g_res[i], &bx, &bz); f.X = bx; f.Z = bz;
    patternText(f.pat, 40, P.dense ? 4 : P.K - 1, P.K, P.thr);
    for (int k = 0; k < g_nfav; k++) if (!wcscmp(g_fav[k].seed, f.seed) && g_fav[k].ver == f.ver && g_fav[k].X == f.X && g_fav[k].Z == f.Z) {
        MessageBoxW(hMain, L"この場所はすでにお気に入りに入っています。", L"お気に入り", MB_OK | MB_ICONINFORMATION); return;
    }
    memmove(&g_fav[1], &g_fav[0], sizeof(FavEntry) * (g_nfav < FAV_MAX - 1 ? g_nfav : FAV_MAX - 1));
    if (g_nfav < FAV_MAX) g_nfav++;
    g_fav[0] = f;
    saveFavorites();
    wchar_t m[120]; _snwprintf(m, 120, L"お気に入りに追加しました（X %lld, Z %lld）。", bx, bz); m[119] = 0; setStatus(m);
}
static void removeFavoriteSelected()
{
    int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (i < 0 || i >= g_shown) { MessageBoxW(hMain, L"一覧から場所を選んでください。", L"選択してください", MB_OK | MB_ICONINFORMATION); return; }
    if (MessageBoxW(hMain, L"このお気に入りを削除しますか？", L"お気に入りの削除", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
    memmove(&g_fav[i], &g_fav[i + 1], sizeof(FavEntry) * (g_nfav - i - 1));
    g_nfav--; saveFavorites(); fillFavList();
}

static void layout()
{
    RECT rc; GetClientRect(hMain, &rc);
    int W = rc.right, H = rc.bottom, M = S(16), pad = S(18);
    int lh = S(22), eh = S(28);
    int left = M + pad, right = W - M - pad;
    int y = S(76) + S(16), x;
    g_card1.left = M; g_card1.right = W - M; g_card1.top = y;
    y += S(16);
    MoveWindow(hLbl[0], left, y + S(4), S(72), lh, TRUE);
    MoveWindow(hSeed, left + S(76), y, S(300), eh, TRUE);
    MoveWindow(hLbl[1], left + S(76 + 300 + 20), y + S(4), S(104), lh, TRUE);
    MoveWindow(hVer, left + S(76 + 300 + 20 + 108), y, S(190), S(320), TRUE);
    y += S(44); x = left;
    MoveWindow(hLbl[2], x, y + S(4), S(50), lh, TRUE); x += S(54);
    MoveWindow(hX, x, y, S(110), eh, TRUE); x += S(110 + 16);
    MoveWindow(hLbl[3], x, y + S(4), S(50), lh, TRUE); x += S(54);
    MoveWindow(hZ, x, y, S(110), eh, TRUE); x += S(110 + 16);
    MoveWindow(hLbl[4], x, y + S(4), S(124), lh, TRUE); x += S(128);
    MoveWindow(hR, x, y, S(110), eh, TRUE);
    y += S(34);
    MoveWindow(hChunks, left, y, right - left, lh, TRUE);
    y += S(34);
    // サイズ選択ボタン + 半径プリセット
    MoveWindow(hLblSeg, left, y + S(5), S(76), lh, TRUE);
    { int sx = left + S(80);
      for (int i = 0; i < 5; i++) { int w = (i == 4) ? 84 : 66; MoveWindow(hSeg[i], sx, y, S(w), S(34), TRUE); sx += S(w + 6); }
      sx += S(18);
      MoveWindow(hLblRad, sx, y + S(5), S(64), lh, TRUE); sx += S(68);
      for (int i = 0; i < 4; i++) { MoveWindow(hRp[i], sx, y + S(3), S(58), S(28), TRUE); sx += S(64); } }
    y += S(46);
    MoveWindow(hPattern, left, y + S(4), S(440), lh, TRUE);
    x = left + S(450);
    MoveWindow(hLbl[5], x, y + S(4), S(90), lh, TRUE); x += S(94);
    MoveWindow(hDK, x, y, S(50), eh, TRUE); x += S(50 + 12);
    MoveWindow(hLbl[6], x, y + S(4), S(70), lh, TRUE); x += S(74);
    MoveWindow(hDM, x, y, S(50), eh, TRUE);
    y += S(44);
    MoveWindow(hIso, left, y + S(7), S(440), lh, TRUE);
    MoveWindow(hGo, left + S(450), y, S(110), S(36), TRUE);
    MoveWindow(hStop, left + S(570), y, S(100), S(36), TRUE);
    y += S(30);
    MoveWindow(hNearest, left, y + S(7), S(440), lh, TRUE);
    y += S(38);
    MoveWindow(hLbl[7], left, y + S(5), S(130), lh, TRUE);
    MoveWindow(hHist, left + S(136), y, S(410), S(320), TRUE);
    MoveWindow(hHistClear, left + S(136 + 410 + 12), y - S(1), S(140), S(32), TRUE);
    y += S(44);
    MoveWindow(hSavePath, left, y, right - left, lh, TRUE);
    y += S(22) + S(14);
    g_card1.bottom = y;
    y += S(14);
    MoveWindow(hProg, M, y, W - 2 * M, S(12), TRUE); y += S(18);
    MoveWindow(hStatus, M, y, W - 2 * M, lh, TRUE); y += S(26);
    MoveWindow(hNote, M, y, W - 2 * M, S(40), TRUE); y += S(44);
    int by2 = H - M - S(36);          // お気に入りボタンの行
    int by  = by2 - S(42);            // 座標コピー等ボタンの行
    g_card2.left = M; g_card2.right = W - M; g_card2.top = y; g_card2.bottom = by - S(12);
    int prevW = S(190);
    int listR = W - M - S(8) - prevW - S(10);
    MoveWindow(hList, M + S(8), y + S(8), listR - (M + S(8)), g_card2.bottom - y - S(16), TRUE);
    MoveWindow(hPreview, listR + S(10), y + S(8), prevW - S(10), g_card2.bottom - y - S(16), TRUE);
    int bx = M;
    MoveWindow(hCopyXZ, bx, by, S(140), S(36), TRUE); bx += S(150);
    MoveWindow(hCopyTP, bx, by, S(180), S(36), TRUE); bx += S(190);
    MoveWindow(hOpen, bx, by, S(160), S(36), TRUE); bx += S(170);
    MoveWindow(hCenter, bx, by, S(160), S(36), TRUE); bx += S(170);
    MoveWindow(hCopyAll, bx, by, S(120), S(36), TRUE);
    int bx2 = M;
    MoveWindow(hFavAdd, bx2, by2, S(190), S(36), TRUE); bx2 += S(200);
    MoveWindow(hFavView, bx2, by2, S(190), S(36), TRUE);
    InvalidateRect(hMain, NULL, TRUE);
}

// ---------------------------------------------------------------------
//  見た目 (背景・カード・ボタン)
// ---------------------------------------------------------------------
static void drawCard(HDC dc, const RECT& r)
{
    int rad = S(18);
    HPEN pn = CreatePen(PS_SOLID, 1, C_SHADOW); HBRUSH bs = CreateSolidBrush(C_SHADOW);
    HGDIOBJ op = SelectObject(dc, pn), ob = SelectObject(dc, bs);
    RoundRect(dc, r.left, r.top + S(3), r.right, r.bottom + S(3), rad, rad);
    HPEN pn2 = CreatePen(PS_SOLID, 1, C_LINE); HBRUSH bw = CreateSolidBrush(C_WHITE);
    SelectObject(dc, pn2); SelectObject(dc, bw);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, rad, rad);
    SelectObject(dc, op); SelectObject(dc, ob);
    DeleteObject(pn); DeleteObject(bs); DeleteObject(pn2); DeleteObject(bw);
}

static void drawBackdrop(HDC dc)
{
    RECT rc; GetClientRect(hMain, &rc);
    HBRUSH bg = CreateSolidBrush(C_MINT); FillRect(dc, &rc, bg); DeleteObject(bg);

    // 上のバナー (グラデーション)
    int bh = S(76);
    TRIVERTEX v[2];
    v[0].x = 0; v[0].y = 0; v[0].Red = 0x7C00; v[0].Green = 0xD800; v[0].Blue = 0x5000; v[0].Alpha = 0;
    v[1].x = rc.right; v[1].y = bh; v[1].Red = 0x36 << 8; v[1].Green = 0xB0 << 8; v[1].Blue = 0x94 << 8; v[1].Alpha = 0;
    GRADIENT_RECT gr = { 0, 1 };
    GradientFill(dc, v, 2, &gr, 1, GRADIENT_FILL_RECT_H);

    // 3×3 のロゴ
    int cs = S(15), gap = S(3), lx = S(24), ly = (bh - (3 * cs + 2 * gap)) / 2;
    HPEN np = (HPEN)GetStockObject(NULL_PEN);
    HGDIOBJ op = SelectObject(dc, np);
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) {
        HBRUSH cb = CreateSolidBrush(((i + j) % 2 == 0) ? RGB(255, 255, 255) : RGB(214, 247, 190));
        HGDIOBJ ob = SelectObject(dc, cb);
        int x0 = lx + i * (cs + gap), y0 = ly + j * (cs + gap);
        RoundRect(dc, x0, y0, x0 + cs + 1, y0 + cs + 1, S(7), S(7));
        SelectObject(dc, ob); DeleteObject(cb);
    }
    SelectObject(dc, op);

    int tx = lx + 3 * cs + 2 * gap + S(18);
    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, hFontTitle); SetTextColor(dc, RGB(255, 255, 255));
    const wchar_t* ttl = T(L"スライムチャンク ファインダー", L"Slime Chunk Finder");
    TextOutW(dc, tx, S(11), ttl, (int)wcslen(ttl));
    SelectObject(dc, hFontSub); SetTextColor(dc, RGB(233, 251, 224));
    const wchar_t* sub = T(L"シード値から、スライムチャンクがそろった場所を探します（Java版）", L"Find slime chunks from your world seed (Java Edition)");
    TextOutW(dc, tx, S(46), sub, (int)wcslen(sub));

    drawCard(dc, g_card1);
    drawCard(dc, g_card2);
}

static void drawButton(const DRAWITEMSTRUCT* d)
{
    HDC dc = d->hDC; RECT r = d->rcItem; int id = (int)d->CtlID;
    bool dis = (d->itemState & ODS_DISABLED) != 0, down = (d->itemState & ODS_SELECTED) != 0;
    bool hot = (d->hwndItem == g_hot) && !dis;
    bool seg = (id >= ID_SEG0 && id < ID_SEG0 + 5), chip = (id >= ID_RP0 && id < ID_RP0 + 4);
    bool on = false;
    if (seg) on = (id - ID_SEG0 == g_mode);
    if (chip) on = (getNum(hR, -1) == RP_VAL[id - ID_RP0]);
    bool inCard = (id == ID_GO || id == ID_STOP || id == ID_HISTCLEAR || seg || chip);
    bool danger = (id == ID_STOP || id == ID_HISTCLEAR || (id == ID_FAVADD && g_viewFav));
    bool favBtn = (id == ID_FAVADD && !g_viewFav);
    HBRUSH bo = CreateSolidBrush(inCard ? C_WHITE : C_MINT); FillRect(dc, &r, bo); DeleteObject(bo);
    COLORREF fill, border, text;
    if (dis && (seg || chip) && on) { fill = RGB(190, 220, 170); border = RGB(170, 205, 150); text = RGB(90, 120, 80); }
    else if (dis) { fill = RGB(240, 244, 236); border = RGB(218, 226, 210); text = RGB(160, 172, 152); }
    else if (seg || chip) {
        if (on) { fill = down ? RGB(84, 178, 54) : (hot ? RGB(132, 216, 98) : C_SLIME); border = C_DEEP; text = RGB(16, 44, 12); }
        else { fill = down ? RGB(214, 236, 196) : (hot ? C_SEL : C_ROW); border = hot ? C_DEEP : RGB(150, 200, 120); text = C_DEEP; }
    }
    else if (id == ID_GO) { fill = down ? RGB(84, 178, 54) : (hot ? RGB(132, 216, 98) : C_SLIME); border = fill; text = RGB(16, 44, 12); }
    else if (danger) { fill = down ? RGB(255, 214, 204) : (hot ? RGB(255, 226, 216) : RGB(255, 238, 232)); border = RGB(240, 150, 130); text = RGB(168, 58, 38); }
    else if (favBtn) { fill = down ? RGB(255, 232, 178) : (hot ? RGB(255, 238, 196) : RGB(255, 244, 214)); border = RGB(224, 176, 80); text = RGB(140, 96, 12); }
    else { fill = down ? RGB(214, 236, 196) : (hot ? C_SEL : C_WHITE); border = hot ? C_DEEP : RGB(150, 200, 120); text = C_DEEP; }
    int rad = chip ? S(16) : S(22);
    HPEN pn = CreatePen(PS_SOLID, 1, border); HBRUSH br = CreateSolidBrush(fill);
    HGDIOBJ op = SelectObject(dc, pn), ob = SelectObject(dc, br);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, rad, rad);
    SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(pn); DeleteObject(br);
    wchar_t t[80]; GetWindowTextW(d->hwndItem, t, 80);
    SelectObject(dc, (id == ID_GO || id == ID_STOP || on) ? hFontBold : hFont);
    SetBkMode(dc, TRANSPARENT); SetTextColor(dc, text);
    RECT tr = r; if (down) OffsetRect(&tr, 0, 1);
    DrawTextW(dc, t, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if ((d->itemState & ODS_FOCUS) && !dis) { RECT f = r; InflateRect(&f, -S(5), -S(5)); DrawFocusRect(dc, &f); }
}

// ---- マウスが乗ったボタンを光らせる / 手のカーソルにする ----
static LRESULT CALLBACK HoverProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR)
{
    if (m == WM_MOUSEMOVE && g_hot != h) {
        HWND old = g_hot; g_hot = h;
        TRACKMOUSEEVENT t = { sizeof t, TME_LEAVE, h, 0 }; TrackMouseEvent(&t);
        InvalidateRect(h, NULL, FALSE); if (old) InvalidateRect(old, NULL, FALSE);
    } else if (m == WM_MOUSELEAVE) {
        if (g_hot == h) g_hot = NULL;
        InvalidateRect(h, NULL, FALSE);
    } else if (m == WM_SETCURSOR && IsWindowEnabled(h)) {
        SetCursor(LoadCursor(NULL, IDC_HAND)); return TRUE;
    } else if (m == WM_NCDESTROY) RemoveWindowSubclass(h, HoverProc, 1);
    return DefSubclassProc(h, m, w, l);
}

// ---- 座標の貼り付け補助: 「123 -456」「X: 123, Z: -456」「/tp @s 123 64 -456」を X・Z 欄にまとめて入れる ----
static LRESULT CALLBACK PasteProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR)
{
    if (m == WM_PASTE && OpenClipboard(h)) {
        long long v[4]; int n = 0;
        HANDLE hd = GetClipboardData(CF_UNICODETEXT);
        if (hd) {
            const wchar_t* s = (const wchar_t*)GlobalLock(hd);
            if (s) {
                size_t len = wcsnlen(s, 200);
                for (size_t i = 0; i < len && n < 4; ) {
                    if (iswdigit(s[i]) || (s[i] == L'-' && i + 1 < len && iswdigit(s[i + 1]))) {
                        bool neg = (s[i] == L'-'); if (neg) i++;
                        long long x = 0; int dg = 0;
                        while (i < len && iswdigit(s[i]) && dg < 12) { x = x * 10 + (s[i] - L'0'); i++; dg++; }
                        v[n++] = neg ? -x : x;
                    } else i++;
                }
                GlobalUnlock(hd);
            }
        }
        CloseClipboard();
        if (n == 2 || n == 3) {
            wchar_t b[40], msg[120];
            _snwprintf(b, 40, L"%lld", v[0]); SetWindowTextW(hX, b);
            _snwprintf(b, 40, L"%lld", v[n - 1]); SetWindowTextW(hZ, b);
            _snwprintf(msg, 120, T(L"貼り付けた座標を中心 X=%lld, Z=%lld に入れました。", L"Pasted coordinates into Center X=%lld, Z=%lld."), v[0], v[n - 1]);
            msg[119] = 0; SetWindowTextW(hStatus, msg);
            return 0;
        }
    } else if (m == WM_NCDESTROY) RemoveWindowSubclass(h, PasteProc, 2);
    return DefSubclassProc(h, m, w, l);
}

// ---- ツールチップ (ボタンや入力欄にマウスを置くと説明が出る) ----
static const wchar_t* tipFor(int id)
{
    static wchar_t buf[200];
    if (id >= ID_SEG0 && id < ID_SEG0 + 4) {
        int k = id - ID_SEG0 + 1;
        _snwprintf(buf, 200, T(L"%d×%d のスライムチャンクがぴったりそろった場所を探します", L"Find spots where a full %d\u00d7%d square of slime chunks lines up"), k, k); buf[199] = 0; return buf;
    }
    if (id == ID_SEG0 + 4) return T(L"狭い範囲にスライムチャンクが多く集まっている場所を探します（スライムファーム向け）", L"Find areas packed with slime chunks (good for slime farms)");
    if (id >= ID_RP0 && id < ID_RP0 + 4) {
        _snwprintf(buf, 200, T(L"探す半径を %lld ブロックにします", L"Set the search radius to %lld blocks"), RP_VAL[id - ID_RP0]); buf[199] = 0; return buf;
    }
    switch (id) {
    case ID_SEED: return T(L"ワールドのシード値。数字でも文字列でもOKです（F3画面や /seed で確認できます）", L"Your world seed. A number or any text works (check with /seed in game)");
    case ID_VER: return T(L"Java版ではスライムチャンクの位置はバージョンによらず同じです。Chunkbaseで開くときの設定に使われます", L"On Java Edition slime chunks are the same in every version. Used when opening Chunkbase");
    case ID_X: case ID_Z: return T(L"探す中心の座標（ブロック）。「123 -456」のような座標をそのまま貼り付けると、X と Z に自動で入ります", L"Center of the search (blocks). Paste text like \"123 -456\" to fill both X and Z");
    case ID_R: return T(L"中心からどこまで探すか（ブロック）。大きいほど時間がかかります", L"How far to search from the center (blocks). Larger takes longer");
    case ID_ISO: return T(L"見つけた場所のまわり1チャンクがスライムチャンクでないものだけに絞ります", L"Keep only spots whose 1-chunk border has no slime chunks");
    case ID_NEAREST: return T(L"中心から外側へ広げて、最初に見つかった一番近い1件で止めます", L"Expand outward from the center and stop at the first (nearest) match");
    case ID_GO: return T(L"検索を始めます（F5）", L"Start searching (F5)");
    case ID_STOP: return T(L"検索を止めます（Esc）", L"Stop searching (Esc)");
    case ID_HISTCLEAR: return T(L"検索履歴をすべて消します", L"Delete the whole search history");
    case ID_COPYXZ: return T(L"選んだ場所の座標をコピーします（一覧をダブルクリックでも可）", L"Copy the selected spot's coordinates (or double-click the row)");
    case ID_COPYTP: return T(L"ゲーム内で使える /tp コマンドをコピーします", L"Copy a /tp command you can paste in game");
    case ID_OPEN: return T(L"Chunkbaseで、この場所を地図で開きます", L"Open this spot on the Chunkbase map");
    case ID_CENTER: return T(L"選んだ場所を次の検索の中心にします", L"Use the selected spot as the next search center");
    case ID_COPYALL: return T(L"表示中の一覧をまとめてコピーします", L"Copy the whole list");
    case ID_FAVADD: return g_viewFav ? T(L"選んだお気に入りを削除します", L"Delete the selected favorite") : T(L"選んだ場所をお気に入りに保存します", L"Save the selected spot as a favorite");
    case ID_FAVVIEW: return T(L"保存したお気に入りと検索結果を切り替えます", L"Switch between favorites and search results");
    }
    return NULL;
}
static void addTip(HWND h)
{
    if (!hTip || !h) return;
    TOOLINFOW ti; ZeroMemory(&ti, sizeof ti);
    ti.cbSize = sizeof ti; ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS; ti.hwnd = hMain; ti.uId = (UINT_PTR)h; ti.lpszText = LPSTR_TEXTCALLBACKW;
    SendMessageW(hTip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
}

// ---- 結果一覧の右クリックメニュー ----
static void showResultMenu(POINT pt)
{
    POINT cp = pt; ScreenToClient(hList, &cp);
    LVHITTESTINFO hi; ZeroMemory(&hi, sizeof hi); hi.pt = cp;
    int it = ListView_HitTest(hList, &hi);
    if (it < 0 || it >= g_shown) return;
    ListView_SetItemState(hList, it, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    HMENU m = CreatePopupMenu();
    wchar_t fav[80]; GetWindowTextW(hFavAdd, fav, 80);
    AppendMenuW(m, MF_STRING, ID_COPYXZ, T(L"座標をコピー", L"Copy coordinates"));
    AppendMenuW(m, MF_STRING, ID_COPYTP, T(L"/tp コマンドをコピー", L"Copy /tp command"));
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, ID_OPEN, T(L"Chunkbaseで開く", L"Open in Chunkbase"));
    AppendMenuW(m, MF_STRING, ID_CENTER, T(L"ここを中心にする", L"Set as center"));
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, ID_FAVADD, fav);
    SetForegroundWindow(hMain);
    TrackPopupMenu(m, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hMain, NULL);
    DestroyMenu(m);
}

static HWND mk(const wchar_t* cls, const wchar_t* txt, DWORD style, DWORD ex, int id)
{
    return CreateWindowExW(ex, cls, txt, WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10, hMain, (HMENU)(INT_PTR)id, hInst, NULL);
}
static void addCol(int i, const wchar_t* t, int w)
{
    LVCOLUMNW c; ZeroMemory(&c, sizeof c);
    c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; c.pszText = (LPWSTR)t; c.cx = S(w); c.iSubItem = i;
    ListView_InsertColumn(hList, i, &c);
}
static BOOL CALLBACK setFontProc(HWND h, LPARAM f) { SendMessage(h, WM_SETFONT, (WPARAM)f, TRUE); return TRUE; }

typedef HRESULT (WINAPI* SetWindowThemeFn)(HWND, LPCWSTR, LPCWSTR);

static void onCreate()
{
    applyTheme(false);
    HDC dc = GetDC(NULL); dpi = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(NULL, dc);
    NONCLIENTMETRICSW ncm; ZeroMemory(&ncm, sizeof ncm); ncm.cbSize = sizeof ncm;
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof ncm, &ncm, 0);
    hFont = CreateFontIndirectW(&ncm.lfMessageFont);
    LOGFONTW lb = ncm.lfMessageFont; lb.lfWeight = FW_BOLD; hFontBold = CreateFontIndirectW(&lb);
    LOGFONTW lt = ncm.lfMessageFont; lt.lfWeight = FW_BOLD; lt.lfHeight = lt.lfHeight * 17 / 10; hFontTitle = CreateFontIndirectW(&lt);
    LOGFONTW ls = ncm.lfMessageFont; ls.lfHeight = ls.lfHeight * 9 / 10; hFontSub = CreateFontIndirectW(&ls);
    g_brMint = CreateSolidBrush(C_MINT); g_brWhite = CreateSolidBrush(C_WHITE); g_brOff = CreateSolidBrush(C_OFF);

    hLbl[0] = mk(L"STATIC", T(L"シード値", L"Seed"), 0, 0, ID_LBL0);
    hSeed = mk(L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_SEED);
    hLbl[1] = mk(L"STATIC", T(L"Javaバージョン", L"Java Version"), 0, 0, ID_LBL0 + 1);
    hVer = mk(L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 0, ID_VER);
    for (int i = 0; i < NVER; i++) SendMessageW(hVer, CB_ADDSTRING, 0, (LPARAM)verDisplayName(i));
    SendMessage(hVer, CB_SETCURSEL, 0, 0);
    hLbl[2] = mk(L"STATIC", T(L"中心 X", L"Center X"), 0, 0, ID_LBL0 + 2);
    hX = mk(L"EDIT", L"0", WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_X);
    hLbl[3] = mk(L"STATIC", T(L"中心 Z", L"Center Z"), 0, 0, ID_LBL0 + 3);
    hZ = mk(L"EDIT", L"0", WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_Z);
    hLbl[4] = mk(L"STATIC", T(L"半径（ブロック）", L"Radius (blocks)"), 0, 0, ID_LBL0 + 4);
    hR = mk(L"EDIT", L"100000", WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_R);
    hChunks = mk(L"STATIC", L"", 0, 0, ID_CHUNKS);
    hPattern = mk(L"STATIC", L"", 0, 0, ID_PATTERN);
    hLbl[5] = mk(L"STATIC", T(L"範囲の一辺", L"Box size"), 0, 0, ID_LBL0 + 5);
    hDK = mk(L"EDIT", L"5", WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE, ID_DK);
    hLbl[6] = mk(L"STATIC", T(L"最小個数", L"Min count"), 0, 0, ID_LBL0 + 6);
    hDM = mk(L"EDIT", L"12", WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE, ID_DM);
    hIso = mk(L"BUTTON", T(L"外周1チャンクにスライムチャンクがないものだけ表示", L"Show only spots with a clean 1-chunk border"), WS_TABSTOP | BS_AUTOCHECKBOX, 0, ID_ISO);
    hNearest = mk(L"BUTTON", T(L"最も近い1件だけを一瞬で探す", L"Instantly find only the single nearest spot"), WS_TABSTOP | BS_AUTOCHECKBOX, 0, ID_NEAREST);
    hGo = mk(L"BUTTON", T(L"探す", L"Search"), WS_TABSTOP | BS_OWNERDRAW, 0, ID_GO);
    hStop = mk(L"BUTTON", T(L"止める", L"Stop"), WS_TABSTOP | BS_OWNERDRAW, 0, ID_STOP);
    EnableWindow(hStop, FALSE);
    hLbl[7] = mk(L"STATIC", T(L"検索履歴", L"Search history"), 0, 0, ID_LBL0 + 7);
    hHist = mk(L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 0, ID_HIST);
    hHistClear = mk(L"BUTTON", T(L"履歴を初期化", L"Clear history"), WS_TABSTOP | BS_OWNERDRAW, 0, ID_HISTCLEAR);
    hSavePath = mk(L"STATIC", L"", SS_PATHELLIPSIS, 0, ID_SAVEPATH);
    hProg = mk(PROGRESS_CLASSW, L"", 0, 0, ID_PROG);
    SendMessage(hProg, PBM_SETRANGE32, 0, 1000);
    { HMODULE ux = LoadLibraryW(L"uxtheme.dll");
      if (ux) { SetWindowThemeFn f = (SetWindowThemeFn)GetProcAddress(ux, "SetWindowTheme"); if (f) f(hProg, L"", L""); } }
    SendMessage(hProg, PBM_SETBARCOLOR, 0, C_SLIME);
    SendMessage(hProg, PBM_SETBKCOLOR, 0, RGB(217, 232, 203));
    hStatus = mk(L"STATIC", T(L"シード値を入れて「探す」を押してください。", L"Enter a seed and press Search."), 0, 0, ID_STATUS);
    hNote = mk(L"STATIC", NOTE_TEXT, 0, 0, ID_NOTE);
    hList = mk(WC_LISTVIEWW, L"", WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 0, ID_LIST);
    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    ListView_SetBkColor(hList, C_WHITE); ListView_SetTextBkColor(hList, C_WHITE); ListView_SetTextColor(hList, C_INK);
    addCol(0, L"#", 44); addCol(1, T(L"中心 X", L"Center X"), 90); addCol(2, T(L"中心 Z", L"Center Z"), 90); addCol(3, T(L"内容", L"Result"), 190);
    addCol(4, T(L"中心からの距離", L"Distance"), 120); addCol(5, T(L"チャンク範囲", L"Chunk range"), 240);
    hCopyXZ = mk(L"BUTTON", T(L"座標をコピー", L"Copy coordinates"), WS_TABSTOP | BS_OWNERDRAW, 0, ID_COPYXZ);
    hCopyTP = mk(L"BUTTON", T(L"/tp コマンドをコピー", L"Copy /tp command"), WS_TABSTOP | BS_OWNERDRAW, 0, ID_COPYTP);
    hOpen = mk(L"BUTTON", T(L"Chunkbaseで開く", L"Open in Chunkbase"), WS_TABSTOP | BS_OWNERDRAW, 0, ID_OPEN);
    hCenter = mk(L"BUTTON", T(L"ここを中心にする", L"Set as center"), WS_TABSTOP | BS_OWNERDRAW, 0, ID_CENTER);
    hCopyAll = mk(L"BUTTON", T(L"一覧をコピー", L"Copy list"), WS_TABSTOP | BS_OWNERDRAW, 0, ID_COPYALL);
    hFavAdd = mk(L"BUTTON", T(L"☆ お気に入りに追加", L"\u2606 Add to favorites"), WS_TABSTOP | BS_OWNERDRAW, 0, ID_FAVADD);
    hFavView = mk(L"BUTTON", T(L"★ お気に入りを見る", L"\u2605 View favorites"), WS_TABSTOP | BS_OWNERDRAW, 0, ID_FAVVIEW);
    hPreview = mk(L"SlimePreviewWnd", L"", 0, WS_EX_CLIENTEDGE, ID_PREVIEW);
    // v1.5.0: サイズ選択ボタンと半径プリセット (Tab の順番がずれないよう「取得中」欄の直後に差し込む)
    hLblSeg = mk(L"STATIC", L"", 0, 0, ID_LBL0 + 8);
    for (int i = 0; i < 5; i++) hSeg[i] = mk(L"BUTTON", L"", WS_TABSTOP | BS_OWNERDRAW, 0, ID_SEG0 + i);
    hLblRad = mk(L"STATIC", L"", 0, 0, ID_LBL0 + 9);
    for (int i = 0; i < 4; i++) hRp[i] = mk(L"BUTTON", L"", WS_TABSTOP | BS_OWNERDRAW, 0, ID_RP0 + i);
    { HWND prev = hChunks;
      HWND order[11] = { hLblSeg, hSeg[0], hSeg[1], hSeg[2], hSeg[3], hSeg[4], hLblRad, hRp[0], hRp[1], hRp[2], hRp[3] };
      for (int i = 0; i < 11; i++) { SetWindowPos(order[i], prev, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); prev = order[i]; } }
    EnumChildWindows(hMain, setFontProc, (LPARAM)hFont);
    SendMessage(hPattern, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    SendMessage(hChunks, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    { HWND hv[] = { hGo, hStop, hHistClear, hCopyXZ, hCopyTP, hOpen, hCenter, hCopyAll, hFavAdd, hFavView, hSeg[0], hSeg[1], hSeg[2], hSeg[3], hSeg[4], hRp[0], hRp[1], hRp[2], hRp[3] };
      for (HWND h : hv) SetWindowSubclass(h, HoverProc, 1, 0); }
    SetWindowSubclass(hX, PasteProc, 2, 0); SetWindowSubclass(hZ, PasteProc, 2, 0);
    hTip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hMain, NULL, hInst, NULL);
    if (hTip) {
        SendMessage(hTip, TTM_SETMAXTIPWIDTH, 0, S(340));
        SendMessage(hTip, TTM_SETDELAYTIME, TTDT_INITIAL, 450);
        HWND tv[] = { hSeed, hVer, hX, hZ, hR, hIso, hNearest, hGo, hStop, hHistClear, hCopyXZ, hCopyTP, hOpen, hCenter, hCopyAll, hFavAdd, hFavView,
                      hSeg[0], hSeg[1], hSeg[2], hSeg[3], hSeg[4], hRp[0], hRp[1], hRp[2], hRp[3] };
      for (HWND h : tv) addTip(h);
    }
    layout();
    loadSettings();
    loadFavorites();
    loadExclude();
    setFavButtonLabel();
    applyLanguage();
}

// ---------------------------------------------------------------------
//  メニュー (コードで作成)
// ---------------------------------------------------------------------
static HMENU buildMenu()
{
    HMENU bar = CreateMenu();

    HMENU file = CreatePopupMenu();
    AppendMenuW(file, MF_STRING, IDM_FILE_SETPATH, T(L"結果の保存先を設定(&S)...", L"Set save location(&S)..."));
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVENOW, T(L"今すぐ結果を保存(&N)\tCtrl+S", L"Save results now(&N)\tCtrl+S"));
    AppendMenuW(file, MF_STRING, IDM_FILE_OPENDIR, T(L"保存先のフォルダを開く(&O)", L"Open save folder(&O)"));
    AppendMenuW(file, MF_SEPARATOR, 0, NULL);
    AppendMenuW(file, MF_STRING, IDM_FILE_AUTOSAVE, T(L"検索が終わったら自動で保存する(&A)", L"Auto-save after each search(&A)"));
    AppendMenuW(file, MF_SEPARATOR, 0, NULL);
    AppendMenuW(file, MF_STRING, IDM_FILE_CLEARRES, T(L"表示中の結果をクリア(&C)", L"Clear displayed results(&C)"));
    AppendMenuW(file, MF_STRING, IDM_FILE_CLEARHIST, T(L"検索履歴を初期化(&R)...", L"Clear search history(&R)..."));
    AppendMenuW(file, MF_SEPARATOR, 0, NULL);
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVECSV, T(L"結果をCSVで保存(&V)...", L"Save results as CSV(&V)..."));
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)file, T(L"ファイル(&F)", L"File(&F)"));

    HMENU set = CreatePopupMenu();
    AppendMenuW(set, MF_STRING, IDM_SIZE_1, T(L"スライムチャンクのサイズ： 1×1(&1)", L"Slime chunk size: 1\u00d71(&1)"));
    AppendMenuW(set, MF_STRING, IDM_SIZE_2, T(L"スライムチャンクのサイズ： 2×2(&2)", L"Slime chunk size: 2\u00d72(&2)"));
    AppendMenuW(set, MF_STRING, IDM_SIZE_3, T(L"スライムチャンクのサイズ： 3×3(&3)", L"Slime chunk size: 3\u00d73(&3)"));
    AppendMenuW(set, MF_STRING, IDM_SIZE_4, T(L"スライムチャンクのサイズ： 4×4(&4)", L"Slime chunk size: 4\u00d74(&4)"));
    AppendMenuW(set, MF_STRING, IDM_SIZE_DENSE, T(L"スライムチャンクのサイズ： 密集している場所(&D)", L"Slime chunk size: Dense area(&D)"));
    AppendMenuW(set, MF_SEPARATOR, 0, NULL);
    AppendMenuW(set, MF_STRING, IDM_SET_RESET, T(L"入力を初期値に戻す(&R)", L"Reset inputs to default(&R)"));
    AppendMenuW(set, MF_SEPARATOR, 0, NULL);
    AppendMenuW(set, MF_STRING, IDM_SET_DARK, T(L"ダークモード(&D)", L"Dark mode(&D)"));
    AppendMenuW(set, MF_STRING, IDM_SET_LANG, T(L"English 表示(&L)", L"日本語表示(&L)"));
    AppendMenuW(set, MF_SEPARATOR, 0, NULL);
    AppendMenuW(set, MF_STRING, IDM_APPVER, T(L"アプリケーションのバージョン(&V)... （" APP_VERSION L"）", L"Application version(&V)... (" APP_VERSION L")"));
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)set, T(L"設定(&S)", L"Settings(&S)"));

    HMENU tool = CreatePopupMenu();
    AppendMenuW(tool, MF_STRING, IDM_TOOL_CONVERT, T(L"ブロック⇔チャンク 変換(&C)", L"Block\u2194Chunk Converter(&C)"));
    AppendMenuW(tool, MF_STRING, IDM_TOOL_BATCH, T(L"複数シード一括検索(&M)...", L"Multi-Seed Batch Search(&M)..."));
    AppendMenuW(tool, MF_STRING, IDM_TOOL_EXCLUDE, T(L"除外リストを編集(&X)...", L"Edit Exclusion List(&X)..."));
    AppendMenuW(tool, MF_SEPARATOR, 0, NULL);
    AppendMenuW(tool, MF_STRING, IDM_TOOL_EXPORT, T(L"設定をエクスポート(&E)...", L"Export settings(&E)..."));
    AppendMenuW(tool, MF_STRING, IDM_TOOL_IMPORT, T(L"設定をインポート(&I)...", L"Import settings(&I)..."));
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)tool, T(L"ツール(&T)", L"Tools(&T)"));

    HMENU help = CreatePopupMenu();
    AppendMenuW(help, MF_STRING, IDM_HELP_USAGE, T(L"使い方(&U)\tF1", L"How to use(&U)\tF1"));
    AppendMenuW(help, MF_STRING, IDM_HELP_ABOUTSLIME, T(L"スライムチャンクとは(&S)", L"About slime chunks(&S)"));
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)help, T(L"ヘルプ(&H)", L"Help(&H)"));

    // 右上に置く「終了する」
    AppendMenuW(bar, MF_STRING, IDM_EXIT, T(L"終了する(&X)", L"Exit(&X)"));
    MENUITEMINFOW mi; ZeroMemory(&mi, sizeof mi);
    mi.cbSize = sizeof mi; mi.fMask = MIIM_FTYPE; mi.fType = MFT_STRING | MFT_RIGHTJUSTIFY;
    SetMenuItemInfoW(bar, IDM_EXIT, FALSE, &mi);
    return bar;
}

// ---------------------------------------------------------------------
//  ヘルプなどの文章を表示する小さなウィンドウ
// ---------------------------------------------------------------------
static void showText(const wchar_t* title, const wchar_t* text)
{
    if (hText && IsWindow(hText)) DestroyWindow(hText);
    RECT r = { 0, 0, S(640), S(520) };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    hText = CreateWindowExW(0, L"SlimeTextWnd", title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, hMain, NULL, hInst, NULL);
    if (!hText) return;
    SetWindowTextW(hTextEdit, text);
    SendMessage(hTextEdit, EM_SETSEL, 0, 0);
}

static const wchar_t* USAGE_TEXT_JA =
    L"【スライムチャンク ファインダー の使い方】\r\n\r\n"
    L"■ 基本の流れ\r\n"
    L"1. 「シード値」に、ワールドのシード値を入力します（数字でも文字列でもOK）。\r\n"
    L"2. 「Javaバージョン」を選びます。\r\n"
    L"3. 「中心 X / Z」と「半径」を入力します。半径の下に、調べるチャンク数が表示されます。\r\n"
    L"4. 「探す」を押します（F5キー、入力欄でEnterでもOK）。終わると下の一覧に結果が出ます。\r\n"
    L"   途中で「止める」（Escキー）を押すと、そこまでの結果が表示されます。\r\n"
    L"   検索が終わったとき、ほかのウィンドウを見ていた場合はタスクバーが光って音が鳴ります。\r\n\r\n"
    L"■ 探すサイズを変える（設定メニュー）\r\n"
    L"・1×1 / 2×2 / 3×3 / 4×4：その大きさのスライムチャンクが、すべてそろっている場所を探します。\r\n"
    L"・3×3 と 4×4 で、完全にそろった場所が無かったときは、「惜しい場所」（あと少しで\r\n"
    L"  そろう場所）を個数の多い順に表示します。4×4はとてもまれで、ほぼ見つかりません。\r\n"
    L"・密集している場所：「範囲の一辺」×「範囲の一辺」の四角の中に、スライムチャンクが\r\n"
    L"  「最小個数」以上ある場所を探します。個数が多い順に並びます（例：5×5の中に12個以上）。\r\n"
    L"  重なっている範囲は、いちばん良いものだけ表示します。\r\n"
    L"・「外周1チャンクにスライムチャンクがないものだけ表示」をチェックすると、\r\n"
    L"  まわりが非スライムでぴったりの場所だけになります（1×1〜4×4のとき）。\r\n\r\n"
    L"■ 検索履歴\r\n"
    L"・検索が終わるたびに、条件が履歴に残ります（最大30件）。\r\n"
    L"・「検索履歴」の欄から選ぶと、そのときの条件（シード・中心・半径・サイズなど）を呼び出せます。\r\n"
    L"・「履歴を初期化」ボタン（またはファイルメニュー）で、履歴をすべて消せます。\r\n\r\n"
    L"■ 結果を保存する（ファイルメニュー）\r\n"
    L"・「結果の保存先を設定」で、txtファイルの保存先を決めます。\r\n"
    L"・「検索が終わったら自動で保存する」にチェックすると、検索のたびに保存します。\r\n"
    L"・「今すぐ結果を保存」（Ctrl+S）で、いま表示中の結果をすぐ保存できます。\r\n"
    L"・保存されるのは、条件に合った上位5000か所までです。\r\n"
    L"・「表示中の結果をクリア」で、一覧を空にできます。\r\n\r\n"
    L"■ 一覧の操作\r\n"
    L"・場所をクリックして、下のボタンから「座標をコピー」「/tp コマンドをコピー」\r\n"
    L"  「Chunkbaseで開く」が使えます。ダブルクリックでも座標をコピーできます。\r\n"
    L"・「ここを中心にする」で、選んだ場所を検索の中心にできます（まわりを詳しく調べたいときに便利です）。\r\n"
    L"・「一覧をコピー」で、表示中の結果をまとめてコピーします。\r\n"
    L"・一覧の右に、選んだ場所のまわりのスライムチャンクを簡単な図で表示します（プレビュー）。\r\n\r\n"
    L"■ お気に入り\r\n"
    L"・場所を選んで「☆ お気に入りに追加」を押すと、シードやバージョンごと保存されます（最大300件）。\r\n"
    L"・「★ お気に入りを見る」で一覧に切り替えられます。もう一度押すと検索結果に戻ります。\r\n"
    L"・お気に入り表示中は、ボタンが「🗑 選択を削除」に変わります。\r\n"
    L"・お気に入りの「ここを中心にする」は、シードとバージョンも呼び出した場所に合わせます\r\n"
    L"  （複数のシードのお気に入りを混ぜて保存していても大丈夫です）。\r\n\r\n"
    L"■ 複数シード一括検索（ツールメニュー）\r\n"
    L"・シード値を1行に1つずつ入力すると、まとめて検索できます（最大200件）。\r\n"
    L"・検索条件（中心・半径・サイズなど）は、メイン画面で設定した内容がそのまま使われます。\r\n"
    L"・結果には、どのシードで見つかったかが一緒に表示されます。\r\n\r\n"
    L"■ そのほか\r\n"
    L"・設定メニューの「入力を初期値に戻す」で、中心・半径などを最初の状態に戻せます。\r\n"
    L"・設定メニューの「ダークモード」で、暗い配色に切り替えられます。\r\n"
    L"・設定メニューの「English 表示」で、画面の表示を英語に切り替えられます。\r\n"
    L"・ツールメニューの「ブロック⇔チャンク 変換」で、座標の単位を計算できます。\r\n"
    L"・ツールメニューの「設定をエクスポート／インポート」で、設定・履歴・お気に入りをファイルに\r\n"
    L"  書き出したり、別のパソコンから読み込んだりできます。\r\n"
    L"・ファイルメニューの「結果をCSVで保存」で、Excelなどで開けるCSV形式でも保存できます。\r\n"
    L"・ウィンドウの位置と大きさ、入力した内容は、次回起動時にも引き継がれます。\r\n\r\n"
    L"■ ご注意\r\n"
    L"・Java版のスライムチャンクの位置は、バージョンによらず同じです。バージョンの指定は\r\n"
    L"  Chunkbaseで開くときのリンクに使われます。実際にChunkbaseを開いて、シードと\r\n"
    L"  バージョンが合っているか確認してください。\r\n"
    L"・統合版（Bedrock）は判定式が違うため、対応していません。\r\n"
    L"・半径を大きくすると時間がかかります（最大100万ブロック）。\r\n"
    L"・お気に入り・検索履歴・結果ファイル（txt/CSV）の中身は、現時点では日本語のままです。\r\n";

static const wchar_t* USAGE_TEXT_EN =
    L"[How to use the Slime Chunk Finder]\r\n\r\n"
    L"\u25a0 Basic steps\r\n"
    L"1. Enter your world's seed in \"Seed\" (a number or a text seed both work).\r\n"
    L"2. Choose the \"Java Version\".\r\n"
    L"3. Enter \"Center X / Z\" and \"Radius\". The number of chunks to scan is shown below the radius.\r\n"
    L"4. Press \"Search\" (F5, or Enter in any field). Results appear in the list below when done.\r\n"
    L"   Press \"Stop\" (Esc) to stop early and see the results found so far.\r\n"
    L"   When a search finishes while another window is focused, the taskbar flashes and a sound plays.\r\n\r\n"
    L"\u25a0 Changing the search pattern (Settings menu)\r\n"
    L"\u2022 1\u00d71 / 2\u00d72 / 3\u00d73 / 4\u00d74: finds spots where every chunk in that square is a slime chunk.\r\n"
    L"\u2022 For 3\u00d73 and 4\u00d74, if no perfect match exists, \"near-miss\" spots (just short of a full match)\r\n"
    L"  are shown instead, sorted by count. 4\u00d74 is extremely rare and usually won't be found at all.\r\n"
    L"\u2022 Dense area: finds spots where a \"Box size\" \u00d7 \"Box size\" square contains at least\r\n"
    L"  \"Min count\" slime chunks, sorted by count (e.g. 12+ chunks within a 5\u00d75 box).\r\n"
    L"  Overlapping boxes are merged, keeping only the best one.\r\n"
    L"\u2022 Check \"Show only spots with a clean 1-chunk border\" to only see spots with no slime chunks\r\n"
    L"  touching the outside (for 1\u00d71 through 4\u00d74).\r\n\r\n"
    L"\u25a0 Search history\r\n"
    L"\u2022 Every completed search is added to the history (up to 30 entries).\r\n"
    L"\u2022 Pick an entry from \"Search history\" to restore those settings (seed, center, radius, size, etc.).\r\n"
    L"\u2022 \"Clear history\" (button or File menu) removes all of them.\r\n\r\n"
    L"\u25a0 Saving results (File menu)\r\n"
    L"\u2022 \"Set save location\" chooses where the .txt file is saved.\r\n"
    L"\u2022 \"Auto-save after each search\" saves automatically every time a search finishes.\r\n"
    L"\u2022 \"Save results now\" (Ctrl+S) saves the current results immediately.\r\n"
    L"\u2022 Up to the top 5000 matching spots are saved.\r\n"
    L"\u2022 \"Clear displayed results\" empties the list.\r\n\r\n"
    L"\u25a0 Using the results list\r\n"
    L"\u2022 Select a spot, then use \"Copy coordinates\", \"Copy /tp command\" or \"Open in Chunkbase\"\r\n"
    L"  below the list. Double-click a row to copy its coordinates too.\r\n"
    L"\u2022 \"Set as center\" makes the selected spot the new search center (handy for zooming in nearby).\r\n"
    L"\u2022 \"Copy list\" copies all displayed results at once.\r\n"
    L"\u2022 A small map preview of the selected spot's surrounding chunks appears on the right of the list.\r\n\r\n"
    L"\u25a0 Favorites\r\n"
    L"\u2022 Select a spot and press \"\u2606 Add to favorites\" to save it along with its seed and version\r\n"
    L"  (up to 300 entries).\r\n"
    L"\u2022 \"\u2605 View favorites\" switches the list to your favorites. Press it again to go back.\r\n"
    L"\u2022 While viewing favorites, the button becomes \"\U0001F5D1 Delete selected\".\r\n"
    L"\u2022 \"Set as center\" on a favorite also restores its seed and version\r\n"
    L"  (so it's fine to mix favorites from different seeds).\r\n\r\n"
    L"\u25a0 Multi-Seed Batch Search (Tools menu)\r\n"
    L"\u2022 Enter one seed per line to search them all at once (up to 200 seeds).\r\n"
    L"\u2022 The search settings (center, radius, size, etc.) are taken from the main window.\r\n"
    L"\u2022 Each result shows which seed it was found in.\r\n\r\n"
    L"\u25a0 Other features\r\n"
    L"\u2022 \"Reset inputs to default\" (Settings menu) resets the center, radius, etc.\r\n"
    L"\u2022 \"Dark mode\" (Settings menu) switches to a dark color scheme.\r\n"
    L"\u2022 \"\u65e5\u672c\u8a9e\u8868\u793a\" (Settings menu) switches the interface language.\r\n"
    L"\u2022 \"Block\u2194Chunk Converter\" (Tools menu) converts between the two coordinate systems.\r\n"
    L"\u2022 \"Export/Import settings\" (Tools menu) saves or loads your settings, history and favorites\r\n"
    L"  as a file, so you can move them to another computer.\r\n"
    L"\u2022 \"Save results as CSV\" (File menu) saves results in a format Excel can open.\r\n"
    L"\u2022 The window's position, size and inputs are remembered for next time.\r\n\r\n"
    L"\u25a0 Notes\r\n"
    L"\u2022 Slime chunk positions in Java Edition don't depend on the version. The version you pick\r\n"
    L"  only affects the link used when opening Chunkbase \u2014 please verify the seed and version\r\n"
    L"  actually match there.\r\n"
    L"\u2022 Bedrock Edition uses a different formula and isn't supported.\r\n"
    L"\u2022 A larger radius takes longer to search (maximum 1,000,000 blocks).\r\n"
    L"\u2022 Favorites, history and saved result files (txt/CSV) are currently written in Japanese\r\n"
    L"  regardless of the interface language.\r\n";
#define USAGE_TEXT T(USAGE_TEXT_JA, USAGE_TEXT_EN)

static const wchar_t* ABOUT_SLIME_TEXT_JA =
    L"【スライムチャンクとは】\r\n\r\n"
    L"・ワールドを16×16ブロックに区切った「チャンク」のうち、約10分の1が「スライムチャンク」です。\r\n"
    L"・スライムチャンクでは、高さY40以下の場所でスライムが湧きます（明るさは関係ありません）。\r\n"
    L"・どのチャンクがスライムチャンクかは、シード値で決まります（Java版）。\r\n"
    L"・スライムが湧く範囲は、プレイヤーから24ブロック以上、128ブロック未満の距離です。\r\n"
    L"  そのため、スライムチャンクが広くそろっている場所ほど、効率のよいスライムトラップが作れます。\r\n\r\n"
    L"・3×3がそろう確率は1か所あたり約10億分の1、4×4は約10京分の1と、とても珍しくなります。\r\n";
static const wchar_t* ABOUT_SLIME_TEXT_EN =
    L"[About slime chunks]\r\n\r\n"
    L"\u2022 The world is divided into 16\u00d716-block \"chunks\", and about 1 in 10 of them is a \"slime chunk\".\r\n"
    L"\u2022 Slimes can spawn in slime chunks at Y40 or below, regardless of light level.\r\n"
    L"\u2022 Which chunks are slime chunks is fixed by the world seed (Java Edition).\r\n"
    L"\u2022 Slimes spawn between 24 and 128 blocks from the player. So the larger the area of\r\n"
    L"  connected slime chunks, the more efficient a slime farm built there can be.\r\n\r\n"
    L"\u2022 The odds of a 3\u00d73 all being slime chunks are about 1 in a billion; for 4\u00d74, about 1 in 10 quadrillion.\r\n";
#define ABOUT_SLIME_TEXT T(ABOUT_SLIME_TEXT_JA, ABOUT_SLIME_TEXT_EN)

// ---------------------------------------------------------------------
//  ウィンドウ プロシージャ
// ---------------------------------------------------------------------
static void showVersion()
{
    MessageBoxW(hMain, T(L"スライムチャンク ファインダー\r\nバージョン " APP_VERSION L"\r\n\r\nJava版のスライムチャンクを探すツールです。",
        L"Slime Chunk Finder\r\nVersion " APP_VERSION L"\r\n\r\nA tool for finding slime chunks in Java Edition."),
        T(L"アプリケーションのバージョン", L"Application version"), MB_OK | MB_ICONINFORMATION);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        hMain = hWnd; hMenu = GetMenu(hWnd); onCreate(); return 0;
    case WM_SIZE:
        if (hList) layout();
        return 0;
    case WM_ERASEBKGND:
        drawBackdrop((HDC)wParam);
        return 1;
    case WM_CTLCOLOREDIT:
        SetTextColor((HDC)wParam, C_INK); SetBkColor((HDC)wParam, C_WHITE);
        return (LRESULT)g_brWhite;
    case WM_CTLCOLORSTATIC:
        {
            HDC cdc = (HDC)wParam; HWND c = (HWND)lParam;
            COLORREF fg = C_INK, bg = C_WHITE; HBRUSH br = g_brWhite;
            if (c == hStatus) { fg = C_DEEP; bg = C_MINT; br = g_brMint; }
            else if (c == hNote) { fg = C_MUTED; bg = C_MINT; br = g_brMint; }
            else if (c == hPattern) fg = C_DEEP;
            else if (c == hChunks) fg = C_TEAL;
            else if (c == hSavePath) fg = C_MUTED;
            else if (c == hDK || c == hDM) { fg = C_MUTED; bg = C_OFF; br = g_brOff; }   // 無効な入力欄
            SetTextColor(cdc, fg); SetBkColor(cdc, bg);
            return (LRESULT)br;
        }
    case WM_DRAWITEM:
        drawButton((const DRAWITEMSTRUCT*)lParam);
        return TRUE;
    case WM_GETMINMAXINFO:
        { MINMAXINFO* mi = (MINMAXINFO*)lParam; mi->ptMinTrackSize.x = S(880); mi->ptMinTrackSize.y = S(840); return 0; }
    case WM_TIMER:
        {
            if (g_lastNearest) {
                LONG rr = g_nearRing, mr = g_nearMaxRing; if (mr < 1) mr = 1;
                double p = (double)rr / mr; if (p > 1) p = 1;
                SendMessage(hProg, PBM_SETPOS, (WPARAM)(p * 1000), 0);
                double el = (GetTickCount() - g_t0) / 1000.0;
                wchar_t b[220];
                _snwprintf(b, 220, T(L"探索中… 半径 %ld / %ld チャンク　%ls　経過 %.1f秒", L"Searching\u2026 radius %ld / %ld chunks\u3000%ls\u3000%.1fs"),
                    (long)rr, (long)mr, g_nearFound ? T(L"（見つかりました）", L"(found)") : T(L"（未発見）", L"(not yet found)"), el);
                b[219] = 0; setStatus(b);
                return 0;
            }
            LONG d = g_done, t = g_nbands; if (t < 1) t = 1;
            double p = (double)d / t; if (p > 1) p = 1;
            SendMessage(hProg, PBM_SETPOS, (WPARAM)(p * 1000), 0);
            double el = (GetTickCount() - g_t0) / 1000.0;
            wchar_t eta[48] = L"";
            if (p > 0.03 && p < 1) {
                double rem = el * (1 - p) / p;
                if (rem >= 60) _snwprintf(eta, 48, T(L"　残り約%.0f分", L"  ~%.0f min left"), ceil(rem / 60)); else _snwprintf(eta, 48, T(L"　残り約%.0f秒", L"  ~%.0fs left"), ceil(rem));
                eta[47] = 0;
            }
            wchar_t b[220];
            _snwprintf(b, 220, T(L"探索中… %.1f%%　%ls：%ld　経過 %.0f秒%ls", L"Searching\u2026 %.1f%%\u3000%ls: %ld\u3000%.0fs%ls"), p * 100, P.dense ? T(L"ヒット数", L"Hits") : T(L"完全一致", L"Exact matches"),
                (long)(P.dense ? g_totalHits : g_exactHits), el, eta);
            b[219] = 0; setStatus(b);
            return 0;
        }
    case WM_DONE:
        {
            KillTimer(hWnd, TIMER_ID);
            if (g_lastNearest) {
                SendMessage(hProg, PBM_SETPOS, 1000, 0);
                fillList();
                double secs = (GetTickCount() - g_t0) / 1000.0;
                wchar_t b[260];
                if (g_nres > 0) {
                    long long bx, bz; centerOf(g_res[0], &bx, &bz);
                    _snwprintf(b, 260, T(L"%ls　見つかりました：X %lld, Z %lld（中心から約%.0f）　所要時間 %.2f秒",
                        L"%ls\u3000Found: X %lld, Z %lld (~%.0f from center)\u3000%.2fs"),
                        g_stop ? T(L"停止しました", L"Stopped") : T(L"完了しました", L"Done"), bx, bz, g_res[0].dist, secs);
                } else {
                    _snwprintf(b, 260, T(L"%ls　見つかりませんでした（半径 %lld ブロックまで探索）　所要時間 %.2f秒",
                        L"%ls\u3000Not found (searched up to radius %lld blocks)\u3000%.2fs"),
                        g_stop ? T(L"停止しました", L"Stopped") : T(L"完了しました", L"Done"), P.R, secs);
                }
                b[259] = 0; setStatus(b);
                setRunning(0);
                if (!g_stop) addHistory((int)secs, g_nres > 0 ? 1 : 0);
                if (g_autoSave && g_savePath[0] && g_nres > 0) doSave(false);
                if (g_nres == 0 && !g_stop) MessageBoxW(hWnd, T(L"この範囲には見つかりませんでした。半径を広げるか、条件をゆるめてみてください。", L"Nothing was found in this range. Try a larger radius or looser conditions."), T(L"結果", L"Result"), MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            SendMessage(hProg, PBM_SETPOS, g_stop ? (WPARAM)(1000.0 * g_done / (g_nbands ? g_nbands : 1)) : 1000, 0);
            fillList();
            double secs = (GetTickCount() - g_t0) / 1000.0;
            long hits = (long)(P.dense ? g_totalHits : g_exactHits);
            const wchar_t* head = g_stop ? T(L"停止しました", L"Stopped") : T(L"完了しました", L"Done");
            wchar_t b[320];
            if (!P.dense && g_exactHits == 0 && g_nres > 0)
                _snwprintf(b, 320, T(L"%ls　完全にそろった場所は見つかりませんでした。惜しい場所（個数が多い順）を %d か所表示します　所要時間 %.1f秒",
                    L"%ls\u3000No perfect match was found. Showing %d near-miss spot(s) (most chunks first)\u3000%.1fs"), head, g_shown, secs);
            else
                _snwprintf(b, 320, T(L"%ls　%ls：%ld か所　表示：%d か所%ls　所要時間 %.1f秒", L"%ls\u3000%ls: %ld\u3000Showing: %d%ls\u3000%.1fs"), head, P.dense ? T(L"ヒット数", L"Hits") : T(L"完全一致", L"Exact matches"), hits, g_shown,
                    g_nres > MAX_SHOW ? T(L"（上位500か所）", L" (top 500)") : L"", secs);
            b[319] = 0; setStatus(b);
            setRunning(0);
            if (!g_stop) addHistory((int)secs, hits);
            if (g_autoSave && g_savePath[0] && g_nres > 0) doSave(false);
            if (GetForegroundWindow() != hWnd) {      // ほかのウィンドウを見ているときは知らせる
                FLASHWINFO fi; ZeroMemory(&fi, sizeof fi);
                fi.cbSize = sizeof fi; fi.hwnd = hWnd; fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG; fi.uCount = 3;
                FlashWindowEx(&fi); MessageBeep(MB_ICONASTERISK);
            }
            if (g_nres == 0 && !g_stop) MessageBoxW(hWnd, T(L"この範囲には見つかりませんでした。半径を広げるか、条件をゆるめてみてください。", L"Nothing was found in this range. Try a larger radius or looser conditions."), T(L"結果", L"Result"), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
    case WM_CLOSE:
        if (g_running && MessageBoxW(hWnd, T(L"検索の途中です。終了してもよいですか？", L"A search is still running. Are you sure you want to exit?"), T(L"終了の確認", L"Confirm exit"), MB_YESNO | MB_ICONQUESTION) != IDYES) return 0;
        if (hConv && IsWindow(hConv)) DestroyWindow(hConv);
        if (hText && IsWindow(hText)) DestroyWindow(hText);
        if (hBatch && IsWindow(hBatch)) DestroyWindow(hBatch);
        if (hExcl && IsWindow(hExcl)) DestroyWindow(hExcl);
        DestroyWindow(hWnd);
        return 0;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case ID_GO: if (!g_running) startSearch(); return 0;
            case ID_STOP: g_stop = 1; return 0;
            case IDM_FILE_SETPATH: chooseSavePath(); return 0;
            case IDM_FILE_SAVENOW: doSave(true); return 0;
            case IDM_FILE_OPENDIR:
                if (!g_savePath[0]) { MessageBoxW(hWnd, L"保存先が設定されていません。", L"保存先", MB_OK | MB_ICONINFORMATION); return 0; }
                if (GetFileAttributesW(g_savePath) != INVALID_FILE_ATTRIBUTES) {
                    wchar_t a[MAX_PATH * 2 + 16]; _snwprintf(a, MAX_PATH * 2 + 15, L"/select,\"%ls\"", g_savePath); a[MAX_PATH * 2 + 15] = 0;
                    ShellExecuteW(hWnd, L"open", L"explorer.exe", a, NULL, SW_SHOWNORMAL);
                } else {
                    wchar_t d[MAX_PATH * 2]; wcscpy(d, g_savePath);
                    wchar_t* sl = wcsrchr(d, L'\\'); if (sl) *sl = 0;
                    ShellExecuteW(hWnd, L"open", d, NULL, NULL, SW_SHOWNORMAL);
                }
                return 0;
            case IDM_FILE_AUTOSAVE:
                g_autoSave = !g_autoSave;
                CheckMenuItem(hMenu, IDM_FILE_AUTOSAVE, MF_BYCOMMAND | (g_autoSave ? MF_CHECKED : MF_UNCHECKED));
                if (g_autoSave && !g_savePath[0]) chooseSavePath();
                updateSavePathLabel();
                return 0;
            case IDM_SIZE_1: case IDM_SIZE_2: case IDM_SIZE_3: case IDM_SIZE_4: case IDM_SIZE_DENSE:
                if (!g_running) setMode(wmId - IDM_SIZE_1);
                return 0;
            case IDM_APPVER: case IDM_ABOUT: showVersion(); return 0;
            case IDM_HELP_USAGE: showText(L"使い方", USAGE_TEXT); return 0;
            case IDM_HELP_ABOUTSLIME: showText(L"スライムチャンクとは", ABOUT_SLIME_TEXT); return 0;
            case IDM_EXIT: SendMessage(hWnd, WM_CLOSE, 0, 0); return 0;
            case IDM_FILE_CLEARHIST: case ID_HISTCLEAR: clearHistory(); return 0;
            case IDM_FILE_CLEARRES: clearResults(); return 0;
            case IDM_FILE_SAVECSV: doSaveCSV(); return 0;
            case IDM_SET_RESET: resetInputs(); return 0;
            case IDM_SET_DARK:
                applyTheme(!g_dark);
                CheckMenuItem(hMenu, IDM_SET_DARK, MF_BYCOMMAND | (g_dark ? MF_CHECKED : MF_UNCHECKED));
                DeleteObject(g_brMint); DeleteObject(g_brWhite); DeleteObject(g_brOff);
                g_brMint = CreateSolidBrush(C_MINT); g_brWhite = CreateSolidBrush(C_WHITE); g_brOff = CreateSolidBrush(C_OFF);
                ListView_SetBkColor(hList, C_WHITE); ListView_SetTextBkColor(hList, C_WHITE); ListView_SetTextColor(hList, C_INK);
                SendMessage(hProg, PBM_SETBARCOLOR, 0, C_SLIME);
                applyTitleBar();
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
                setStatus(T(g_dark ? L"ダークモードにしました。" : L"ライトモードにしました。", g_dark ? L"Switched to dark mode." : L"Switched to light mode."));
                return 0;
            case IDM_SET_LANG:
                if (hConv && IsWindow(hConv)) DestroyWindow(hConv);
                if (hText && IsWindow(hText)) DestroyWindow(hText);
                if (hBatch && IsWindow(hBatch)) DestroyWindow(hBatch);
                if (hExcl && IsWindow(hExcl)) DestroyWindow(hExcl);
                g_lang = g_lang == 0 ? 1 : 0;
                applyLanguage();
                setStatus(g_lang == 0 ? L"日本語表示にしました。" : L"Switched to English.");
                return 0;
            case IDM_TOOL_CONVERT: showConverter(); return 0;
            case IDM_TOOL_BATCH: showBatchSearch(); return 0;
            case IDM_TOOL_EXCLUDE: showExcludeList(); return 0;
            case IDM_TOOL_EXPORT: exportSettings(); return 0;
            case IDM_TOOL_IMPORT: importSettings(); return 0;
            case ID_HIST: if (HIWORD(wParam) == CBN_SELCHANGE) restoreHistory(); return 0;
            case ID_FAVADD: if (g_viewFav) removeFavoriteSelected(); else addFavoriteFromSelection(); return 0;
            case ID_FAVVIEW: toggleFavView(); return 0;
            case ID_CENTER:
                { int i = selIndex(); if (i < 0) return 0; wchar_t b[64]; long long bx, bz;
                  if (g_viewFav) { bx = g_fav[i].X; bz = g_fav[i].Z; SetWindowTextW(hSeed, g_fav[i].seed); SendMessage(hVer, CB_SETCURSEL, g_fav[i].ver, 0); }
                  else centerOf(g_res[i], &bx, &bz);
                  _snwprintf(b, 64, L"%lld", bx); SetWindowTextW(hX, b); _snwprintf(b, 64, L"%lld", bz); SetWindowTextW(hZ, b);
                  updateChunks();
                  wchar_t m[160]; _snwprintf(m, 160, L"検索の中心を (%lld, %lld) に設定しました。%ls", bx, bz, g_viewFav ? L"シードとバージョンも合わせました。" : L""); m[159] = 0; setStatus(m); return 0; }
            case ID_DK: case ID_DM:
                if (HIWORD(wParam) == EN_CHANGE && g_mode == 4) setMode(4);
                return 0;
            case ID_R:
                if (HIWORD(wParam) == EN_CHANGE) { updateChunks(); for (int i = 0; i < 4; i++) InvalidateRect(hRp[i], NULL, FALSE); }
                return 0;
            case ID_SEG0: case ID_SEG0 + 1: case ID_SEG0 + 2: case ID_SEG0 + 3: case ID_SEG0 + 4:
                if (!g_running) setMode(wmId - ID_SEG0);
                return 0;
            case ID_RP0: case ID_RP0 + 1: case ID_RP0 + 2: case ID_RP0 + 3:
                if (!g_running) { wchar_t b[24]; _snwprintf(b, 24, L"%lld", RP_VAL[wmId - ID_RP0]); SetWindowTextW(hR, b); }
                return 0;
            case ID_COPYXZ:
                { int i = selIndex(); if (i < 0) return 0; wchar_t b[64]; long long bx, bz;
                  if (g_viewFav) { bx = g_fav[i].X; bz = g_fav[i].Z; } else centerOf(g_res[i], &bx, &bz);
                  _snwprintf(b, 64, L"%lld %lld", bx, bz); copyText(b); setStatus(L"座標をコピーしました。"); return 0; }
            case ID_COPYTP:
                { int i = selIndex(); if (i < 0) return 0; wchar_t b[96]; long long bx, bz;
                  if (g_viewFav) { bx = g_fav[i].X; bz = g_fav[i].Z; } else centerOf(g_res[i], &bx, &bz);
                  _snwprintf(b, 96, L"/tp @s %lld ~ %lld", bx, bz); copyText(b); setStatus(L"/tp コマンドをコピーしました。"); return 0; }
            case ID_OPEN:
                { int i = selIndex(); if (i < 0) return 0; wchar_t u[400]; long long bx, bz; const wchar_t* seed; const wchar_t* verk;
                  if (g_viewFav) { bx = g_fav[i].X; bz = g_fav[i].Z; seed = g_fav[i].seed; verk = verKeys[g_fav[i].ver]; }
                  else { centerOf(g_res[i], &bx, &bz); seed = g_seedStr; verk = g_verKey; }
                  _snwprintf(u, 400, L"https://www.chunkbase.com/apps/slime-finder#seed=%ls&platform=java_%ls&x=%lld&z=%lld&zoom=0.75",
                      seed, verk, bx, bz);
                  ShellExecuteW(hWnd, L"open", u, NULL, NULL, SW_SHOWNORMAL); return 0; }
            case ID_COPYALL:
                { if (g_shown == 0) { MessageBoxW(hWnd, L"コピーする結果がありません。", L"確認", MB_OK | MB_ICONINFORMATION); return 0; }
                  if (g_viewFav) {
                      WBuf b = { 0, 0, 0 }; wappend(b, L"お気に入り一覧\r\n\r\n");
                      for (int i = 0; i < g_shown; i++) wappend(b, L"%d. X %lld, Z %lld（%ls）　シード %ls（%ls）　追加 %ls\r\n",
                          i + 1, g_fav[i].X, g_fav[i].Z, g_fav[i].pat, g_fav[i].seed, verNames[g_fav[i].ver], g_fav[i].stamp);
                      copyText(b.p); free(b.p); setStatus(L"お気に入り一覧をコピーしました。"); return 0;
                  }
                  WBuf b = { 0, 0, 0 }; buildReport(b, g_shown); copyText(b.p); free(b.p); setStatus(L"一覧をコピーしました。"); return 0; }
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
    case WM_NOTIFY:
        {
            NMHDR* nh = (NMHDR*)lParam;
            if (nh->code == TTN_GETDISPINFOW) {
                NMTTDISPINFOW* di = (NMTTDISPINFOW*)lParam;
                const wchar_t* tx = tipFor(GetDlgCtrlID((HWND)nh->idFrom));
                if (tx) di->lpszText = (LPWSTR)tx;
                return 0;
            }
            if (nh->hwndFrom == hList && nh->code == NM_RCLICK) { POINT pt; GetCursorPos(&pt); showResultMenu(pt); return 0; }
            if (nh->hwndFrom == hList && nh->code == LVN_COLUMNCLICK) { onColumnClick(((LPNMLISTVIEW)lParam)->iSubItem); return 0; }
            if (nh->hwndFrom == hList && nh->code == NM_DBLCLK) { SendMessage(hWnd, WM_COMMAND, ID_COPYXZ, 0); return 0; }
            if (nh->hwndFrom == hList && nh->code == LVN_ITEMCHANGED) {
                LPNMLISTVIEW lv = (LPNMLISTVIEW)lParam;
                if ((lv->uChanged & LVIF_STATE) && (lv->uNewState & LVIS_SELECTED)) InvalidateRect(hPreview, NULL, TRUE);
                return 0;
            }
            if (nh->hwndFrom == hList && nh->code == NM_CUSTOMDRAW) {   // 一覧を縞模様＋やさしい選択色に
                LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)lParam;
                switch (cd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
                case CDDS_POSTPAINT:
                    if (ListView_GetItemCount(hList) == 0) {          // 結果がまだ無いときの案内
                        RECT cr, hr; GetClientRect(hList, &cr);
                        HWND hh = ListView_GetHeader(hList); if (hh) { GetWindowRect(hh, &hr); cr.top += hr.bottom - hr.top; }
                        InflateRect(&cr, -S(24), -S(24));
                        const wchar_t* msg = g_running ? T(L"探しています…", L"Searching\u2026")
                            : g_viewFav ? T(L"お気に入りはまだありません。\n検索結果で場所を選んで「☆ お気に入りに追加」を押すと、ここに並びます。", L"No favorites yet.\nSelect a spot in the results and press \u201c\u2606 Add to favorites\u201d.")
                            : T(L"ここに検索結果が表示されます。\n上でサイズを選んで「探す」（F5）を押してください。\n座標は貼り付けるだけで X・Z に入ります。", L"Results will appear here.\nPick a size above and press Search (F5).\nYou can paste coordinates straight into X and Z.");
                        HGDIOBJ of = SelectObject(cd->nmcd.hdc, hFont);
                        SetBkMode(cd->nmcd.hdc, TRANSPARENT); SetTextColor(cd->nmcd.hdc, C_MUTED);
                        DrawTextW(cd->nmcd.hdc, msg, -1, &cr, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
                        SelectObject(cd->nmcd.hdc, of);
                    }
                    return CDRF_DODEFAULT;
                case CDDS_ITEMPREPAINT: return CDRF_NOTIFYSUBITEMDRAW;
                case (CDDS_ITEMPREPAINT | CDDS_SUBITEM):
                    {
                        int i = (int)cd->nmcd.dwItemSpec;
                        bool sel = (ListView_GetItemState(hList, i, LVIS_SELECTED) & LVIS_SELECTED) != 0;
                        if (sel) { cd->clrTextBk = C_SEL; cd->clrText = C_DEEP; cd->nmcd.uItemState &= ~(CDIS_SELECTED | CDIS_FOCUS); }
                        else { cd->clrTextBk = (i & 1) ? C_ROW : C_WHITE; cd->clrText = C_INK; }
                        return CDRF_NEWFONT;
                    }
                }
                return CDRF_DODEFAULT;
            }
            break;
        }
    case WM_DESTROY:
        g_stop = 1; saveSettings(); DeleteObject(g_brMint); DeleteObject(g_brWhite); DeleteObject(g_brOff); PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static LRESULT CALLBACK TextWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_CREATE:
        hTextEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            0, 0, 10, 10, h, NULL, hInst, NULL);
        SendMessage(hTextEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        return 0;
    case WM_SIZE: { RECT rc; GetClientRect(h, &rc); MoveWindow(hTextEdit, 0, 0, rc.right, rc.bottom, TRUE); return 0; }
    case WM_SETFOCUS: SetFocus(hTextEdit); return 0;
    case WM_DESTROY: hText = NULL; hTextEdit = NULL; return 0;
    }
    return DefWindowProc(h, m, w, l);
}

// ---------------------------------------------------------------------
//  結果プレビュー (選んだ場所のまわりのスライムチャンクを簡単な図で表示)
// ---------------------------------------------------------------------
static void paintPreview(HWND h, HDC dc, const RECT& rc)
{
    HBRUSH bg = CreateSolidBrush(C_WHITE); FillRect(dc, &rc, bg); DeleteObject(bg);
    SetBkMode(dc, TRANSPARENT);
    int i = -1;
    if (!g_viewFav) i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    SelectObject(dc, hFontBold); SetTextColor(dc, C_DEEP);
    RECT tr = rc; tr.left += S(8); tr.top += S(6); tr.right -= S(8);
    if (g_viewFav) { DrawTextW(dc, T(L"プレビュー", L"Preview"), -1, &tr, DT_LEFT); tr.top += S(20);
        SelectObject(dc, hFont); SetTextColor(dc, C_MUTED);
        DrawTextW(dc, T(L"お気に入り表示中は\r\n省略します。", L"Not shown while\r\nviewing favorites."), -1, &tr, DT_LEFT); return; }
    if (i < 0 || i >= g_shown || !g_res) { DrawTextW(dc, T(L"プレビュー", L"Preview"), -1, &tr, DT_LEFT); tr.top += S(20);
        SelectObject(dc, hFont); SetTextColor(dc, C_MUTED);
        DrawTextW(dc, T(L"一覧から場所を\r\n選ぶと、まわりの\r\n地図が出ます。", L"Select a spot from\r\nthe list to see a\r\nmap of the area."), -1, &tr, DT_LEFT); return; }
    DrawTextW(dc, T(L"プレビュー", L"Preview"), -1, &tr, DT_LEFT);
    const Res& r = g_res[i];
    int K = P.K;
    int pad = 4;                       // K×K の外側に足す余白チャンク数
    int N = K + pad * 2; if (N > 21) N = 21;
    RECT gr = rc; gr.left += S(8); gr.top += S(28); gr.right -= S(8); gr.bottom -= S(8);
    int gw = gr.right - gr.left, gh = gr.bottom - gr.top;
    int cell = (gw < gh ? gw : gh) / N; if (cell < 3) cell = 3;
    int ox = gr.left + (gw - cell * N) / 2, oy = gr.top;
    HPEN pn = (HPEN)GetStockObject(NULL_PEN); HGDIOBJ op = SelectObject(dc, pn);
    for (int a = 0; a < N; a++) for (int b = 0; b < N; b++) {
        int cx = r.cx - pad + a, cz = r.cz - pad + b;
        bool inBox = (a >= pad && a < pad + K && b >= pad && b < K + pad);
        bool slime = slimeAt(cx, cz) != 0;
        COLORREF col = slime ? C_SLIME : C_ROW;
        HBRUSH br = CreateSolidBrush(col); HGDIOBJ ob = SelectObject(dc, br);
        Rectangle(dc, ox + a * cell, oy + b * cell, ox + a * cell + cell - 1, oy + b * cell + cell - 1);
        SelectObject(dc, ob); DeleteObject(br);
    }
    SelectObject(dc, op);
    HPEN bp = CreatePen(PS_SOLID, 2, C_DEEP); HGDIOBJ obp = SelectObject(dc, bp);
    HBRUSH nb = (HBRUSH)GetStockObject(NULL_BRUSH); HGDIOBJ onb = SelectObject(dc, nb);
    Rectangle(dc, ox + pad * cell, oy + pad * cell, ox + (pad + K) * cell, oy + (pad + K) * cell);
    SelectObject(dc, obp); SelectObject(dc, onb); DeleteObject(bp);
    RECT lr = rc; lr.left += S(8); lr.right -= S(8); lr.top = oy + N * cell + S(8); lr.bottom = rc.bottom - S(4);
    SelectObject(dc, hFont); SetTextColor(dc, C_MUTED);
    wchar_t t[64]; _snwprintf(t, 64, T(L"緑＝スライムチャンク\r\n枠＝%d×%d", L"Green = slime chunk\r\nBox = %d×%d"), K, K); t[63] = 0;
    DrawTextW(dc, t, -1, &lr, DT_LEFT);
}
static LRESULT CALLBACK PreviewWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT:
        { PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); RECT rc; GetClientRect(h, &rc); paintPreview(h, dc, rc); EndPaint(h, &ps); return 0; }
    }
    return DefWindowProc(h, m, w, l);
}

// ---------------------------------------------------------------------
//  ブロック⇔チャンク 変換ツール
// ---------------------------------------------------------------------
static void updateConv()
{
    if (!hConv) return;
    wchar_t b[64], out[200];
    long long bx = 0, bz = 0, cx = 0, cz = 0;
    GetWindowTextW(hCvBX, b, 64); bx = _wtoi64(b); GetWindowTextW(hCvBZ, b, 64); bz = _wtoi64(b);
    long long ccx = (long long)floor((double)bx / 16.0), ccz = (long long)floor((double)bz / 16.0);
    _snwprintf(out, 200, L"チャンク座標： X %lld, Z %lld\r\nリージョン座標： X %lld, Z %lld （r.%lld.%lld.mca）",
        ccx, ccz, (long long)floor((double)ccx / 32.0), (long long)floor((double)ccz / 32.0),
        (long long)floor((double)ccx / 32.0), (long long)floor((double)ccz / 32.0));
    out[199] = 0; SetWindowTextW(hCvOut1, out);
    GetWindowTextW(hCvCX, b, 64); cx = _wtoi64(b); GetWindowTextW(hCvCZ, b, 64); cz = _wtoi64(b);
    _snwprintf(out, 200, L"ブロック範囲： X %lld〜%lld, Z %lld〜%lld\r\n中心： X %lld, Z %lld",
        cx * 16, cx * 16 + 15, cz * 16, cz * 16 + 15, cx * 16 + 8, cz * 16 + 8);
    out[199] = 0; SetWindowTextW(hCvOut2, out);
}
static LRESULT CALLBACK ConvWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_CREATE:
        {
            hCvHdr1 = CreateWindowExW(0, L"STATIC", L"ブロック座標 →", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hCvLblBX = CreateWindowExW(0, L"STATIC", L"X", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hCvBX = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 10, 10, h, (HMENU)(INT_PTR)ID_CVBX, hInst, NULL);
            hCvLblBZ = CreateWindowExW(0, L"STATIC", L"Z", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hCvBZ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 10, 10, h, (HMENU)(INT_PTR)ID_CVBZ, hInst, NULL);
            hCvOut1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hCvHdr2 = CreateWindowExW(0, L"STATIC", L"チャンク座標 →", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hCvLblCX = CreateWindowExW(0, L"STATIC", L"X", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hCvCX = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 10, 10, h, (HMENU)(INT_PTR)ID_CVCX, hInst, NULL);
            hCvLblCZ = CreateWindowExW(0, L"STATIC", L"Z", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hCvCZ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 10, 10, h, (HMENU)(INT_PTR)ID_CVCZ, hInst, NULL);
            hCvOut2 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY, 0, 0, 10, 10, h, NULL, hInst, NULL);
            HWND kids[] = { hCvHdr1, hCvLblBX, hCvBX, hCvLblBZ, hCvBZ, hCvOut1, hCvHdr2, hCvLblCX, hCvCX, hCvLblCZ, hCvCZ, hCvOut2 };
            for (HWND k : kids) SendMessage(k, WM_SETFONT, (WPARAM)hFont, TRUE);
            updateConv();
            return 0;
        }
    case WM_SIZE:
        {
            RECT rc; GetClientRect(h, &rc); int W = rc.right, M = S(16), y = M;
            MoveWindow(hCvHdr1, M, y, W - 2 * M, S(20), TRUE); y += S(26);
            MoveWindow(hCvLblBX, M, y + S(4), S(16), S(20), TRUE);
            MoveWindow(hCvBX, M + S(20), y, S(150), S(24), TRUE);
            MoveWindow(hCvLblBZ, M + S(180), y + S(4), S(16), S(20), TRUE);
            MoveWindow(hCvBZ, M + S(200), y, S(150), S(24), TRUE); y += S(34);
            MoveWindow(hCvOut1, M, y, W - 2 * M, S(56), TRUE); y += S(72);
            MoveWindow(hCvHdr2, M, y, W - 2 * M, S(20), TRUE); y += S(26);
            MoveWindow(hCvLblCX, M, y + S(4), S(16), S(20), TRUE);
            MoveWindow(hCvCX, M + S(20), y, S(150), S(24), TRUE);
            MoveWindow(hCvLblCZ, M + S(180), y + S(4), S(16), S(20), TRUE);
            MoveWindow(hCvCZ, M + S(200), y, S(150), S(24), TRUE); y += S(34);
            MoveWindow(hCvOut2, M, y, W - 2 * M, S(56), TRUE);
            return 0;
        }
    case WM_COMMAND:
        if (HIWORD(w) == EN_CHANGE) { updateConv(); return 0; }
        break;
    case WM_SETFOCUS: SetFocus(hCvBX); return 0;
    case WM_DESTROY: hConv = NULL; return 0;
    }
    return DefWindowProc(h, m, w, l);
}
static void showConverter()
{
    if (hConv && IsWindow(hConv)) { SetForegroundWindow(hConv); return; }
    RECT r = { 0, 0, S(420), S(320) };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    hConv = CreateWindowExW(0, L"SlimeConvWnd", T(L"ブロック⇔チャンク 変換", L"Block\u2194Chunk Converter"), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, hMain, NULL, hInst, NULL);
}

// ---------------------------------------------------------------------
//  除外リスト (指定した場所の周りを検索結果から除く)
// ---------------------------------------------------------------------
static void exclToString(const ExclZone& z, wchar_t* out, size_t n)
{
    wchar_t note[80]; wcsncpy(note, z.note, 79); note[79] = 0; for (wchar_t* q = note; *q; q++) if (*q == L'|') *q = L'/';
    _snwprintf(out, n, L"%lld|%lld|%lld|%ls", z.X, z.Z, z.R, note); out[n - 1] = 0;
}
static bool exclFromString(const wchar_t* s, ExclZone& z)
{
    wchar_t buf[200]; wcsncpy(buf, s, 199); buf[199] = 0;
    wchar_t* f[4]; int n = 0; wchar_t* p = buf; f[n++] = p;
    while (*p && n < 4) { if (*p == L'|') { *p = 0; f[n++] = p + 1; } p++; }
    if (n < 4) return false;
    ZeroMemory(&z, sizeof z);
    z.X = _wtoi64(f[0]); z.Z = _wtoi64(f[1]); z.R = _wtoi64(f[2]); wcsncpy(z.note, f[3], 39);
    return z.R > 0;
}
static void saveExclude()
{
    wchar_t ini[MAX_PATH * 2], b[220], k[16];
    iniPath(ini, MAX_PATH * 2);
    WritePrivateProfileStringW(L"exclude", NULL, NULL, ini);
    _snwprintf(b, 16, L"%d", g_nexcl); WritePrivateProfileStringW(L"exclude", L"Count", b, ini);
    for (int i = 0; i < g_nexcl; i++) { _snwprintf(k, 16, L"Z%d", i); exclToString(g_excl[i], b, 220); WritePrivateProfileStringW(L"exclude", k, b, ini); }
    WritePrivateProfileStringW(L"main", L"UseExclude", g_useExclude ? L"1" : L"0", ini);
}
static void loadExclude()
{
    wchar_t ini[MAX_PATH * 2], b[220], k[16];
    iniPath(ini, MAX_PATH * 2);
    g_nexcl = 0;
    int cnt = (int)GetPrivateProfileIntW(L"exclude", L"Count", 0, ini);
    if (cnt > EXCL_MAX) cnt = EXCL_MAX;
    for (int i = 0; i < cnt; i++) { _snwprintf(k, 16, L"Z%d", i); GetPrivateProfileStringW(L"exclude", k, L"", b, 220, ini);
        ExclZone z; if (b[0] && exclFromString(b, z)) g_excl[g_nexcl++] = z; }
    g_useExclude = GetPrivateProfileIntW(L"main", L"UseExclude", 0, ini) != 0;
}
static void exclFillList()
{
    ListView_DeleteAllItems(hExList);
    for (int i = 0; i < g_nexcl; i++) {
        wchar_t t[64];
        LVITEMW it; ZeroMemory(&it, sizeof it); it.mask = LVIF_TEXT; it.iItem = i;
        _snwprintf(t, 64, L"%d", i + 1); it.pszText = t; ListView_InsertItem(hExList, &it);
        _snwprintf(t, 64, L"%lld", g_excl[i].X); ListView_SetItemText(hExList, i, 1, t);
        _snwprintf(t, 64, L"%lld", g_excl[i].Z); ListView_SetItemText(hExList, i, 2, t);
        _snwprintf(t, 64, L"%lld", g_excl[i].R); ListView_SetItemText(hExList, i, 3, t);
        ListView_SetItemText(hExList, i, 4, g_excl[i].note);
    }
}
static void exclAddCol(int i, const wchar_t* t, int w)
{
    LVCOLUMNW c; ZeroMemory(&c, sizeof c); c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; c.pszText = (LPWSTR)t; c.cx = S(w); c.iSubItem = i;
    ListView_InsertColumn(hExList, i, &c);
}
static void exclAdd()
{
    if (g_nexcl >= EXCL_MAX) { MessageBoxW(hExcl, T(L"除外リストは100件までです。", L"The exclusion list can hold up to 100 entries."), T(L"確認", L"Confirm"), MB_OK | MB_ICONWARNING); return; }
    wchar_t bx[32], bz[32], br[32], note[64];
    GetWindowTextW(hExX, bx, 32); GetWindowTextW(hExZ, bz, 32); GetWindowTextW(hExR, br, 32); GetWindowTextW(hExNote, note, 64);
    long long R = _wtoi64(br);
    if (R <= 0) { MessageBoxW(hExcl, T(L"半径は1以上の数字で指定してください。", L"Radius must be a number of 1 or more."), T(L"入力エラー", L"Input error"), MB_OK | MB_ICONWARNING); return; }
    ExclZone z; z.X = _wtoi64(bx); z.Z = _wtoi64(bz); z.R = R; wcsncpy(z.note, note, 39); z.note[39] = 0;
    g_excl[g_nexcl++] = z;
    saveExclude(); exclFillList();
    SetWindowTextW(hExNote, L"");
}
static void exclRemove()
{
    int i = ListView_GetNextItem(hExList, -1, LVNI_SELECTED);
    if (i < 0 || i >= g_nexcl) { MessageBoxW(hExcl, T(L"一覧から除外ゾーンを選んでください。", L"Please select a zone from the list."), T(L"選択してください", L"Please select"), MB_OK | MB_ICONINFORMATION); return; }
    memmove(&g_excl[i], &g_excl[i + 1], sizeof(ExclZone) * (g_nexcl - i - 1));
    g_nexcl--; saveExclude(); exclFillList();
}
static LRESULT CALLBACK ExclWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_CREATE:
        {
            hExInfo = CreateWindowExW(0, L"STATIC", T(L"指定した場所の周り（半径）を、検索結果とお気に入り候補から除きます。",
                L"Excludes the area (radius) around each listed spot from search results."), WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hExUse = CreateWindowExW(0, L"BUTTON", T(L"除外リストを適用する", L"Apply the exclusion list"), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 10, 10, h, (HMENU)(INT_PTR)ID_EXUSE, hInst, NULL);
            SendMessage(hExUse, BM_SETCHECK, g_useExclude ? BST_CHECKED : BST_UNCHECKED, 0);
            hExLblX = CreateWindowExW(0, L"STATIC", L"X", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hExX = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 10, 10, h, (HMENU)(INT_PTR)ID_EXX, hInst, NULL);
            hExLblZ = CreateWindowExW(0, L"STATIC", L"Z", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hExZ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 10, 10, h, (HMENU)(INT_PTR)ID_EXZ, hInst, NULL);
            hExLblR = CreateWindowExW(0, L"STATIC", T(L"半径", L"Radius"), WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hExR = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"500", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 10, 10, h, (HMENU)(INT_PTR)ID_EXR, hInst, NULL);
            hExLblNote = CreateWindowExW(0, L"STATIC", T(L"メモ", L"Note"), WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hExNote = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 10, 10, h, (HMENU)(INT_PTR)ID_EXNOTE, hInst, NULL);
            hExAdd = CreateWindowExW(0, L"BUTTON", T(L"追加", L"Add"), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 10, 10, h, (HMENU)(INT_PTR)1001, hInst, NULL);
            hExList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                0, 0, 10, 10, h, NULL, hInst, NULL);
            ListView_SetExtendedListViewStyle(hExList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            exclAddCol(0, L"#", 32); exclAddCol(1, L"X", 90); exclAddCol(2, L"Z", 90); exclAddCol(3, T(L"半径", L"Radius"), 80); exclAddCol(4, T(L"メモ", L"Note"), 160);
            hExRemove = CreateWindowExW(0, L"BUTTON", T(L"選択を削除", L"Remove selected"), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 10, 10, h, (HMENU)(INT_PTR)1002, hInst, NULL);
            HWND kids[] = { hExInfo, hExUse, hExLblX, hExX, hExLblZ, hExZ, hExLblR, hExR, hExLblNote, hExNote, hExAdd, hExList, hExRemove };
            for (HWND k : kids) SendMessage(k, WM_SETFONT, (WPARAM)hFont, TRUE);
            exclFillList();
            return 0;
        }
    case WM_SIZE:
        {
            RECT rc; GetClientRect(h, &rc); int W = rc.right, Hh = rc.bottom, M = S(14), y = M;
            MoveWindow(hExInfo, M, y, W - 2 * M, S(34), TRUE); y += S(38);
            MoveWindow(hExUse, M, y, S(260), S(20), TRUE); y += S(28);
            int x = M;
            MoveWindow(hExLblX, x, y + S(4), S(14), S(20), TRUE); x += S(18);
            MoveWindow(hExX, x, y, S(100), S(24), TRUE); x += S(110);
            MoveWindow(hExLblZ, x, y + S(4), S(14), S(20), TRUE); x += S(18);
            MoveWindow(hExZ, x, y, S(100), S(24), TRUE); x += S(110);
            MoveWindow(hExLblR, x, y + S(4), S(40), S(20), TRUE); x += S(44);
            MoveWindow(hExR, x, y, S(90), S(24), TRUE); x += S(100);
            MoveWindow(hExLblNote, x, y + S(4), S(36), S(20), TRUE); x += S(40);
            MoveWindow(hExNote, x, y, W - M - x, S(24), TRUE); y += S(32);
            MoveWindow(hExAdd, M, y, S(100), S(30), TRUE); y += S(38);
            int by = Hh - M - S(32);
            MoveWindow(hExList, M, y, W - 2 * M, by - S(8) - y, TRUE);
            MoveWindow(hExRemove, M, by, S(150), S(32), TRUE);
            return 0;
        }
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case ID_EXUSE: g_useExclude = SendMessage(hExUse, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0; saveExclude(); return 0;
        case 1001: exclAdd(); return 0;
        case 1002: exclRemove(); return 0;
        }
        break;
    case WM_DESTROY: hExcl = NULL; return 0;
    }
    return DefWindowProc(h, m, w, l);
}
static void showExcludeList()
{
    if (hExcl && IsWindow(hExcl)) { SetForegroundWindow(hExcl); return; }
    RECT r = { 0, 0, S(560), S(440) };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    hExcl = CreateWindowExW(0, L"SlimeExclWnd", T(L"除外リスト", L"Exclusion List"), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, hMain, NULL, hInst, NULL);
}

// ---------------------------------------------------------------------
//  複数シード一括検索
// ---------------------------------------------------------------------
static int cmpBatch(const void* a, const void* b)
{
    const BatchHit *x = (const BatchHit*)a, *y = (const BatchHit*)b;
    if (P.dense || x->count != y->count) {
        // dense系、または個数が違う (惜しい場所) ときは個数が多い順
        int cx = P.dense ? x->count : (x->count >= P.K * P.K ? 1 : 0);
        int cy = P.dense ? y->count : (y->count >= P.K * P.K ? 1 : 0);
        if (cx != cy) return cy - cx;
        if (P.dense && x->count != y->count) return y->count - x->count;
    }
    double d = x->dist - y->dist; return d < 0 ? -1 : (d > 0 ? 1 : 0);
}
static void batchCenterOf(const BatchHit& h, long long* bx, long long* bz)
{
    *bx = (long long)h.cx * 16 + P.K * 8; *bz = (long long)h.cz * 16 + P.K * 8;
}
static DWORD WINAPI batchWorker(LPVOID textPtr)
{
    wchar_t* text = (wchar_t*)textPtr;
    wchar_t* ctx = NULL;
    wchar_t* line = wcstok_s(text, L"\r\n", &ctx);
    g_nbatch = 0; g_batchDoneSeeds = 0;
    while (line) {
        if (g_batchStop) break;
        while (*line == L' ' || *line == L'\t') line++;
        wchar_t* eol = line + wcslen(line); while (eol > line && (eol[-1] == L' ' || eol[-1] == L'\t')) *--eol = 0;
        if (*line) {
            I64 sd;
            if (parseSeed(line, &sd)) {
                SEED = sd;
                searchCore();
                int take = g_nres < 10 ? g_nres : 10;
                for (int i = 0; i < take; i++) {
                    if (g_nbatch >= g_batchCap) { g_batchCap = g_batchCap ? g_batchCap * 2 : 512; g_batch = (BatchHit*)realloc(g_batch, g_batchCap * sizeof(BatchHit)); }
                    BatchHit& h = g_batch[g_nbatch++];
                    wcsncpy(h.seed, line, 39); h.seed[39] = 0;
                    h.cx = g_res[i].cx; h.cz = g_res[i].cz; h.count = g_res[i].count; h.iso = g_res[i].iso; h.dist = g_res[i].dist;
                }
            }
        }
        InterlockedIncrement(&g_batchDoneSeeds);
        line = wcstok_s(NULL, L"\r\n", &ctx);
    }
    free(text);
    qsort(g_batch, g_nbatch, sizeof(BatchHit), cmpBatch);
    PostMessageW(hBatch, WM_BATCH_DONE, 0, 0);
    return 0;
}
static void batchFillList()
{
    g_batchShown = g_nbatch < 500 ? g_nbatch : 500;
    ListView_DeleteAllItems(hBatchList);
    SendMessage(hBatchList, WM_SETREDRAW, FALSE, 0);
    for (int i = 0; i < g_batchShown; i++) {
        wchar_t t[160]; long long bx, bz; batchCenterOf(g_batch[i], &bx, &bz);
        LVITEMW it; ZeroMemory(&it, sizeof it); it.mask = LVIF_TEXT; it.iItem = i;
        _snwprintf(t, 160, L"%d", i + 1); it.pszText = t; ListView_InsertItem(hBatchList, &it);
        ListView_SetItemText(hBatchList, i, 1, g_batch[i].seed);
        _snwprintf(t, 160, L"%lld", bx); ListView_SetItemText(hBatchList, i, 2, t);
        _snwprintf(t, 160, L"%lld", bz); ListView_SetItemText(hBatchList, i, 3, t);
        if (P.dense) _snwprintf(t, 160, T(L"%d×%d に %d個", L"%d\u00d7%d, %d chunks"), P.K, P.K, g_batch[i].count);
        else if (g_batch[i].count < P.K * P.K) _snwprintf(t, 160, T(L"惜しい %d/%d", L"Close %d/%d"), g_batch[i].count, P.K * P.K);
        else _snwprintf(t, 160, L"%ls", g_batch[i].iso ? T(L"外周もきれい", L"Clean border") : T(L"隣接あり", L"Adjacent"));
        ListView_SetItemText(hBatchList, i, 4, t);
        _snwprintf(t, 160, T(L"約 %.0f", L"~%.0f"), g_batch[i].dist); ListView_SetItemText(hBatchList, i, 5, t);
    }
    SendMessage(hBatchList, WM_SETREDRAW, TRUE, 0);
    if (g_batchShown > 0) ListView_SetItemState(hBatchList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}
static void batchStart()
{
    if (g_batchRunning || g_running) return;
    int len = GetWindowTextLengthW(hBatchSeeds);
    if (len <= 0) { MessageBoxW(hBatch, T(L"シード値を1行に1つずつ入力してください。", L"Please enter one seed per line."), T(L"入力エラー", L"Input error"), MB_OK | MB_ICONWARNING); return; }
    wchar_t* raw = (wchar_t*)malloc((len + 2) * sizeof(wchar_t));
    GetWindowTextW(hBatchSeeds, raw, len + 1);
    // 行数を数える (上限チェック用) - 検索本体には複製した文字列を渡す (wcstok_s が破壊するため)
    int lines = 0; for (wchar_t* p = raw; *p; p++) if (*p == L'\n' || (p == raw)) lines++;
    if (lines > 200) { MessageBoxW(hBatch, T(L"シードは200件までです。", L"Please enter no more than 200 seeds."), T(L"入力エラー", L"Input error"), MB_OK | MB_ICONWARNING); free(raw); return; }
    Params p; wchar_t err[160];
    if (!buildParamsFromUI(p, err, 160)) { MessageBoxW(hBatch, err, T(L"入力エラー", L"Input error"), MB_OK | MB_ICONWARNING); free(raw); return; }
    P = p;
    int vi = (int)SendMessage(hVer, CB_GETCURSEL, 0, 0); if (vi < 0) vi = 0;
    wcscpy(g_batchVerKey, verKeys[vi]); wcscpy(g_batchVerLabel, verNames[vi]);
    g_batchStop = 0; g_batchRunning = 1; g_batchTotalSeeds = lines < 1 ? 1 : lines; g_batchDoneSeeds = 0;
    EnableWindow(hBatchGo, FALSE); EnableWindow(hBatchStop, TRUE); EnableWindow(hBatchSeeds, FALSE);
    SendMessage(hBatchProg, PBM_SETRANGE32, 0, 1000); SendMessage(hBatchProg, PBM_SETPOS, 0, 0);
    SetWindowTextW(hBatchStatus, T(L"検索中…", L"Searching\u2026"));
    SetTimer(hBatch, 2, 150, NULL);
    HANDLE th = CreateThread(NULL, 0, batchWorker, raw, 0, NULL);
    if (th) CloseHandle(th);
}
static int batchSelIndex()
{
    int i = ListView_GetNextItem(hBatchList, -1, LVNI_SELECTED);
    if (i < 0 || i >= g_batchShown) { MessageBoxW(hBatch, T(L"一覧から場所を選んでください。", L"Please select a spot from the list."), T(L"選択してください", L"Please select"), MB_OK | MB_ICONINFORMATION); return -1; }
    return i;
}
static void batchAddCol(int i, const wchar_t* t, int w)
{
    LVCOLUMNW c; ZeroMemory(&c, sizeof c); c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; c.pszText = (LPWSTR)t; c.cx = S(w); c.iSubItem = i;
    ListView_InsertColumn(hBatchList, i, &c);
}
static LRESULT CALLBACK BatchWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_CREATE:
        {
            hBatchInfo = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hBatchSeeds = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
                0, 0, 10, 10, h, NULL, hInst, NULL);
            hBatchGo = CreateWindowExW(0, L"BUTTON", T(L"検索", L"Search"), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 10, 10, h, (HMENU)(INT_PTR)1, hInst, NULL);
            hBatchStop = CreateWindowExW(0, L"BUTTON", T(L"止める", L"Stop"), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 10, 10, h, (HMENU)(INT_PTR)2, hInst, NULL);
            EnableWindow(hBatchStop, FALSE);
            hBatchProg = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hBatchStatus = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, NULL, hInst, NULL);
            hBatchList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                0, 0, 10, 10, h, NULL, hInst, NULL);
            ListView_SetExtendedListViewStyle(hBatchList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            batchAddCol(0, L"#", 36); batchAddCol(1, T(L"シード", L"Seed"), 140); batchAddCol(2, T(L"中心 X", L"Center X"), 80);
            batchAddCol(3, T(L"中心 Z", L"Center Z"), 80); batchAddCol(4, T(L"内容", L"Result"), 140); batchAddCol(5, T(L"距離", L"Distance"), 90);
            hBatchCopyXZ = CreateWindowExW(0, L"BUTTON", T(L"座標をコピー", L"Copy coordinates"), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 10, 10, h, (HMENU)(INT_PTR)3, hInst, NULL);
            hBatchOpen = CreateWindowExW(0, L"BUTTON", T(L"Chunkbaseで開く", L"Open in Chunkbase"), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 10, 10, h, (HMENU)(INT_PTR)4, hInst, NULL);
            hBatchCopyAll = CreateWindowExW(0, L"BUTTON", T(L"一覧をコピー", L"Copy list"), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 10, 10, h, (HMENU)(INT_PTR)5, hInst, NULL);
            HWND kids[] = { hBatchInfo, hBatchSeeds, hBatchGo, hBatchStop, hBatchStatus, hBatchList, hBatchCopyXZ, hBatchOpen, hBatchCopyAll };
            for (HWND k : kids) SendMessage(k, WM_SETFONT, (WPARAM)hFont, TRUE);
            wchar_t info[300];
            _snwprintf(info, 300, T(L"現在の検索条件（中心・半径・パターン）をメイン画面から使います。シードを1行に1つずつ入力してください（最大200件）。",
                L"Uses the current center, radius and pattern from the main window. Enter one seed per line (up to 200)."));
            info[299] = 0; SetWindowTextW(hBatchInfo, info);
            SetWindowTextW(hBatchStatus, T(L"シードを入力して「検索」を押してください。", L"Enter seeds and press Search."));
            return 0;
        }
    case WM_SIZE:
        {
            RECT rc; GetClientRect(h, &rc); int W = rc.right, Hh = rc.bottom, M = S(14), y = M;
            MoveWindow(hBatchInfo, M, y, W - 2 * M, S(34), TRUE); y += S(38);
            MoveWindow(hBatchSeeds, M, y, W - 2 * M, S(90), TRUE); y += S(96);
            MoveWindow(hBatchGo, M, y, S(100), S(32), TRUE);
            MoveWindow(hBatchStop, M + S(108), y, S(100), S(32), TRUE); y += S(40);
            MoveWindow(hBatchProg, M, y, W - 2 * M, S(12), TRUE); y += S(18);
            MoveWindow(hBatchStatus, M, y, W - 2 * M, S(20), TRUE); y += S(28);
            int by = Hh - M - S(32);
            MoveWindow(hBatchList, M, y, W - 2 * M, by - S(8) - y, TRUE);
            MoveWindow(hBatchCopyXZ, M, by, S(140), S(32), TRUE);
            MoveWindow(hBatchOpen, M + S(148), by, S(160), S(32), TRUE);
            MoveWindow(hBatchCopyAll, M + S(316), by, S(120), S(32), TRUE);
            return 0;
        }
    case WM_TIMER:
        {
            LONG d = g_batchDoneSeeds, t = g_batchTotalSeeds; if (t < 1) t = 1;
            double pr = (double)d / t; if (pr > 1) pr = 1;
            SendMessage(hBatchProg, PBM_SETPOS, (WPARAM)(pr * 1000), 0);
            wchar_t b[160]; _snwprintf(b, 160, T(L"検索中… シード %ld / %ld　見つかった数 %d", L"Searching\u2026 seed %ld / %ld\u3000found %d"),
                (long)d, (long)t, g_nbatch); b[159] = 0;
            SetWindowTextW(hBatchStatus, b);
            return 0;
        }
    case WM_BATCH_DONE:
        {
            KillTimer(h, 2);
            g_batchRunning = 0;
            EnableWindow(hBatchGo, TRUE); EnableWindow(hBatchStop, FALSE); EnableWindow(hBatchSeeds, TRUE);
            SendMessage(hBatchProg, PBM_SETPOS, 1000, 0);
            batchFillList();
            wchar_t b[200]; _snwprintf(b, 200, T(L"%ls　見つかった数 %d 件（%d シード分を検索）", L"%ls\u3000found %d spot(s) (searched %d seed(s))"),
                g_batchStop ? T(L"停止しました", L"Stopped") : T(L"完了しました", L"Done"), g_nbatch, (int)g_batchDoneSeeds);
            b[199] = 0; SetWindowTextW(hBatchStatus, b);
            return 0;
        }
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case 1: batchStart(); return 0;
        case 2: g_batchStop = 1; return 0;
        case 3: { int i = batchSelIndex(); if (i < 0) return 0; long long bx, bz; batchCenterOf(g_batch[i], &bx, &bz);
            wchar_t b[64]; _snwprintf(b, 64, L"%lld %lld", bx, bz); copyText(b); SetWindowTextW(hBatchStatus, T(L"座標をコピーしました。", L"Coordinates copied.")); return 0; }
        case 4: { int i = batchSelIndex(); if (i < 0) return 0; long long bx, bz; batchCenterOf(g_batch[i], &bx, &bz);
            wchar_t u[400]; _snwprintf(u, 400, L"https://www.chunkbase.com/apps/slime-finder#seed=%ls&platform=java_%ls&x=%lld&z=%lld&zoom=0.75",
                g_batch[i].seed, g_batchVerKey, bx, bz);
            ShellExecuteW(h, L"open", u, NULL, NULL, SW_SHOWNORMAL); return 0; }
        case 5: {
            if (g_batchShown == 0) { MessageBoxW(h, T(L"コピーする結果がありません。", L"No results to copy."), T(L"確認", L"Confirm"), MB_OK | MB_ICONINFORMATION); return 0; }
            WBuf b = { 0, 0, 0 };
            wappend(b, T(L"複数シード一括検索の結果（%ls）\r\n\r\n", L"Multi-seed batch search results (%ls)\r\n\r\n"), g_batchVerLabel);
            for (int i = 0; i < g_batchShown; i++) {
                long long bx, bz; batchCenterOf(g_batch[i], &bx, &bz);
                wappend(b, L"%d. %ls  X %lld, Z %lld\r\n", i + 1, g_batch[i].seed, bx, bz);
            }
            copyText(b.p); free(b.p); SetWindowTextW(hBatchStatus, T(L"一覧をコピーしました。", L"List copied.")); return 0;
        }
        }
        break;
    case WM_CLOSE:
        if (g_batchRunning) { g_batchStop = 1; return 0; }   // 実行中は止めるだけにして、終わってから閉じ直してもらう
        DestroyWindow(h); return 0;
    case WM_DESTROY: hBatch = NULL; return 0;
    }
    return DefWindowProc(h, m, w, l);
}
static void showBatchSearch()
{
    if (hBatch && IsWindow(hBatch)) { SetForegroundWindow(hBatch); return; }
    RECT r = { 0, 0, S(560), S(520) };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    hBatch = CreateWindowExW(0, L"SlimeBatchWnd", T(L"複数シード一括検索", L"Multi-Seed Batch Search"), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, hMain, NULL, hInst, NULL);
}

// ---------------------------------------------------------------------
//  ウィンドウ クラスの登録と起動
// ---------------------------------------------------------------------
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SLIMECHUNK));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName   = nullptr;                    // メニューはコードで作成します
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    ATOM a = RegisterClassExW(&wcex);

    WNDCLASSEXW tc = wcex;
    tc.lpfnWndProc = TextWndProc;
    tc.lpszClassName = L"SlimeTextWnd";
    tc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&tc);

    WNDCLASSEXW pc = wcex;
    pc.lpfnWndProc = PreviewWndProc;
    pc.lpszClassName = L"SlimePreviewWnd";
    pc.hbrBackground = NULL;
    RegisterClassExW(&pc);

    WNDCLASSEXW cc = wcex;
    cc.lpfnWndProc = ConvWndProc;
    cc.lpszClassName = L"SlimeConvWnd";
    cc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassExW(&cc);

    WNDCLASSEXW bc = wcex;
    bc.lpfnWndProc = BatchWndProc;
    bc.lpszClassName = L"SlimeBatchWnd";
    bc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassExW(&bc);

    WNDCLASSEXW ec = wcex;
    ec.lpfnWndProc = ExclWndProc;
    ec.lpszClassName = L"SlimeExclWnd";
    ec.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassExW(&ec);
    return a;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    HDC dc = GetDC(NULL); int d = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(NULL, dc);
    RECT r = { 0, 0, MulDiv(940, d, 96), MulDiv(940, d, 96) };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, TRUE);
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, 0, r.right - r.left, r.bottom - r.top, nullptr, buildMenu(), hInstance, nullptr);
    if (!hWnd) return FALSE;
    // 前回終了時のウィンドウ位置・大きさを、ここ (ウィンドウが完全に作られたあと) で安全に復元する。
    // WM_CREATE の途中で SetWindowPlacement を呼ぶと、環境によってはウィンドウが
    // 不正な状態のまま非表示になってしまうことがあるため、必ず作成完了後に行う。
    if (g_hasWinRect) SetWindowPos(hWnd, NULL, g_winRect.left, g_winRect.top,
        g_winRect.right - g_winRect.left, g_winRect.bottom - g_winRect.top, SWP_NOZORDER | SWP_NOACTIVATE);
    ShowWindow(hWnd, g_startMax ? SW_SHOWMAXIMIZED : nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    SetProcessDPIAware();
    INITCOMMONCONTROLSEX ic = { sizeof ic, ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES };
    InitCommonControlsEx(&ic);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SLIMECHUNK, szWindowClass, MAX_LOADSTRING);
    if (!szWindowClass[0]) wcscpy(szWindowClass, L"SLIMECHUNK");
    wcscpy(szTitle, L"スライムチャンク ファインダー");
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow)) return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SLIMECHUNK));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            // ショートカットキー: F5=探す / Esc=止める / F1=使い方 / Ctrl+S=結果を保存
            if (msg.message == WM_KEYDOWN && hMain && GetAncestor(msg.hwnd, GA_ROOT) == hMain) {
                bool ctrl = GetKeyState(VK_CONTROL) < 0;
                if (msg.wParam == VK_F5) { PostMessageW(hMain, WM_COMMAND, ID_GO, 0); continue; }
                if (msg.wParam == VK_ESCAPE) { PostMessageW(hMain, WM_COMMAND, ID_STOP, 0); continue; }
                if (msg.wParam == VK_F1) { PostMessageW(hMain, WM_COMMAND, IDM_HELP_USAGE, 0); continue; }
                if (ctrl && msg.wParam == 'S') { PostMessageW(hMain, WM_COMMAND, IDM_FILE_SAVENOW, 0); continue; }
            }
            // 入力欄で Enter を押したら「探す」を実行
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN && GetParent(msg.hwnd) == hMain) {
                wchar_t cn[16]; GetClassNameW(msg.hwnd, cn, 16);
                if (_wcsicmp(cn, L"Edit") == 0) { PostMessageW(hMain, WM_COMMAND, ID_GO, 0); continue; }
            }
            if (!IsDialogMessage(hMain, &msg)) { TranslateMessage(&msg); DispatchMessage(&msg); }
        }
    }
    return (int)msg.wParam;
}
#endif // SLIME_TEST
