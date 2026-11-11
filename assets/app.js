/* ============================================================
   Slime Chunk Detector 配布ページ スクリプト
   ------------------------------------------------------------
   ・表示言語の判定（日本語ブラウザ → 日本語、それ以外 → 英語）
   ・GitHub Releases から最新版の情報と更新履歴を自動取得
   ・配色（ダーク / ライト）の切り替え
   ・ダウンロードリンクのコピー
   通常の更新で、このファイルを触る必要はありません。
   文章の修正は assets/i18n.js で行います。
   ============================================================ */

(function () {
  "use strict";

  var CFG = window.SITE_CONFIG || {};
  var API = "https://api.github.com/repos/" + CFG.owner + "/" + CFG.repo + "/releases?per_page=10";
  var CACHE_KEY = "scd:releases";
  var CACHE_MIN = 30; // 分：この間はAPIを叩かずキャッシュを使う（API制限対策）

  var LANG = "ja";       // 現在の表示言語
  var RELEASES = null;   // 取得済みリリース（言語切替時の再描画用）
  var LOAD_FAILED = false;

  var $ = function (id) { return document.getElementById(id); };

  /* ---------- 言語 ---------- */

  function detectLang() {
    var saved = null;
    try { saved = localStorage.getItem("scd:lang"); } catch (e) {}
    if (saved === "ja" || saved === "en") return saved;

    var list = (navigator.languages && navigator.languages.length)
      ? navigator.languages
      : [navigator.language || navigator.userLanguage || "en"];

    for (var i = 0; i < list.length; i++) {
      if (/^ja\b/i.test(list[i])) return "ja";
    }
    return "en";
  }

  function t(key) {
    var dict = (window.I18N && window.I18N[LANG]) || {};
    return dict[key] != null ? dict[key] : "";
  }

  function applyLang(lang) {
    LANG = lang;
    if (!window.I18N || !window.I18N[lang]) return; // i18n.js が無ければHTMLの文面をそのまま使う

    document.documentElement.lang = lang;
    if (t("meta.title")) document.title = t("meta.title");
    setMeta('meta[name="description"]', "content", t("meta.desc"));
    setMeta('meta[property="og:title"]', "content", t("meta.ogTitle"));
    setMeta('meta[property="og:description"]', "content", t("meta.ogDesc"));

    each(document.querySelectorAll("[data-i18n]"), function (el) {
      var v = t(el.getAttribute("data-i18n"));
      if (v) el.innerHTML = v;
    });
    each(document.querySelectorAll("[data-i18n-aria]"), function (el) {
      var v = t(el.getAttribute("data-i18n-aria"));
      if (v) { el.setAttribute("aria-label", v); el.setAttribute("title", v); }
    });

    // 動的に作っている部分を描き直す
    if (RELEASES) { render(RELEASES); }
    else if (LOAD_FAILED) { renderFallback(); }

    function setMeta(sel, attr, val) {
      var el = document.querySelector(sel);
      if (el && val) el.setAttribute(attr, val);
    }
  }

  function initLangToggle() {
    var btn = $("langToggle");
    if (!btn) return;
    btn.addEventListener("click", function () {
      var next = LANG === "ja" ? "en" : "ja";
      try { localStorage.setItem("scd:lang", next); } catch (e) {}
      applyLang(next);
    });
  }

  function each(nodes, fn) { Array.prototype.forEach.call(nodes, fn); }

  /* ---------- 配色の切り替え ---------- */

  function initTheme() {
    var saved = null;
    try { saved = localStorage.getItem("scd:theme"); } catch (e) {}
    var prefersLight = window.matchMedia && window.matchMedia("(prefers-color-scheme: light)").matches;
    apply(saved || (prefersLight ? "light" : "dark"));

    var btn = $("themeToggle");
    if (btn) {
      btn.addEventListener("click", function () {
        var next = document.documentElement.getAttribute("data-theme") === "light" ? "dark" : "light";
        apply(next);
        try { localStorage.setItem("scd:theme", next); } catch (e) {}
      });
    }

    function apply(th) {
      document.documentElement.setAttribute("data-theme", th);
      var meta = document.querySelector('meta[name="theme-color"]');
      if (meta) meta.setAttribute("content", th === "light" ? "#f2f6fa" : "#0a1626");
    }
  }

  /* ---------- 固定リンクと著作権表記 ---------- */

  function initStaticLinks() {
    var base = "https://github.com/" + CFG.owner + "/" + CFG.repo;
    setHref("releasesLink", base + "/releases");
    setHref("issuesLink", base + "/issues");
    setHref("repoLink", base);

    var cr = $("footerCopyright");
    if (cr && CFG.copyright) {
      cr.textContent = CFG.copyright.replace("{year}", String(new Date().getFullYear()));
    }
    function setHref(id, url) { var el = $(id); if (el) el.href = url; }
  }

  /* ---------- 表示用のユーティリティ ---------- */

  function formatBytes(n) {
    if (typeof n !== "number" || !isFinite(n)) return "—";
    return n >= 1048576 ? (n / 1048576).toFixed(2) + " MB" : (n / 1024).toFixed(1) + " KB";
  }

  function formatDate(iso) {
    if (!iso) return "—";
    var d = new Date(iso);
    if (isNaN(d.getTime())) return "—";
    try {
      return d.toLocaleDateString(LANG === "ja" ? "ja-JP" : "en-US",
        { year: "numeric", month: "long", day: "numeric" });
    } catch (e) {
      return d.toISOString().slice(0, 10);
    }
  }

  function escapeHtml(s) {
    return String(s).replace(/[&<>"']/g, function (c) {
      return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c];
    });
  }

  /* ---------- リリース情報の取得 ---------- */

  function loadReleases() {
    var el = $("dlVersion");
    if (el) el.textContent = t("dl.loading") || "…";

    var cached = readCache();
    if (cached) { RELEASES = cached; render(cached); return; }

    fetch(API, { headers: { Accept: "application/vnd.github+json" } })
      .then(function (res) {
        if (!res.ok) throw new Error("HTTP " + res.status);
        return res.json();
      })
      .then(function (data) {
        var list = (data || []).filter(function (r) { return !r.draft; });
        if (!list.length) throw new Error("no releases");
        writeCache(list);
        RELEASES = list;
        render(list);
      })
      .catch(function () { LOAD_FAILED = true; renderFallback(); });
  }

  function readCache() {
    try {
      var raw = localStorage.getItem(CACHE_KEY);
      if (!raw) return null;
      var o = JSON.parse(raw);
      if (Date.now() - o.t > CACHE_MIN * 60000) return null;
      return o.d;
    } catch (e) { return null; }
  }

  function writeCache(d) {
    try { localStorage.setItem(CACHE_KEY, JSON.stringify({ t: Date.now(), d: d })); } catch (e) {}
  }

  /* ---------- 画面への反映 ---------- */

  function render(list) {
    var latest = list[0];
    var asset = pickAsset(latest);

    setDownload({
      version: latest.tag_name || latest.name || "—",
      fileName: asset ? asset.name : (CFG.fallback && CFG.fallback.fileName) || "—",
      size: asset ? formatBytes(asset.size) : "—",
      date: formatDate(latest.published_at),
      url: asset ? asset.browser_download_url
                 : (latest.html_url || (CFG.fallback && CFG.fallback.url))
    });

    note(asset ? "" : t("dl.noAsset"));
    renderChangelog(list.slice(0, CFG.changelogCount || 3));
  }

  function pickAsset(release) {
    var assets = release.assets || [];
    var m = (CFG.assetMatch || ".exe").toLowerCase();
    for (var i = 0; i < assets.length; i++) {
      if (String(assets[i].name).toLowerCase().indexOf(m) !== -1) return assets[i];
    }
    return assets[0] || null;
  }

  function renderFallback() {
    var f = CFG.fallback || {};
    setDownload({
      version: f.version || "—",
      fileName: f.fileName || "—",
      size: f.size || "—",
      date: f.date ? formatDate(f.date) : "—",
      url: f.url || ("https://github.com/" + CFG.owner + "/" + CFG.repo + "/releases")
    });
    note(t("dl.failed"));

    var box = $("changelogList");
    if (box) {
      box.innerHTML = '<p class="muted">' + (t("changelog.failed") || "")
        .replace("{url}", "https://github.com/" + CFG.owner + "/" + CFG.repo + "/releases") + "</p>";
    }
  }

  function setDownload(info) {
    text("dlVersion", info.version);
    text("dlFileName", info.fileName);
    text("dlSize", info.size);
    text("dlDate", info.date);
    text("mobileVersion", info.version);

    ["downloadBtn", "mobileDownloadBtn"].forEach(function (id) {
      var el = $(id);
      if (!el) return;
      el.href = info.url;
      if (/\.(exe|zip|jar)$/i.test(info.url)) el.setAttribute("download", info.fileName);
    });

    initCopy();
    initMobileBar();

    function text(id, v) { var el = $(id); if (el) el.textContent = v; }
  }

  function renderChangelog(list) {
    var box = $("changelogList");
    if (!box) return;
    box.innerHTML = list.map(function (r, i) {
      var title = r.name || r.tag_name || "";
      return '<article class="release">'
        + '<div class="release-head">'
        + '<span class="rel-title">' + escapeHtml(title) + "</span>"
        + (i === 0 ? '<span class="latest">' + escapeHtml(t("changelog.latest")) + "</span>" : "")
        + '<time datetime="' + escapeHtml(r.published_at || "") + '">' + formatDate(r.published_at) + "</time>"
        + "</div>"
        + "</article>";
    }).join("");
  }

  function note(msg) { var el = $("dlNote"); if (el) el.textContent = msg || ""; }

  /* ---------- リンクのコピー ---------- */

  function initCopy() {
    var btn = $("copyLinkBtn");
    if (!btn || btn.dataset.ready) return;
    btn.dataset.ready = "1";
    btn.addEventListener("click", function () {
      var dl = $("downloadBtn");
      var target = dl ? dl.href : "";
      var done = function () {
        btn.textContent = t("dl.copied") || "Copied";
        setTimeout(function () { btn.textContent = t("dl.copy") || "Copy link"; }, 1600);
      };
      if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(target).then(done, function () { note(target); });
      } else {
        note(target);
      }
    });
  }

  /* ---------- モバイル用の固定バー ---------- */

  function initMobileBar() {
    var bar = $("mobileBar");
    var card = $("downloadCard");
    if (!bar || !card || bar.dataset.ready) return;
    bar.dataset.ready = "1";

    if (!("IntersectionObserver" in window)) return;
    new IntersectionObserver(function (entries) {
      bar.hidden = entries[0].isIntersecting || window.innerWidth > 600;
    }, { threshold: 0.15 }).observe(card);
  }

  /* ---------- 起動 ---------- */

  initTheme();
  applyLang(detectLang());
  initLangToggle();
  initStaticLinks();
  loadReleases();
})();
