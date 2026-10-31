#define UNICODE
#define _UNICODE
#define _WIN32_IE 0x0600
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <omp.h>

typedef int64_t L; typedef int32_t I;
static L SEED = 0;

static inline int slime(I x, I z) {
  L s = SEED + (L)(I)((uint32_t)x*(uint32_t)x*0x4c1906u) + (L)(I)((uint32_t)x*0x5ac0dbu)
      + (L)(I)((uint32_t)z*(uint32_t)z)*0x4307a7LL + (L)(I)((uint32_t)z*0x5f24fu);
  s ^= 0x3ad8025fLL;
  uint64_t r = ((uint64_t)s ^ 0x5DEECE66DULL) & ((1ULL<<48)-1);
  for (;;) {
    r = (r*0x5DEECE66DULL + 0xBULL) & ((1ULL<<48)-1);
    I bits = (I)(r >> 17);
    I val = bits % 10;
    if ((I)((uint32_t)bits - (uint32_t)val + 9u) >= 0) return val == 0;
  }
}
static int isolated(I cx, I cz) {
  for (int a=-1;a<=3;a++) for (int b=-1;b<=3;b++) {
    if (a>=0&&a<3&&b>=0&&b<3) continue;
    if (slime(cx+a, cz+b)) return 0;
  }
  return 1;
}

typedef struct { I cx, cz; int iso; double dist; } Res;
static Res *res = NULL; static size_t nres = 0, capres = 0;
static int cmpres(const void*a, const void*b){ double d=((Res*)a)->dist-((Res*)b)->dist; return d<0?-1:d>0; }

/* ---- UI ids ---- */
enum { ID_SEED=101, ID_VER, ID_X, ID_Z, ID_R, ID_ISO, ID_GO, ID_STOP, ID_PROG, ID_STATUS, ID_LIST,
       ID_COPYXZ, ID_COPYTP, ID_OPEN, ID_COPYALL, ID_NOTE, ID_LBL0 = 200 };
#define TIMER_ID 1
#define WM_DONE (WM_APP + 1)

static HWND hMain, hSeed, hVer, hX, hZ, hR, hIso, hGo, hStop, hProg, hStatus, hList, hCopyXZ, hCopyTP, hOpen, hCopyAll;
static HWND hLbl[6], hNote;
static HFONT hFont;
static int dpi = 96;
#define S(v) MulDiv((v), dpi, 96)

static const wchar_t *verNames[] = { L"Java 26.3", L"Java 26.2", L"Java 26.1", L"Java 1.21.9 - 1.21.11", L"Java 1.21.6 - 1.21.8", L"Java 1.21.5", L"Java 1.21.4", L"Java 1.21.2 - 1.21.3", L"Java 1.21 - 1.21.1", L"Java 1.20", L"Java 1.19.3 - 1.19.4", L"Java 1.19 - 1.19.2", L"Java 1.18", L"Java 1.17", L"Java 1.16", L"Java 1.15", L"Java 1.14", L"Java 1.13", L"Java 1.12", L"Java 1.11", L"Java 1.10", L"Java 1.9", L"Java 1.8", L"Java 1.7（リンクは1.8扱い）" };
static const wchar_t *verKeys[]  = { L"26_3", L"26_2", L"26_1", L"1_21_9", L"1_21_6", L"1_21_5", L"1_21_4", L"1_21_2", L"1_21", L"1_20", L"1_19_3", L"1_19", L"1_18", L"1_17", L"1_16", L"1_15", L"1_14", L"1_13", L"1_12", L"1_11", L"1_10", L"1_9", L"1_8", L"1_8" };
#define NVER 24

/* ---- search state ---- */
static volatile LONG g_done = 0, g_total = 1, g_stop = 0, g_running = 0;
static long long g_X, g_Z, g_R; static int g_onlyiso;
static wchar_t g_seedStr[64], g_verKey[16], g_verLabel[64];
static DWORD g_t0;
static int g_shown = 0;
#define MAX_SHOW 500

static DWORD WINAPI worker(LPVOID unused) {
  I ccx = (I)floor((double)g_X/16.0), ccz = (I)floor((double)g_Z/16.0);
  I rc = (I)((g_R + 15) / 16);
  I x0 = ccx - rc, z0 = ccz - rc;
  long long W = 2LL*rc + 1, H = 2LL*rc + 1;
  const int BAND = 256;
  long long nbands = (H - 2 + BAND - 1) / BAND;
  InterlockedExchange(&g_total, (LONG)nbands);
  InterlockedExchange(&g_done, 0);
  #pragma omp parallel
  {
    unsigned char *g = (unsigned char*)malloc((size_t)W * (BAND + 2));
    #pragma omp for schedule(dynamic,1)
    for (long long bi = 0; bi < nbands; bi++) {
      if (g_stop) continue;
      long long r0 = bi * BAND;
      long long rows = BAND; if (r0 + rows > H - 2) rows = H - 2 - r0;
      for (long long j = 0; j < rows + 2; j++)
        for (long long i = 0; i < W; i++) g[j*W + i] = (unsigned char)slime((I)(x0 + i), (I)(z0 + r0 + j));
      for (long long j = 0; j < rows; j++)
        for (long long i = 0; i + 2 < W; i++) {
          int ok = 1;
          for (int a = 0; a < 3 && ok; a++) for (int b = 0; b < 3; b++)
            if (!g[(j+b)*W + i + a]) { ok = 0; break; }
          if (ok) {
            I cx = (I)(x0 + i), cz = (I)(z0 + r0 + j);
            int iso = isolated(cx, cz);
            if (g_onlyiso && !iso) continue;
            #pragma omp critical(add)
            {
              if (nres == capres) { capres = capres ? capres*2 : 64; res = (Res*)realloc(res, capres*sizeof(Res)); }
              double bx = cx*16.0+24, bz = cz*16.0+24;
              res[nres].cx = cx; res[nres].cz = cz; res[nres].iso = iso; res[nres].dist = hypot(bx - g_X, bz - g_Z);
              nres++;
            }
          }
        }
      InterlockedIncrement(&g_done);
    }
    free(g);
  }
  qsort(res, nres, sizeof(Res), cmpres);
  PostMessage(hMain, WM_DONE, 0, 0);
  return 0;
}

static void copyText(const wchar_t *t) {
  size_t n = wcslen(t) + 1;
  HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, n * sizeof(wchar_t));
  if (!h) return;
  memcpy(GlobalLock(h), t, n * sizeof(wchar_t)); GlobalUnlock(h);
  if (OpenClipboard(hMain)) { EmptyClipboard(); SetClipboardData(CF_UNICODETEXT, h); CloseClipboard(); }
  else GlobalFree(h);
}

static void setStatus(const wchar_t *t) { SetWindowText(hStatus, t); }

static int parseSeed(const wchar_t *s, L *out) {
  while (*s == L' ') s++;
  size_t n = wcslen(s);
  while (n > 0 && s[n-1] == L' ') n--;
  if (n == 0) return 0;
  wchar_t buf[64]; if (n >= 64) n = 63; wcsncpy(buf, s, n); buf[n] = 0;
  int numeric = 1; size_t i = 0; if (buf[0] == L'-') i = 1;
  if (i >= n) numeric = 0;
  for (size_t k = i; k < n; k++) if (buf[k] < L'0' || buf[k] > L'9') { numeric = 0; break; }
  if (numeric) {
    errno = 0; wchar_t *e; long long v = _wcstoi64(buf, &e, 10);
    if (errno != ERANGE) { *out = v; return 1; }
  }
  int32_t h = 0;
  for (size_t k = 0; k < n; k++) h = (int32_t)((uint32_t)h * 31u + (uint32_t)buf[k]);
  *out = h; return 1;
}
static long long getNum(HWND h, long long def) {
  wchar_t b[64]; GetWindowText(h, b, 64);
  if (!b[0]) return def;
  return _wtoi64(b);
}

static void setRunning(int r) {
  g_running = r;
  EnableWindow(hGo, !r); EnableWindow(hStop, r);
  EnableWindow(hSeed, !r); EnableWindow(hVer, !r); EnableWindow(hX, !r); EnableWindow(hZ, !r);
  EnableWindow(hR, !r); EnableWindow(hIso, !r);
}

static void startSearch(void) {
  wchar_t sb[64]; GetWindowText(hSeed, sb, 64);
  L seed;
  if (!parseSeed(sb, &seed)) { MessageBox(hMain, L"シード値を入力してください。", L"入力エラー", MB_OK | MB_ICONWARNING); return; }
  SEED = seed;
  _snwprintf(g_seedStr, 64, L"%I64d", (long long)seed);
  int vi = (int)SendMessage(hVer, CB_GETCURSEL, 0, 0); if (vi < 0) vi = 0;
  wcscpy(g_verKey, verKeys[vi]); wcscpy(g_verLabel, verNames[vi]);
  g_X = getNum(hX, 0); g_Z = getNum(hZ, 0); g_R = getNum(hR, 100000);
  if (g_R < 64) g_R = 64; if (g_R > 1000000) g_R = 1000000;
  g_onlyiso = (SendMessage(hIso, BM_GETCHECK, 0, 0) == BST_CHECKED);
  nres = 0; g_stop = 0; g_shown = 0;
  ListView_DeleteAllItems(hList);
  setRunning(1);
  g_t0 = GetTickCount();
  SendMessage(hProg, PBM_SETPOS, 0, 0);
  setStatus(L"探索を開始しました…");
  SetTimer(hMain, TIMER_ID, 150, NULL);
  HANDLE th = CreateThread(NULL, 0, worker, NULL, 0, NULL);
  if (th) CloseHandle(th);
}

static void fillList(void) {
  g_shown = (int)(nres < MAX_SHOW ? nres : MAX_SHOW);
  SendMessage(hList, WM_SETREDRAW, FALSE, 0);
  for (int i = 0; i < g_shown; i++) {
    wchar_t t[128];
    long long bx = (long long)res[i].cx*16+24, bz = (long long)res[i].cz*16+24;
    LVITEM it; ZeroMemory(&it, sizeof it);
    it.mask = LVIF_TEXT; it.iItem = i;
    _snwprintf(t, 128, L"%d", i+1); it.pszText = t;
    ListView_InsertItem(hList, &it);
    _snwprintf(t, 128, L"%I64d", bx); ListView_SetItemText(hList, i, 1, t);
    _snwprintf(t, 128, L"%I64d", bz); ListView_SetItemText(hList, i, 2, t);
    ListView_SetItemText(hList, i, 3, res[i].iso ? L"外周もきれい" : L"隣にスライムチャンクあり");
    _snwprintf(t, 128, L"約 %.0f", res[i].dist); ListView_SetItemText(hList, i, 4, t);
    _snwprintf(t, 128, L"X %d〜%d / Z %d〜%d", res[i].cx, res[i].cx+2, res[i].cz, res[i].cz+2);
    ListView_SetItemText(hList, i, 5, t);
  }
  SendMessage(hList, WM_SETREDRAW, TRUE, 0);
  if (g_shown > 0) ListView_SetItemState(hList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

static int selIndex(void) {
  int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
  if (i < 0 || i >= g_shown) { MessageBox(hMain, L"一覧から場所を選んでください。", L"選択してください", MB_OK | MB_ICONINFORMATION); return -1; }
  return i;
}

static void layout(void) {
  RECT rc; GetClientRect(hMain, &rc);
  int W = rc.right, H = rc.bottom, M = S(14);
  int y = M, lh = S(22), eh = S(26);
  MoveWindow(hLbl[0], M, y + S(3), S(72), lh, TRUE);
  MoveWindow(hSeed, M + S(76), y, S(300), eh, TRUE);
  MoveWindow(hLbl[1], M + S(76 + 300 + 20), y + S(3), S(100), lh, TRUE);
  MoveWindow(hVer, M + S(76 + 300 + 20 + 104), y, S(190), S(320), TRUE);
  y += S(40);
  int x = M;
  MoveWindow(hLbl[2], x, y + S(3), S(50), lh, TRUE); x += S(54);
  MoveWindow(hX, x, y, S(110), eh, TRUE); x += S(110 + 16);
  MoveWindow(hLbl[3], x, y + S(3), S(50), lh, TRUE); x += S(54);
  MoveWindow(hZ, x, y, S(110), eh, TRUE); x += S(110 + 16);
  MoveWindow(hLbl[4], x, y + S(3), S(120), lh, TRUE); x += S(124);
  MoveWindow(hR, x, y, S(110), eh, TRUE);
  y += S(40);
  MoveWindow(hIso, M, y + S(3), S(440), lh, TRUE);
  MoveWindow(hGo, M + S(450), y - S(2), S(100), S(32), TRUE);
  MoveWindow(hStop, M + S(560), y - S(2), S(90), S(32), TRUE);
  y += S(44);
  MoveWindow(hProg, M, y, W - 2*M, S(16), TRUE); y += S(22);
  MoveWindow(hStatus, M, y, W - 2*M, lh, TRUE); y += S(28);
  MoveWindow(hNote, M, y, W - 2*M, S(40), TRUE); y += S(44);
  int by = H - M - S(32);
  MoveWindow(hList, M, y, W - 2*M, by - S(10) - y, TRUE);
  int bx = M, bw = S(150);
  MoveWindow(hCopyXZ, bx, by, bw, S(32), TRUE); bx += bw + S(8);
  MoveWindow(hCopyTP, bx, by, S(190), S(32), TRUE); bx += S(190) + S(8);
  MoveWindow(hOpen, bx, by, S(170), S(32), TRUE); bx += S(170) + S(8);
  MoveWindow(hCopyAll, bx, by, S(130), S(32), TRUE);
}

static HWND mk(const wchar_t *cls, const wchar_t *txt, DWORD style, DWORD ex, int id) {
  HWND h = CreateWindowEx(ex, cls, txt, WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10, hMain, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL);
  return h;
}
static void addCol(int i, const wchar_t *t, int w) {
  LVCOLUMN c; ZeroMemory(&c, sizeof c);
  c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; c.pszText = (LPWSTR)t; c.cx = S(w); c.iSubItem = i;
  ListView_InsertColumn(hList, i, &c);
}

static BOOL CALLBACK setFont(HWND h, LPARAM f) { SendMessage(h, WM_SETFONT, (WPARAM)f, TRUE); return TRUE; }

static void onCreate(void) {
  HDC dc = GetDC(NULL); dpi = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(NULL, dc);
  NONCLIENTMETRICS ncm; ZeroMemory(&ncm, sizeof ncm); ncm.cbSize = sizeof ncm;
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof ncm, &ncm, 0);
  hFont = CreateFontIndirect(&ncm.lfMessageFont);

  hLbl[0] = mk(L"STATIC", L"シード値", 0, 0, ID_LBL0);
  hSeed = mk(L"EDIT", L"70392165441118850", WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_SEED);
  hLbl[1] = mk(L"STATIC", L"Javaバージョン", 0, 0, ID_LBL0 + 1);
  hVer = mk(L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 0, ID_VER);
  for (int i = 0; i < NVER; i++) SendMessage(hVer, CB_ADDSTRING, 0, (LPARAM)verNames[i]);
  SendMessage(hVer, CB_SETCURSEL, 0, 0);
  hLbl[2] = mk(L"STATIC", L"中心 X", 0, 0, ID_LBL0 + 2);
  hX = mk(L"EDIT", L"0", WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_X);
  hLbl[3] = mk(L"STATIC", L"中心 Z", 0, 0, ID_LBL0 + 3);
  hZ = mk(L"EDIT", L"0", WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_Z);
  hLbl[4] = mk(L"STATIC", L"半径（ブロック）", 0, 0, ID_LBL0 + 4);
  hR = mk(L"EDIT", L"100000", WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_R);
  hIso = mk(L"BUTTON", L"外周1チャンクにスライムチャンクがないものだけ表示", WS_TABSTOP | BS_AUTOCHECKBOX, 0, ID_ISO);
  hGo = mk(L"BUTTON", L"探す", WS_TABSTOP | BS_DEFPUSHBUTTON, 0, ID_GO);
  hStop = mk(L"BUTTON", L"止める", WS_TABSTOP, 0, ID_STOP);
  EnableWindow(hStop, FALSE);
  hProg = mk(PROGRESS_CLASS, L"", 0, 0, ID_PROG);
  SendMessage(hProg, PBM_SETRANGE32, 0, 1000);
  hStatus = mk(L"STATIC", L"シード値を入れて「探す」を押してください。", 0, 0, ID_STATUS);
  hNote = mk(L"STATIC", L"※バージョンの指定はChunkbaseの表記に合わせています。実際にChunkbaseを開いて、シードとバージョンが正しく反映されているか確認してください。", 0, 0, ID_NOTE);
  hList = mk(WC_LISTVIEW, L"", WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, ID_LIST);
  ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
  addCol(0, L"#", 44); addCol(1, L"中心 X", 90); addCol(2, L"中心 Z", 90); addCol(3, L"外周", 150);
  addCol(4, L"中心からの距離", 120); addCol(5, L"チャンク範囲", 260);
  hCopyXZ = mk(L"BUTTON", L"座標をコピー", WS_TABSTOP, 0, ID_COPYXZ);
  hCopyTP = mk(L"BUTTON", L"/tp コマンドをコピー", WS_TABSTOP, 0, ID_COPYTP);
  hOpen = mk(L"BUTTON", L"Chunkbaseで開く", WS_TABSTOP, 0, ID_OPEN);
  hCopyAll = mk(L"BUTTON", L"一覧をコピー", WS_TABSTOP, 0, ID_COPYALL);
  EnumChildWindows(hMain, setFont, (LPARAM)hFont);
  layout();
}

static LRESULT CALLBACK wndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  switch (m) {
  case WM_CREATE: hMain = h; onCreate(); return 0;
  case WM_SIZE: if (hList) layout(); return 0;
  case WM_GETMINMAXINFO: { MINMAXINFO *mi = (MINMAXINFO*)l; mi->ptMinTrackSize.x = S(720); mi->ptMinTrackSize.y = S(520); return 0; }
  case WM_TIMER: {
    LONG d = g_done, t = g_total; if (t < 1) t = 1;
    double p = (double)d / t;
    SendMessage(hProg, PBM_SETPOS, (WPARAM)(p * 1000), 0);
    wchar_t b[200];
    _snwprintf(b, 200, L"探索中… %.1f%%　見つかった数：%d　経過 %.0f秒", p*100, (int)nres, (GetTickCount() - g_t0) / 1000.0);
    setStatus(b);
    return 0; }
  case WM_DONE: {
    KillTimer(h, TIMER_ID);
    SendMessage(hProg, PBM_SETPOS, g_stop ? (WPARAM)(1000.0 * g_done / (g_total ? g_total : 1)) : 1000, 0);
    fillList();
    wchar_t b[200];
    _snwprintf(b, 200, L"%s　見つかった数：%d か所（近い順%s）　所要時間 %.1f秒",
      g_stop ? L"停止しました" : L"完了しました", (int)nres, nres > MAX_SHOW ? L"に500か所を表示" : L"",
      (GetTickCount() - g_t0) / 1000.0);
    setStatus(b);
    setRunning(0);
    if (nres == 0 && !g_stop) MessageBox(h, L"この範囲には見つかりませんでした。半径を広げてみてください。", L"結果", MB_OK | MB_ICONINFORMATION);
    return 0; }
  case WM_COMMAND:
    switch (LOWORD(w)) {
    case ID_GO: if (!g_running) startSearch(); return 0;
    case ID_STOP: g_stop = 1; return 0;
    case ID_COPYXZ: { int i = selIndex(); if (i < 0) return 0; wchar_t b[64];
      _snwprintf(b, 64, L"%I64d %I64d", (long long)res[i].cx*16+24, (long long)res[i].cz*16+24); copyText(b); setStatus(L"座標をコピーしました。"); return 0; }
    case ID_COPYTP: { int i = selIndex(); if (i < 0) return 0; wchar_t b[96];
      _snwprintf(b, 96, L"/tp @s %I64d ~ %I64d", (long long)res[i].cx*16+24, (long long)res[i].cz*16+24); copyText(b); setStatus(L"/tp コマンドをコピーしました。"); return 0; }
    case ID_OPEN: { int i = selIndex(); if (i < 0) return 0; wchar_t u[400];
      _snwprintf(u, 400, L"https://www.chunkbase.com/apps/slime-finder#seed=%s&platform=java_%s&x=%I64d&z=%I64d&zoom=0.75",
        g_seedStr, g_verKey, (long long)res[i].cx*16+24, (long long)res[i].cz*16+24);
      ShellExecute(h, L"open", u, NULL, NULL, SW_SHOWNORMAL); return 0; }
    case ID_COPYALL: {
      if (g_shown == 0) { MessageBox(h, L"コピーする結果がありません。", L"確認", MB_OK | MB_ICONINFORMATION); return 0; }
      size_t cap = 200 + (size_t)g_shown * 160;
      wchar_t *buf = (wchar_t*)malloc(cap * sizeof(wchar_t)); size_t pos = 0;
      pos += _snwprintf(buf + pos, cap - pos, L"シード: %s（%s）\r\n", g_seedStr, g_verLabel);
      pos += _snwprintf(buf + pos, cap - pos, L"探索: 中心(%I64d, %I64d) 半径%I64dブロック\r\n\r\n", g_X, g_Z, g_R);
      for (int i = 0; i < g_shown; i++)
        pos += _snwprintf(buf + pos, cap - pos, L"%d. X %I64d, Z %I64d（%s、距離約%.0f）\r\n", i+1,
          (long long)res[i].cx*16+24, (long long)res[i].cz*16+24, res[i].iso ? L"外周もきれい" : L"隣接あり", res[i].dist);
      copyText(buf); free(buf); setStatus(L"一覧をコピーしました。"); return 0; }
    }
    break;
  case WM_NOTIFY: {
    NMHDR *nh = (NMHDR*)l;
    if (nh->hwndFrom == hList && nh->code == NM_DBLCLK) { SendMessage(h, WM_COMMAND, ID_COPYXZ, 0); return 0; }
    break; }
  case WM_DESTROY: g_stop = 1; PostQuitMessage(0); return 0;
  }
  return DefWindowProc(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show) {
  INITCOMMONCONTROLSEX ic = { sizeof ic, ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES };
  InitCommonControlsEx(&ic);
  WNDCLASSEX wc; ZeroMemory(&wc, sizeof wc);
  wc.cbSize = sizeof wc; wc.lpfnWndProc = wndProc; wc.hInstance = hi;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = L"SlimeFinderWnd";
  wc.hIcon = LoadIcon(hi, MAKEINTRESOURCE(2)); wc.hIconSm = wc.hIcon;
  RegisterClassEx(&wc);
  HDC dc = GetDC(NULL); int d = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(NULL, dc);
  RECT r = { 0, 0, MulDiv(800, d, 96), MulDiv(660, d, 96) };
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
  HWND w = CreateWindowEx(0, L"SlimeFinderWnd", L"スライムチャンク 3×3 ファインダー", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
    CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, hi, NULL);
  if (!w) return 1;
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0) > 0) {
    if (!IsDialogMessage(w, &msg)) { TranslateMessage(&msg); DispatchMessage(&msg); }
  }
  return 0;
}
