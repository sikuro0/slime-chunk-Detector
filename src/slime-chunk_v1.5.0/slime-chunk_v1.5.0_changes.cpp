// =====================================================================
//  slime-chunk.cpp  v1.4.1 → v1.5.0  変更箇所だけをまとめたファイル
// =====================================================================
//  ・このファイルは「コンパイルするファイル」ではなく、slime-chunk.cpp の中で
//    変える場所を順番に並べたものです（プロジェクトには追加しないでください）。
//  ・各「変更 N」で、// 【探す】の行を slime-chunk.cpp から検索し、
//    // 【変更後】のコードに置き換える（または続けて追加する）だけです。
//  ・変更は全部で 24 か所。上から順に進めると、そのままビルドが通ります。
// =====================================================================


// ---------------------------------------------------------------------
// 変更 1
// 【探す】
//     #define APP_VERSION L"1.4.1"
// 【変更後】
#define APP_VERSION L"1.5.0"


// ---------------------------------------------------------------------
// 変更 2
// 【探す】
//     #include <string.h>
// 【変更後】
#include <string.h>
#include <wctype.h>


// ---------------------------------------------------------------------
// 変更 3
// 【探す】
//     #define TIMER_ID 1
// 【変更後】
// v1.5.0: メイン画面のサイズ選択ボタン / 半径プリセットボタン
enum { ID_SEG0 = 5301, ID_RP0 = 5311 };
#define TIMER_ID 1


// ---------------------------------------------------------------------
// 変更 4
// 【探す】
//     static HWND hNearest;
// 【変更後】
static HWND hNearest;
static HWND hSeg[5], hRp[4], hLblSeg, hLblRad, hTip;      // v1.5.0
static HWND g_hot = NULL;                                  // マウスが乗っているボタン
static int g_sortCol = -1; static bool g_sortAsc = true;   // 結果一覧の並べ替え状態
static const long long RP_VAL[4] = { 10000, 50000, 100000, 500000 };


// ---------------------------------------------------------------------
// 変更 5
// 【探す】
//     static HMENU hMenu;
//     static int dpi = 96;
// 【変更後】
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


// ---------------------------------------------------------------------
// 変更 6
// 【探す】
//         EnableWindow(hNearest, !r);
//         EnableWindow(hHist, !r);
// 【変更後】
    EnableWindow(hNearest, !r);
    for (int i = 0; i < 5; i++) if (hSeg[i]) EnableWindow(hSeg[i], !r);
    for (int i = 0; i < 4; i++) if (hRp[i]) EnableWindow(hRp[i], !r);
    EnableWindow(hHist, !r);


// ---------------------------------------------------------------------
// 変更 7
// 【探す】
//         EnableWindow(hDK, !g_running && m == 4); EnableWindow(hDM, !g_running && m == 4);
//     }
// 【変更後】
    EnableWindow(hDK, !g_running && m == 4); EnableWindow(hDM, !g_running && m == 4);
    for (int i = 0; i < 5; i++) if (hSeg[i]) InvalidateRect(hSeg[i], NULL, FALSE);
}


// ---------------------------------------------------------------------
// 変更 8
// 【探す】
//     g_exactHits = 0; g_done = 0; g_nbands = 1;
//         ListView_DeleteAllItems(hList);
//         setRunning(1);
// 【変更後】
g_exactHits = 0; g_done = 0; g_nbands = 1;
    ListView_DeleteAllItems(hList); g_sortCol = -1;
    setRunning(1);


// ---------------------------------------------------------------------
// 変更 9
// 【探す】
//     static void fillFavList()
//     {
//         ListView_DeleteAllItems(hList);
// 【変更後】
static void fillFavList()
{
    ListView_DeleteAllItems(hList); g_sortCol = -1; setSortArrow(-1, true);


// ---------------------------------------------------------------------
// 変更 10
// 【探す】
//     static void fillList()
//     {
//         g_shown
// 【変更後】
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
    g_shown


// ---------------------------------------------------------------------
// 変更 11
// 【探す】
//     static int selIndex()
//     {
// 【変更後】
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


// ---------------------------------------------------------------------
// 変更 12
// 【探す】
//         MoveWindow(hChunks, left, y, right - left, lh, TRUE);
//         y += S(34);
//         MoveWindow(hPattern
// 【変更後】
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
    MoveWindow(hPattern


// ---------------------------------------------------------------------
// 変更 13
// 【探す】
//         hPreview = mk(L"SlimePreviewWnd", L"", 0, WS_EX_CLIENTEDGE, ID_PREVIEW);
//         EnumChildWindows
// 【変更後】
    hPreview = mk(L"SlimePreviewWnd", L"", 0, WS_EX_CLIENTEDGE, ID_PREVIEW);
    // v1.5.0: サイズ選択ボタンと半径プリセット (Tab の順番がずれないよう「取得中」欄の直後に差し込む)
    hLblSeg = mk(L"STATIC", L"", 0, 0, ID_LBL0 + 8);
    for (int i = 0; i < 5; i++) hSeg[i] = mk(L"BUTTON", L"", WS_TABSTOP | BS_OWNERDRAW, 0, ID_SEG0 + i);
    hLblRad = mk(L"STATIC", L"", 0, 0, ID_LBL0 + 9);
    for (int i = 0; i < 4; i++) hRp[i] = mk(L"BUTTON", L"", WS_TABSTOP | BS_OWNERDRAW, 0, ID_RP0 + i);
    { HWND prev = hChunks;
      HWND order[11] = { hLblSeg, hSeg[0], hSeg[1], hSeg[2], hSeg[3], hSeg[4], hLblRad, hRp[0], hRp[1], hRp[2], hRp[3] };
      for (int i = 0; i < 11; i++) { SetWindowPos(order[i], prev, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); prev = order[i]; } }
    EnumChildWindows


// ---------------------------------------------------------------------
// 変更 14
// 【探す】
//         SendMessage(hChunks, WM_SETFONT, (WPARAM)hFontBold, TRUE);
//         layout();
// 【変更後】
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


// ---------------------------------------------------------------------
// 変更 15
// 【探す】
//         SetWindowTextW(hGo, T(L"探す", L"Search"));
//         SetWindowTextW(hStop
// 【変更後】
    SetWindowTextW(hLblSeg, T(L"サイズ", L"Size"));
    SetWindowTextW(hLblRad, T(L"半径", L"Radius"));
    SetWindowTextW(hSeg[0], L"1\u00d71"); SetWindowTextW(hSeg[1], L"2\u00d72"); SetWindowTextW(hSeg[2], L"3\u00d73"); SetWindowTextW(hSeg[3], L"4\u00d74");
    SetWindowTextW(hSeg[4], T(L"密集", L"Dense"));
    SetWindowTextW(hRp[0], T(L"1万", L"10k")); SetWindowTextW(hRp[1], T(L"5万", L"50k"));
    SetWindowTextW(hRp[2], T(L"10万", L"100k")); SetWindowTextW(hRp[3], T(L"50万", L"500k"));
    SendMessageW(hSeed, EM_SETCUEBANNER, TRUE, (LPARAM)T(L"例：70392165441118850 や好きな文字列", L"e.g. 70392165441118850 or any text"));
    SetWindowTextW(hGo, T(L"探す", L"Search"));
    SetWindowTextW(hStop


// ---------------------------------------------------------------------
// 変更 16
// 【探す】
//         InvalidateRect(hMain, NULL, TRUE);
//         if (hPreview) InvalidateRect(hPreview, NULL, TRUE);
//     }
// 【変更後】
    InvalidateRect(hMain, NULL, TRUE);
    if (hPreview) InvalidateRect(hPreview, NULL, TRUE);
    applyTitleBar();
}


// ---------------------------------------------------------------------
// 変更 17
// 【探す】
//     mi->ptMinTrackSize.y = S(780);
// 【変更後】
mi->ptMinTrackSize.y = S(840);


// ---------------------------------------------------------------------
// 変更 18
// 【探す】
//                     RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
//                     setStatus(T(g_dark
// 【変更後】
                applyTitleBar();
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
                setStatus(T(g_dark


// ---------------------------------------------------------------------
// 変更 19
// 【探す】
//                 case ID_R:
//                     if (HIWORD(wParam) == EN_CHANGE) updateChunks();
//                     return 0;
// 【変更後】
            case ID_R:
                if (HIWORD(wParam) == EN_CHANGE) { updateChunks(); for (int i = 0; i < 4; i++) InvalidateRect(hRp[i], NULL, FALSE); }
                return 0;
            case ID_SEG0: case ID_SEG0 + 1: case ID_SEG0 + 2: case ID_SEG0 + 3: case ID_SEG0 + 4:
                if (!g_running) setMode(wmId - ID_SEG0);
                return 0;
            case ID_RP0: case ID_RP0 + 1: case ID_RP0 + 2: case ID_RP0 + 3:
                if (!g_running) { wchar_t b[24]; _snwprintf(b, 24, L"%lld", RP_VAL[wmId - ID_RP0]); SetWindowTextW(hR, b); }
                return 0;


// ---------------------------------------------------------------------
// 変更 20
// 【探す】
//                 NMHDR* nh = (NMHDR*)lParam;
//                 if (nh->hwndFrom == hList && nh->code == NM_DBLCLK)
// 【変更後】
            NMHDR* nh = (NMHDR*)lParam;
            if (nh->code == TTN_GETDISPINFOW) {
                NMTTDISPINFOW* di = (NMTTDISPINFOW*)lParam;
                const wchar_t* tx = tipFor(GetDlgCtrlID((HWND)nh->idFrom));
                if (tx) di->lpszText = (LPWSTR)tx;
                return 0;
            }
            if (nh->hwndFrom == hList && nh->code == NM_RCLICK) { POINT pt; GetCursorPos(&pt); showResultMenu(pt); return 0; }
            if (nh->hwndFrom == hList && nh->code == LVN_COLUMNCLICK) { onColumnClick(((LPNMLISTVIEW)lParam)->iSubItem); return 0; }
            if (nh->hwndFrom == hList && nh->code == NM_DBLCLK)


// ---------------------------------------------------------------------
// 変更 21
// 【探す】
//                     case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW;
// 【変更後】
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


// ---------------------------------------------------------------------
// 変更 22
// 【探す】
//         RECT r = { 0, 0, MulDiv(940, d, 96), MulDiv(880, d, 96) };
// 【変更後】
    RECT r = { 0, 0, MulDiv(940, d, 96), MulDiv(940, d, 96) };


// ---------------------------------------------------------------------
// 変更 23
// 【探す】
//     ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES
// 【変更後】
ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES


// ---------------------------------------------------------------------
// 変更 24（drawButton 関数をまるごと置き換え）
// 【探す】  static void drawButton(const DRAWITEMSTRUCT* d)  から、次の
//          static HWND mk(...) の直前までを削除して、次の内容に置き換える
// 【変更後】
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
