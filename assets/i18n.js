/* ============================================================
   表示テキスト（日本語 / 英語）
   ------------------------------------------------------------
   サイトの文章を変えたいときは、このファイルを編集します。
   ja と en が同じキーで並んでいるので、両方を直してください。

   ・HTMLタグ（<strong> や <br>、<li>）はそのまま書けます
   ・" を文章に入れたいときは \" と書きます
   ============================================================ */

window.I18N = {

  /* ========================= 日本語 ========================= */
  ja: {
    "lang.other": "EN",
    "lang.aria": "言語を切り替える",
    "theme.aria": "配色を切り替える",
    "skip": "本文へスキップ",

    "meta.title": "Slime Chunk Detector 日本語版 | Minecraft スライムチャンク判定ツール",
    "meta.desc": "Minecraft（マイクラ）のシード値と座標から、スライムチャンクを高速に自動判定する軽量ツールです。オフライン動作・日本語対応。GitHub Releases から無料でダウンロードできます。",
    "meta.ogTitle": "Slime Chunk Detector 日本語版 - ダウンロード",
    "meta.ogDesc": "Minecraft用のスライムチャンク判定ツール。オフラインで動作する軽量実行ファイルを GitHub Releases からダウンロードできます。",

    "brand.sub": "日本語版 配布ページ",
    "nav.overview": "概要",
    "nav.usage": "使い方",
    "nav.changelog": "更新履歴",
    "nav.safety": "安全性",
    "nav.support": "サポート",

    "hero.eyebrow": "Minecraft Java Edition ユーティリティ",
    "hero.title": "スライムチャンクを<br>一瞬で見つける",
    "hero.lead": "シード値と座標を入れるだけ。インストール不要の単体 exe で、スライムトラップに使えるチャンクをまとめて判定します。",
    "hero.badge1": "完全オフライン動作",
    "hero.badge2": "インストール不要",
    "hero.badge3": "日本語 / English",

    "dl.label": "最新版",
    "dl.loading": "読み込み中…",
    "dl.button": "ダウンロード",
    "dl.file": "ファイル",
    "dl.size": "サイズ",
    "dl.date": "公開日",
    "dl.os": "対応 OS",
    "dl.osValue": "Windows 10 / 11",
    "dl.past": "過去のバージョン",
    "dl.copy": "リンクをコピー",
    "dl.copied": "コピーしました",
    "dl.failed": "最新情報を取得できませんでした。表示は前回の内容です。",
    "dl.noAsset": "対象ファイルが見つかりませんでした。Releases ページから選んでください。",

    "overview.h2": "概要",
    "overview.body": "Slime Chunk Detector は、Minecraft（Java Edition）のスライムチャンクを判定するツールです。シングルプレイでもサーバーでも使え、ゲームバージョンを指定した判定と、見つかった座標の保存に対応しています。",
    "overview.h3": "特徴",

    "feat1.t": "軽量な単体 exe", "feat1.d": "ランタイム不要、ダブルクリックで起動",
    "feat2.t": "多機能",         "feat2.d": "範囲判定・座標検索をまとめて実行",
    "feat3.t": "日本語対応",     "feat3.d": "v1.3.0 から英語表示にも対応",
    "feat4.t": "バージョン指定", "feat4.d": "ゲームバージョンごとの判定に対応",
    "feat5.t": "結果を保存",     "feat5.d": "発見した座標を txt に書き出し",
    "feat6.t": "オフライン",     "feat6.d": "ネットワーク通信は一切行いません",

    "usage.h2": "使い方",
    "step1.t": "exe をダウンロード",
    "step1.d": "上のボタン、または Releases ページから最新版を取得します。",
    "step2.t": "exe を起動",
    "step2.d": "インストールは不要です。保存したファイルをそのまま実行してください。",
    "step3.t": "シード値と座標を入力",
    "step3.d": "ワールドのシード値、探索する座標の範囲、ゲームバージョンを指定します。",
    "step4.t": "結果を確認・保存",
    "step4.d": "スライムチャンクの一覧が表示されます。必要なら txt に書き出せます。",

    "changelog.h2": "更新履歴",
    "changelog.loading": "読み込み中…",
    "changelog.latest": "最新",
    "changelog.failed": "更新履歴を読み込めませんでした。<a href=\"{url}\" target=\"_blank\" rel=\"noopener\">Releases ページ</a>でご確認ください。",

    "safety.h2": "安全性と警告への対処",
    "safety.lead": "署名のない個人製の exe は、Windows やウイルス対策ソフトに警告されることがあります。以下の内容を確認したうえで、必要な場合のみ設定を変更してください。",

    "sa1.s": "このプログラムについて",
    "sa1.p": "シード値からスライムチャンクを判定するだけの単機能ツールです。<strong>ネットワーク接続は行いません</strong>。ソースコードは GitHub リポジトリで公開しています。",

    "sa2.s": "SmartScreen の「WindowsによってPCが保護されました」が出る場合",
    "sa2.list": "<li>警告画面の「詳細情報」をクリック</li><li>表示された「実行」をクリック</li>",
    "sa2.note": "ダウンロード数が少ない新しいファイルに出る警告です。配布元が信頼できるかを必ず確認してください。",

    "sa3.s": "Smart App Control でブロックされる場合",
    "sa3.p": "Windows 11 の Smart App Control は、署名されていない実行ファイルをブロックすることがあります。",
    "sa3.list": "<li>「Windows セキュリティ」を開く</li><li>「アプリとブラウザー コントロール」を選択</li><li>「Smart App Control の設定」を開く</li><li>「オフ」に切り替える（再起動が必要）</li>",
    "sa3.note": "Windows の保護機能のため、自己責任で設定してください。一度オフにすると再度オンにはできません。",

    "sa4.s": "ウイルスと脅威の防止で削除される場合",
    "sa4.list": "<li>「Windows セキュリティ」を開く</li><li>「ウイルスと脅威の防止」を選択</li><li>「設定の管理」をクリック</li><li>「除外の追加または削除」を選択</li><li>「除外の追加」→「ファイル」から exe を指定</li>",
    "sa4.note": "除外設定はセキュリティを弱める可能性があるため、必要な場合のみ設定してください。",

    "sa5.s": "誤検知かどうかを自分で確かめる",
    "sa5.list": "<li>VirusTotal にファイルをアップロードしてスキャン結果を確認する</li><li>GitHub のソースコードを読んで動作を確認する</li><li>必要であればベンダーに誤検知として報告する</li>",
    "sa5.note": "疑わしい点があれば実行を中止し、Issue でご相談ください。",

    "support.h2": "サポート",
    "support.c1t": "不具合報告・要望",
    "support.c1d": "GitHub Issues に投稿してください",
    "support.c2t": "ソースコード",
    "support.c2d": "リポジトリで公開しています",

    "footer.dist": "配布: GitHub Releases",
    "noscript": "JavaScript が無効のため、最新版の情報を表示できません。<a href=\"{url}\">GitHub Releases ページ</a>から直接ダウンロードしてください。"
  },

  /* ========================= English ======================== */
  en: {
    "lang.other": "日本語",
    "lang.aria": "Switch language",
    "theme.aria": "Switch color theme",
    "skip": "Skip to content",

    "meta.title": "Slime Chunk Detector | Slime chunk finder for Minecraft",
    "meta.desc": "A lightweight tool that finds slime chunks from a Minecraft world seed and coordinates. Runs fully offline. Free download from GitHub Releases.",
    "meta.ogTitle": "Slime Chunk Detector - Download",
    "meta.ogDesc": "A slime chunk finder for Minecraft. Download the lightweight, fully offline executable from GitHub Releases.",

    "brand.sub": "Download page",
    "nav.overview": "Overview",
    "nav.usage": "How to use",
    "nav.changelog": "Changelog",
    "nav.safety": "Security",
    "nav.support": "Support",

    "hero.eyebrow": "A utility for Minecraft: Java Edition",
    "hero.title": "Find slime chunks<br>in an instant",
    "hero.lead": "Just enter a seed and a coordinate range. A single executable, no installation, that lists every chunk you could build a slime farm in.",
    "hero.badge1": "Works fully offline",
    "hero.badge2": "No installation",
    "hero.badge3": "English / Japanese",

    "dl.label": "Latest",
    "dl.loading": "Loading…",
    "dl.button": "Download",
    "dl.file": "File",
    "dl.size": "Size",
    "dl.date": "Released",
    "dl.os": "OS",
    "dl.osValue": "Windows 10 / 11",
    "dl.past": "Previous versions",
    "dl.copy": "Copy link",
    "dl.copied": "Copied",
    "dl.failed": "Could not load the latest release. Showing the last known version.",
    "dl.noAsset": "No matching file was found. Please pick one from the Releases page.",

    "overview.h2": "Overview",
    "overview.body": "Slime Chunk Detector finds slime chunks in Minecraft: Java Edition. It works for both single player worlds and servers, lets you pick the game version used for detection, and can save the coordinates it finds.",
    "overview.h3": "Features",

    "feat1.t": "Single lightweight exe", "feat1.d": "No runtime required — just double-click it",
    "feat2.t": "Does the work for you",  "feat2.d": "Area scanning and coordinate lookup in one run",
    "feat3.t": "English and Japanese",   "feat3.d": "English display supported since v1.3.0",
    "feat4.t": "Version aware",          "feat4.d": "Detection matched to the game version",
    "feat5.t": "Save your results",      "feat5.d": "Export the coordinates you find to a txt file",
    "feat6.t": "Offline",                "feat6.d": "Makes no network connections at all",

    "usage.h2": "How to use",
    "step1.t": "Download the exe",
    "step1.d": "Use the button above, or grab the latest build from the Releases page.",
    "step2.t": "Run it",
    "step2.d": "No installation needed. Just run the file you downloaded.",
    "step3.t": "Enter the seed and coordinates",
    "step3.d": "Set the world seed, the coordinate range to search, and the game version.",
    "step4.t": "Check and save the results",
    "step4.d": "The slime chunks are listed. Export them to a txt file if you need to.",

    "changelog.h2": "Changelog",
    "changelog.loading": "Loading…",
    "changelog.latest": "Latest",
    "changelog.failed": "Could not load the changelog. Please check <a href=\"{url}\" target=\"_blank\" rel=\"noopener\">the Releases page</a>.",

    "safety.h2": "Security warnings and how to handle them",
    "safety.lead": "Unsigned software written by an individual is sometimes flagged by Windows or by antivirus software. Read the notes below, and only change a setting if you actually need to.",

    "sa1.s": "About this program",
    "sa1.p": "It is a single-purpose tool that works out slime chunks from a world seed. <strong>It makes no network connections.</strong> The source code is published in the GitHub repository.",

    "sa2.s": "If SmartScreen says \"Windows protected your PC\"",
    "sa2.list": "<li>Click \"More info\" on the warning screen</li><li>Click \"Run anyway\"</li>",
    "sa2.note": "This warning shows up for new files with few downloads. Always make sure you trust where a file came from.",

    "sa3.s": "If Smart App Control blocks the file",
    "sa3.p": "Smart App Control on Windows 11 may block executables that are not signed.",
    "sa3.list": "<li>Open \"Windows Security\"</li><li>Select \"App &amp; browser control\"</li><li>Open \"Smart App Control settings\"</li><li>Switch it to \"Off\" (a restart is required)</li>",
    "sa3.note": "This is a Windows protection feature, so change it at your own risk. Once it is off, it cannot be turned back on.",

    "sa4.s": "If Virus &amp; threat protection deletes the file",
    "sa4.list": "<li>Open \"Windows Security\"</li><li>Select \"Virus &amp; threat protection\"</li><li>Click \"Manage settings\"</li><li>Select \"Add or remove exclusions\"</li><li>Choose \"Add an exclusion\" → \"File\" and pick the exe</li>",
    "sa4.note": "An exclusion can weaken your protection, so only add one if you need it.",

    "sa5.s": "Checking for yourself whether it is a false positive",
    "sa5.list": "<li>Upload the file to VirusTotal and look at the scan results</li><li>Read the source code on GitHub to see what it does</li><li>Report it to the vendor as a false positive if needed</li>",
    "sa5.note": "If anything looks wrong to you, stop running it and open an Issue.",

    "support.h2": "Support",
    "support.c1t": "Bugs and requests",
    "support.c1d": "Please open a GitHub Issue",
    "support.c2t": "Source code",
    "support.c2d": "Published in the repository",

    "footer.dist": "Distributed via GitHub Releases",
    "noscript": "JavaScript is disabled, so the latest release cannot be shown. Please download directly from the <a href=\"{url}\">GitHub Releases page</a>."
  }
};
