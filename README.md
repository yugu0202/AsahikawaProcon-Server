
# AsahikawaProcon-Server

北海道旭川市で毎年開催される、[U-16旭川プログラミングコンテスト](http://www.procon-asahikawa.org/)で使用されるサーバーです。

ルールや通信仕様の詳細は公式サイトや同梱のdocファイル等を参照してください。

このサーバーはC++とクロスプラットフォームライブラリQtによって開発されています。
現在のサーバーではドキュメント通りの通信仕様であるため、過去に旭川プロコンで使用されたライブラリ・クライアントと互換性を持ちます。

<div align="center">
  <img src="doc/Screenshot3.png" width=800>
  <img src="doc/Screenshot1.png" width=600>
  <img src="doc/Screenshot2.png" width=600>
</div>

## サーバの起動方法
`AsahikawaProcon-Server.exe` を実行するとサーバの起動できます。

コマンドラインから実行する場合は、以下のようにマップファイルを指定した起動が可能です。マップファイルは相対パス、絶対パスが利用できます。大会等で同じマップで何度も対戦する場合にご利用ください。

- 相対パスの場合: `AsahikawaProcon-Server.exe ./Map/test.map`
- 絶対パスの場合: `AsahikawaProcon-Server.exe C:/Map/test.map`

## クライアント
このサーバーは通常TCPで接続するクライントを、別の特殊なクライアントで代用することが可能です。

* **TCPユーザー** 通常のクライアントです。クライアントとなるPCのAIにより動作します。
* **自動くん** 特に何もしないモードです。デバッグにどうぞ。
* **ManualClient** 別ウインドウでコントローラが開かれ、ユーザーが直接操作できます。
* **botV4** [2020年版のボットプログラム](https://www.procon-asahikawa.org/files/U16asahikawaBot.zip)が動作します。
  ただし、OSはWindows限定で ./2019-U16asahikawaBot/u16asahikawaBot.exe がある場合のみ起動可能です。

## テクスチャ設定
下記のように画像ファイルを配置することで、テクスチャを変更することが可能です。
Imageフォルダがない場合は新規作成してください。
また、画像ファイルはPNG形式である必要があります。

<pre>
.
├── AsahikawaProcon-Server.exe
├── (botV4)
├── (各種dll)
├── (各種設定ファイル)
└── Image
    └── テクスチャ表示名
       ├── Blind.png
       ├── Block.png
       ├── Cool.png
       ├── Floor.png
       ├── Getready.png
       ├── Hot.png
       ├── Item.png
       ├── Look.png
       └── Search.png
</pre>

## サーバー設定
サーバーの動作設定ができます。また、**設定は再起動後有効になります。**

* **ログ保存場所** ログを保存する場所です。初期設定はカレントディレクトリになっていますが、胡散臭いので絶対パスに変えることを推奨します。
* **ゲーム進行速度** ゲーム中のアニメーションの待ち時間です。初期設定は150[ms]（1000ms=1秒）です。少ないほど高速になりますが、処理時間の都合上一定速度以下にはなりません。
* **通信タイムアウト時間** TCPクライアントにおけるレスポンスの待ち時間です。長ければ長いほどクライアントがタイムアウトしにくくなります。

## 開発環境(Qt6)
- Windows 11 25H2
- Qt Creator 20.0.0
- Desktop Qt 6.10.3 MinGW 64-bit

## 開発環境(Qt5)
- MacOSX 10.11.3 ElCapitan
- Qt Creator 3.3.0
- Desktop Qt 5.4.2 clang 64bit


## 自動テスト用の起動

Qt 6 でビルドした実行ファイルにマップを指定し、`--headless` を付けると、
画面を表示せず TCP の接続待機と対戦開始を行います。

```sh
./AsahikawaProcon-Server /absolute/path/test.map --headless
```

COOL は 2009 番、HOT は 2010 番ポートで待ち受けます。両クライアントの接続と
名前の送信後に対戦を開始します。終了時は標準出力に
`RESULT DRAW 0 0` の形式で勝者（`COOL`、`HOT`、`DRAW`）と両者の得点を出力し、
プロセスを終了します。ポートが使用中の場合は終了コード 2 で終了します。
入力するマップは 15×17 または 21×17 の正常なマップを用意してください。
既存の読込処理の制約により、マップ名は空でなく、先頭文字に T・D・C・H を使用できません。
クライアントの切断・タイムアウト・不正応答では `ERROR CLIENT_FAILURE COOL` のように
標準エラーへ対象チームを出し、終了コード 3 で終了します。
接続待ち時間や対戦時間の上限は呼び出し側で設定してください。

自動テストは Qt Test とビルド済み実行ファイルを使って実行します。

```sh
mkdir -p build-tests && cd build-tests
qmake ../tests/cli.pro
make
CHASER_SERVER_BINARY=/absolute/path/AsahikawaProcon-Server ./cli_test
```

### CI と CLI ビルドの配布

`master` または `main` への push ごとに、テスト成功後の Linux x86_64 向け
実行ファイルを `cli-<コミット SHA>` のプレリリースに保存します。リリースには
`cli-linux-x86_64.tar.gz` と SHA-256 の検証ファイルが含まれます。
Ubuntu 24.04 向けの動的リンク版であり、実行環境には Qt 6 のランタイムが必要です。
例えば Ubuntu 24.04 では `sudo apt-get install libqt6widgets6 libqt6network6 libqt6multimedia6 qt6-qpa-plugins` で導入できます。
別リポジトリでは、対象コミットのリリースを指定して取得し、ハッシュを確認してください。

リリースの再実行では、未公開ドラフトの成果物を再アップロードしてから公開します。
公開済みの同一 SHA のリリースは変更しません。
