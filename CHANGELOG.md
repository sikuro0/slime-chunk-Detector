# Changelog

**English** | [日本語](#変更履歴)

## v1.5.0

A large improvement to usability and appearance. The search itself (the results) is unchanged from v1.4.1.

### Added

- **Pattern size buttons** — Added "1×1 / 2×2 / 3×3 / 4×4 / Dense" buttons to the main window, so you can switch pattern with a single click without opening a menu (the active one is highlighted in green; the settings menu still works as before and stays in sync).
- **Radius presets** — Added buttons for 10k / 50k / 100k / 500k blocks, so you can set a radius without typing a number (shown as 1万 / 5万 / 10万 / 50万 in Japanese).
- **Right-click menu in the result list** — Copy coordinates, copy a /tp command, open in Chunkbase, center the search here, and add to (or remove from) favorites, straight from a row in the list.
- **Click a column header to sort** — Sort by center X, center Z, contents or distance (click again to reverse; an arrow shows the current order).
- **Coordinate paste helper** — Pasting text such as "123 -456", "X: 123, Z: -456" or "/tp @s 123 64 -456" into the center X/Z fields splits it into X and Z automatically.
- **Tooltips** — Hovering over a button or an input field shows a short explanation of what it does (Japanese / English).
- **The title bar goes dark in dark mode too** (Windows 10 1809 and later).

### Changed

- Buttons change color on hover, and the cursor becomes a hand.
- When there are no results yet, usage guidance is shown inside the list (with its own wording for the favorites view and while a search is running).
- The seed field now shows a faint example of what to type.
- The pattern size and radius buttons are also disabled while a search is running, so the search conditions cannot change part-way through.
- Adjusted the window's initial height and the minimum height it can be shrunk to, to fit the new button rows.

## v1.4.1 (addendum)

### Fixed

- Fixed a bug where the preview pane (its heading, guidance text and legend) stayed in the language used at startup and would not change until the app was restarted. The preview pane is now redrawn immediately when the language is switched.

## v1.4.1

### Fixed

- Fixed a bug where the following stayed in Japanese after switching the display to English
  - "Help" and "Exit" in the top menu
  - The placeholder text in the search history field, and the message shown when there is no history
  - The "Save location" label
  - The heading, guidance text and legend in the preview pane
  - The "Center X" and "Center Z" column headers in the result list (these columns are created only once when the app starts, so they were not being updated when the language was switched)
  - The wording of "1.7 (link treated as 1.8)" in the Java version list
- Fixed a layout problem where, in English, the "Search history" label was long enough to push the dropdown beside it underneath the buttons

## v1.4.0

### Added

- **A mode that finds only the nearest match, instantly** — Added a new search method that expands outward from the center in rings. Tick the checkbox and press Search, and instead of scanning the whole radius it stops as soon as the nearest match is found (and it is certain that no closer candidate exists), so if there is something nearby you get a result in seconds to tens of seconds. Works with 1×1 to 4×4 and with dense mode.
- **Exclusion list** — A dedicated window, opened from the Tools menu, where you can register "exclusion zones" with coordinates, a radius and a note (up to 100). Turn on "Apply exclusion list" and candidates inside those areas are excluded from both the normal search and the nearest-match mode. Useful when you want to search around bases and builds you already have. The settings are saved automatically and remembered the next time you start.

## v1.3.0

### Added

- **Batch search across multiple seeds** (Tools menu) — A new window where you enter one seed per line and search them all together (up to 200). The search conditions (center, radius, pattern and so on) are taken from the main window as they are. Results show which seed each match came from, sorted by distance and by count. You can copy coordinates or open Chunkbase from each row.
- **Multiple languages (Japanese / English)** — The display language can be switched from the settings menu. Almost the entire interface can be shown in English, including menus, buttons, labels, list headers, the contents of search results, the how-to and slime chunk explanations, and the main message dialogs. The language you pick is remembered the next time you start.
  - For now, the pattern shown in favorites and search history, and the contents of saved txt/CSV files, remain in Japanese (planned for a future update).

### Fixed

- **Fixed a bug where the app quit without responding, or could not be closed, on the second and later launches** — The cause was restoring the previous window position and size before window creation had fully finished (part-way through `WM_CREATE`). On some systems the window ended up in an invalid state during this (off-screen or hidden), which looked as though it had closed right after starting, or pressing Exit put the confirmation dialog off-screen where it could not be used, leaving only the process running in the background. The restore now happens after window creation has completed, and Exit now also reliably closes the Help and converter child windows.

## v1.2.0

### Added

- **Favorites** — Pick a place and use "☆ Add to favorites" to save it together with its seed and version (up to 300). "★ View favorites" switches to the list of them. While favorites are shown the button becomes "🗑 Delete selected", so you can remove them at any time. "Center here" restores the favorite's seed and version as well, so mixing favorites from several seeds never gets confusing.
- **4×4 search** — Added to the settings menu. The odds are about 1 in 100 quadrillion, which is extremely rare, so when there is no fully complete place it instead lists "near misses" (places that are only a little short) in order of how many they contain. The same mechanism is applied to 3×3.
- **Result preview** — Selecting a place in the list draws a simple diagram of the slime chunks around it on the right. Green squares are slime chunks, and the frame is the K×K area that was searched.
- **CSV export** — "Save results as CSV" in the File menu saves in a format you can open in Excel and other spreadsheet software.
- **Block ⇔ chunk converter** — A small window just for coordinate conversion, opened from the new Tools menu. Block → chunk/region and chunk → block range are both calculated live as you type.
- **Dark mode** — Switchable from the settings menu. Remembered the next time you start.
- **Export / import settings** — From the Tools menu you can write your settings, search history and favorites out to a single file, and load them on another PC.
- **Reset search history button** — Added next to the history field, with a confirmation dialog. Also available from the File menu.
- Shortcut keys: F5 (search), Esc (stop), F1 (how to use), Ctrl+S (save results)
- Shows an estimate of the remaining search time
- When a search finishes while you are looking at another window, the taskbar flashes and a sound plays
- The place selected in the list can be set as the center of the next search with the "Center here" button
- Added "Reset inputs to defaults" to the settings menu, and "Clear displayed results" to the File menu
- The window position and size are restored the next time you start
- Trying to close the window during a search now shows a confirmation dialog

### Changed

- Redesigned the whole UI: a pale mint green background, a gradient banner, rounded cards, round buttons and a striped result list
- The number of chunks to be checked is shown live under the radius field (with a warning once it goes over a billion)
- Expanded the Java version choices to 24 (26.3 down to 1.7), and reflected this in the Chunkbase link

### Fixed

- Fixed a problem in the block ⇔ chunk converter where layout code that depended on the window Z-order could occasionally swap the X and Z labels

---

## v1.1.0

### Added

- Search history (up to 30) and recalling conditions from it
- Dense search mode (at least a given number of slime chunks within a K×K area), which is the groundwork for 4×4 search

---

## v1.0.0

### Added

- First release. Searches for 1×1 to 3×3 slime chunks from a seed, a Java version, center coordinates and a radius
- An option to narrow the results down to places with a clean outer edge
- The result list, copying coordinates, copying a `/tp` command, and opening in Chunkbase
- Saving search results to a txt file (with an auto-save option)

<br>

---
---

<br>

# 変更履歴

[English](#changelog) | **日本語**

## v1.5.0

使いやすさと見た目を大きく改良しました。検索の中身（結果）は v1.4.1 と同じです。

### 追加

- **サイズ選択ボタン** — メイン画面に「1×1 / 2×2 / 3×3 / 4×4 / 密集」のボタンを追加。メニューを開かなくても、ワンクリックでパターンを切り替えられる（選択中は緑色で強調。設定メニューからの切り替えも従来どおり使え、同期する）。
- **半径プリセット** — 「1万 / 5万 / 10万 / 50万」ブロックのボタンを追加。数字を入力しなくても半径を決められる（English表示では 10k / 50k / 100k / 500k）。
- **結果一覧の右クリックメニュー** — 座標コピー、/tp コピー、Chunkbaseで開く、ここを中心にする、お気に入りの追加（削除）を、一覧の行から直接実行できる。
- **列見出しクリックで並べ替え** — 中心X・中心Z・内容・距離で並べ替え可能（もう一度クリックで逆順。矢印で現在の並びを表示）。
- **座標の貼り付け補助** — 「123 -456」「X: 123, Z: -456」「/tp @s 123 64 -456」のような文字を中心X・Z欄に貼り付けると、X と Z に自動で振り分けて入力される。
- **ツールチップ** — ボタン・入力欄にマウスを置くと、使い方の説明が表示される（日本語／English 対応）。
- **ダークモード時にタイトルバーも暗くする**（Windows 10 1809 以降）。

### 変更

- ボタンにマウスを乗せると色が変わり、カーソルが手の形になる。
- 結果がまだ無いとき、一覧の中に使い方の案内を表示（お気に入り画面・検索中は、それぞれ専用の文言）。
- シード値の入力欄に、入力例をうすく表示。
- 検索中は、サイズ選択ボタン・半径ボタンも操作できないようにした（検索条件が途中で変わらないように）。
- ウィンドウの初期の高さと、縮められる最小の高さを調整（ボタン行の追加に合わせて）。

## v1.4.1（追記）

### 修正

- プレビュー欄（見出し・案内文・凡例）が、起動時の言語のまま固定され、アプリを再起動しないと切り替わらなかった不具合を修正。言語を切り替えたとき、プレビュー欄もすぐに再描画されるようにした。

## v1.4.1

### 修正

- 英語表示（English）に切り替えたとき、以下が日本語のまま残っていた不具合を修正
  - 上部メニューの「ヘルプ」「終了する」
  - 検索履歴欄のプレースホルダー文、履歴が無いときのメッセージ
  - 「保存先」ラベル
  - プレビュー欄の見出し・案内文・凡例
  - 結果一覧の列見出し「中心 X」「中心 Z」（これはアプリ起動時に一度だけ作られる列のため、言語を切り替えても更新されていませんでした）
  - Javaバージョン一覧の「1.7（リンクは1.8扱い）」の表記
- 英語表示で「検索履歴」ラベルの文字（Search history）が長く、隣の選択欄がボタンの下に隠れてしまっていたレイアウトを修正

## v1.4.0

### 追加

- **最も近い1件だけを一瞬で探すモード** — 中心座標から外側へリング状に広げながら探す新しい検索方式を追加。チェックボックスをオンにして「探す」を押すと、指定した半径の全体をスキャンせず、最も近い1件が見つかった時点（かつ、それより近い候補がもう存在しないと確定した時点）で検索を打ち切るため、近くに該当する場所があれば数秒〜数十秒で結果が出る。1×1〜4×4・密集モードのいずれでも使用可能。
- **除外リスト** — ツールメニューから開く専用ウィンドウで、座標・半径・メモを指定して「除外ゾーン」を登録できる（最大100件）。「除外リストを適用する」をオンにすると、登録した範囲内の候補は通常検索・最も近い1件モードの両方から除外される。既存の拠点や建築物を避けて探したいときに使用。設定は自動的に保存され、次回起動時も記憶される。

## v1.3.0

### 追加

- **複数シード一括検索**（ツールメニュー）— シード値を1行に1つずつ入力して、まとめて検索できる新しいウィンドウを追加（最大200件）。検索条件（中心・半径・パターンなど）はメイン画面の設定をそのまま使用。結果には、どのシードで見つかったかが一緒に表示され、近い順・個数順に並ぶ。各行から座標コピーやChunkbaseを開く操作も可能。
- **多言語対応（日本語／English）** — 設定メニューから表示言語を切り替え可能。メニュー、ボタン、ラベル、一覧の見出し、検索結果の内容表示、使い方・スライムチャンク説明、主要なメッセージダイアログを含む、画面のほぼ全体を英語表示にできる。切り替えた言語は次回起動時も記憶される。
  - 現時点では、お気に入り・検索履歴のパターン表示、および保存されるtxt/CSVファイルの中身は日本語のままです（今後のアップデートで対応予定）。

### 修正

- **2回目以降の起動時にアプリが応答なく終了する／終了できなくなる不具合を修正** — 前回のウィンドウ位置・大きさを復元する処理を、ウィンドウの作成が完全に終わる前（`WM_CREATE` の途中）に実行していたことが原因。環境によってはこの処理中にウィンドウが不正な状態（画面外・非表示）になり、見た目には「起動直後に閉じた」ように見えたり、「終了する」を押しても確認ダイアログが画面外に表示されて操作できず、プロセスだけ裏で残ってしまうことがあった。復元処理をウィンドウ作成が完了した後に行うよう修正し、あわせて「終了する」実行時にヘルプ・変換ツールの子ウィンドウも確実に閉じるようにした。

## v1.2.0

### 追加

- **お気に入り** — 場所を選んで「☆ お気に入りに追加」でシード・バージョンごと保存（最大300件）。「★ お気に入りを見る」で一覧表示に切り替え可能。お気に入り表示中はボタンが「🗑 選択を削除」になり、いつでも取り除ける。「ここを中心にする」はお気に入りのシード・バージョンも呼び出すので、複数シードのお気に入りを混ぜて保存しても迷わない。
- **4×4 検索** — 設定メニューに追加。確率は約10京分の1と非常にまれなため、完全にそろった場所が無いときは「惜しい場所」（あと少しでそろう場所）を個数の多い順に表示する。3×3にも同じ仕組みを適用。
- **結果プレビュー** — 一覧で場所を選ぶと、右側にそのまわりのスライムチャンクを簡単な図で表示。緑のマスがスライムチャンク、枠が検索したK×Kの範囲。
- **CSV出力** — ファイルメニューの「結果をCSVで保存」で、Excelなどの表計算ソフトで開ける形式で保存可能。
- **ブロック⇔チャンク変換ツール** — 新しい「ツール」メニューから開ける、座標変換専用の小窓。ブロック→チャンク／リージョン、チャンク→ブロック範囲を、どちらも入力しながらリアルタイムに計算。
- **ダークモード** — 設定メニューから切り替え可能。次回起動時も設定を記憶。
- **設定のエクスポート／インポート** — ツールメニューから、設定・検索履歴・お気に入りをまとめてファイルに書き出したり、別のPCで読み込んだりできる。
- **検索履歴の初期化ボタン** — 履歴欄の隣に専用ボタンを追加（確認ダイアログあり）。ファイルメニューからも実行可能。
- ショートカットキー：F5（探す）、Esc（止める）、F1（使い方）、Ctrl+S（結果を保存）
- 検索の残り時間の目安を表示
- 検索が完了したとき、他のウィンドウを見ているとタスクバーが点滅して通知音が鳴る
- 一覧で選んだ場所を「ここを中心にする」ボタンで次の検索の中心に設定可能
- 設定メニューに「入力を初期値に戻す」、ファイルメニューに「表示中の結果をクリア」を追加
- ウィンドウの位置・大きさを次回起動時に復元
- 検索中にウィンドウを閉じようとすると確認ダイアログを表示

### 変更

- UI全体を再デザイン。淡いミントグリーンの背景、グラデーションのバナー、角丸カード、丸いボタン、縞模様の一覧表示に変更
- 半径の入力欄の下に「調べるチャンク数」をリアルタイム表示（件数が10億を超えると注意書きを表示）
- Javaバージョンの選択肢を24種類（26.3〜1.7）に拡充。Chunkbaseへのリンクに反映

### 修正

- ブロック⇔チャンク変換ツールで、ウィンドウのZ順序に依存したレイアウト処理により、まれにX/Zのラベル表示が入れ替わる問題を修正

---

## v1.1.0

### 追加

- 検索履歴（最大30件）と、履歴からの条件呼び出し
- 4×4検索の土台となる密集検索モード（K×Kの範囲に指定個数以上のスライムチャンク）

---

## v1.0.0

### 追加

- 初回リリース。シード値・Javaバージョン・中心座標・半径を指定して、1×1〜3×3のスライムチャンクを検索
- 外周がきれいな場所だけに絞り込むオプション
- 結果の一覧表示、座標コピー、`/tp` コマンドコピー、Chunkbaseで開く機能
- 検索結果をtxtファイルに保存（自動保存オプションあり）
