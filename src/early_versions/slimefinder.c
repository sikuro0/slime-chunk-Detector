// Slime Chunk 3x3 Finder (Java Edition) - console version
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <omp.h>
#ifdef _WIN32
#include <windows.h>
#endif
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
static int cmp(const void*a, const void*b){ double d=((Res*)a)->dist-((Res*)b)->dist; return d<0?-1:d>0; }

// read a line as UTF-16 (Windows console) or UTF-8; returns via utf8 buffer and computes java hash
static char line8[512]; static int32_t linehash;
static void readline(const char *prompt) {
  printf("%s", prompt); fflush(stdout);
#ifdef _WIN32
  wchar_t w[512]; DWORD n = 0;
  HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode;
  if (GetConsoleMode(h, &mode)) {
    ReadConsoleW(h, w, 510, &n, NULL);
  } else {
    char tmp[512]; if (!fgets(tmp, 512, stdin)) tmp[0]=0;
    n = MultiByteToWideChar(CP_UTF8, 0, tmp, -1, w, 510); if (n>0) n--;
  }
  while (n>0 && (w[n-1]==L'\r'||w[n-1]==L'\n')) n--;
  w[n]=0;
  int32_t hsh=0; for (DWORD i=0;i<n;i++) hsh = (int32_t)((uint32_t)hsh*31u + (uint32_t)w[i]);
  linehash = hsh;
  WideCharToMultiByte(CP_UTF8, 0, w, -1, line8, sizeof line8, NULL, NULL);
#else
  if (!fgets(line8, sizeof line8, stdin)) line8[0]=0;
  size_t n=strlen(line8); while(n>0&&(line8[n-1]=='\n'||line8[n-1]=='\r')) line8[--n]=0;
  int32_t hsh=0; for(size_t i=0;i<n;i++) hsh=(int32_t)((uint32_t)hsh*31u+(uint8_t)line8[i]); linehash=hsh;
#endif
}
static long long asknum(const char *prompt, long long def) {
  char p[256]; snprintf(p, sizeof p, "%s[%lld]: ", prompt, def);
  readline(p);
  if (!line8[0]) return def;
  return atoll(line8);
}
static int is_number(const char*s){ if(*s=='-')s++; if(!*s) return 0; for(;*s;s++) if(*s<'0'||*s>'9') return 0; return 1; }

int main(void) {
#ifdef _WIN32
  SetConsoleOutputCP(65001);
#endif
  printf("=== スライムチャンク 3x3 ファインダー (Java版) ===\n\n");
  readline("シード値（数字または文字列）: ");
  if (!line8[0]) { printf("シード値が入力されていません。\n"); goto end; }
  if (is_number(line8) && strlen(line8) <= 19) {
    // parse as signed 64-bit; fall back to hash if out of range
    char *e; long long v = strtoll(line8, &e, 10);
    SEED = v;
    // range check via string length/overflow
    char back[64]; snprintf(back, sizeof back, "%lld", v);
    const char *s = line8; if (*s=='+') s++;
    if (strcmp(back, s) != 0) SEED = (L)linehash;
  } else SEED = (L)linehash;
  long long X = asknum("中心 X（ブロック）", 0);
  long long Z = asknum("中心 Z（ブロック）", 0);
  long long R = asknum("探す半径（ブロック）", 100000);
  long long onlyiso = asknum("外周もきれいなものだけ？ 1=はい 0=いいえ", 0);
  if (R < 64) R = 64; if (R > 1000000) R = 1000000;
  I ccx = (I)floor((double)X/16.0), ccz = (I)floor((double)Z/16.0);
  I rc = (I)((R + 15) / 16);
  I x0 = ccx - rc, z0 = ccz - rc;
  long long W = 2LL*rc + 1, H = 2LL*rc + 1;
  printf("\nシード: %lld / 中心: (%lld, %lld) / 半径: %lld ブロック\n", (long long)SEED, X, Z, R);
  printf("調べるチャンク数: 約 %.2f 億 / スレッド数: %d\n\n", (double)W*H/1e8, omp_get_max_threads());

  const int BAND = 256;
  long long nbands = (H - 2 + BAND - 1) / BAND; // windows start rows: 0..H-3
  long long doneb = 0; double t0 = omp_get_wtime();
  #pragma omp parallel
  {
    unsigned char *g = malloc((size_t)W * (BAND + 2));
    #pragma omp for schedule(dynamic,1)
    for (long long bi = 0; bi < nbands; bi++) {
      long long r0 = bi * BAND;                 // first window top row
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
            if (onlyiso && !iso) continue;
            #pragma omp critical(add)
            {
              if (nres == capres) { capres = capres ? capres*2 : 64; res = realloc(res, capres*sizeof(Res)); }
              double bx = cx*16.0+24, bz = cz*16.0+24;
              res[nres++] = (Res){cx, cz, iso, hypot(bx - X, bz - Z)};
            }
          }
        }
      #pragma omp critical(prog)
      {
        doneb++;
        printf("\r探索中... %5.1f%%  見つかった数: %zu  経過 %.0f秒   ", 100.0*doneb/nbands, nres, omp_get_wtime()-t0);
        fflush(stdout);
      }
    }
    free(g);
  }
  printf("\r完了しました。見つかった数: %zu / 所要時間 %.1f秒                    \n\n", nres, omp_get_wtime()-t0);
  qsort(res, nres, sizeof(Res), cmp);
  FILE *f = fopen("slime_result.txt", "w");
  if (f) fprintf(f, "seed: %lld\ncenter: (%lld, %lld) radius: %lld blocks\n\n", (long long)SEED, X, Z, R);
  size_t show = nres < 50 ? nres : 50;
  if (!nres) printf("この範囲には見つかりませんでした。半径を広げてみてください。\n");
  for (size_t i = 0; i < show; i++) {
    long long bx = (long long)res[i].cx*16+24, bz = (long long)res[i].cz*16+24;
    printf("%2zu. X %lld / Z %lld  (%s, 中心から約%.0fブロック)\n", i+1, bx, bz, res[i].iso?"外周もきれい":"隣にスライムチャンクあり", res[i].dist);
    printf("    チャンク X %d~%d / Z %d~%d   ブロック X %d~%d / Z %d~%d\n", res[i].cx, res[i].cx+2, res[i].cz, res[i].cz+2,
           res[i].cx*16, res[i].cx*16+47, res[i].cz*16, res[i].cz*16+47);
    if (f) fprintf(f, "%zu. X %lld, Z %lld  (%s, dist %.0f)\n   chunk X %d..%d Z %d..%d\n", i+1, bx, bz, res[i].iso?"isolated":"adjacent", res[i].dist,
           res[i].cx, res[i].cx+2, res[i].cz, res[i].cz+2);
  }
  if (f) { fclose(f); if (nres) printf("\n結果を slime_result.txt にも保存しました。\n"); }
end:
  printf("\nEnterキーで終了します。");
  fflush(stdout);
  { char tmp[16]; if (!fgets(tmp, sizeof tmp, stdin)) {} }
  return 0;
}
