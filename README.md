# Socket Programming - HTTP Client/Server

C言語によるシンプルなHTTPクライアント・サーバーの実装。IPv4/IPv6デュアルスタックに対応。

## ファイル構成

```
socket_programming/
├── http_server.c      # HTTPサーバー実装
├── http_client.c      # HTTPクライアント実装
├── calculator.c       # 計算機能の実装
├── calculator.h       # 計算機能のヘッダー
├── http_utils.c       # HTTPユーティリティ関数の実装
├── http_utils.h       # HTTPユーティリティ関数の宣言
├── Makefile           # ビルドシステム
└── README.md          # このファイル
```

### モジュール構成

- **calculator.c/h**: 計算機能のコア実装
  - `validate_query()`: クエリ文字列の検証（長さ制限、不正文字チェック）
  - `calculate_query()`: 数式の解析と計算実行
  - 四則演算のサポート（+, -, *, /）
  - 整数オーバーフローチェック、ゼロ除算防止

- **http_utils.c/h**: HTTPユーティリティ関数
  - `url_decode()`: URLデコード
  - その他HTTPリクエスト処理に必要な機能

## 特徴

### HTTPサーバー ([http_server.c](http_server.c))

- **計算機能**: HTTP GET リクエストで数式を計算（`/calc?query=`エンドポイント）
- **セキュリティ強化**:
  - 入力検証（バッファオーバーフロー対策）
  - 整数オーバーフローチェック
  - URLデコードの厳密な検証
- **適切なHTTPレスポンス**: ステータスコード付き（200, 400, 500）
- **グレースフルシャットダウン**: SIGINT/SIGTERMハンドリング
- **クライアント情報ロギング**: 接続元IPアドレスとポートの記録
- **SIGPIPE対策**: クライアント切断時のクラッシュ防止

### HTTPクライアント ([http_client.c](http_client.c))

- **モダンAPI使用**: `getaddrinfo()`を使用した名前解決
- **堅牢なエラーハンドリング**: 適切なエラーチェックとリソース管理
- **部分送受信対応**: 部分的なwrite/readへの対応
- **タイムアウト設定**: ソケットタイムアウトによるハング防止
- **入力検証**: メッセージ長の制限

## ビルド方法

```bash
# すべてビルド
make

# デバッグビルド（AddressSanitizer有効）
make DEBUG=1

# クリーンビルド
make rebuild
```

## 実行方法

### サーバーの起動

```bash
# ポート80を使用するため、root権限が必要
sudo ./http_server
```

または

```bash
make run-server
```

### クライアントの実行

```bash
# デフォルト設定で実行
./http_client

# サーバー、ポート、メッセージを指定
./http_client localhost http "GET /calc?query=2%2b11 HTTP/1.1"
```

または

```bash
make run-client
```

## 使用例

### 計算機能のテスト

```bash
# curlを使用した例
curl -G --data-urlencode "query=2+10" localhost/calc
# 結果: 12
```

## コード品質

### コンパイラ警告
- `-Wall -Wextra -Werror`: すべての警告を有効化しエラーとして扱う
- C17標準準拠 (`-std=c17`)
- セキュリティ関連の警告を有効化

### 静的解析

```bash
# clang-tidyによる静的解析
make analyze
```
