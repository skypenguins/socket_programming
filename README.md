# Socket Programming - HTTP Client/Server

C言語によるシンプルなHTTPクライアント・サーバーの実装。IPv4/IPv6デュアルスタックに対応。

## ファイル構成

```
socket_programming/
├── http_client.c      # HTTPクライアント実装
├── http_server.c      # HTTPサーバー実装（ユーティリティ関数も実装）
├── common.h           # 共通定義（マクロとインクルード）
├── http_utils.h       # HTTPユーティリティ関数の宣言
├── Makefile           # ビルドシステム
└── README.md          # このファイル
```

### ヘッダーファイルの役割

- **common.h**: 共通の定数定義とインクルード（実装は含まない）
- **http_utils.h**: 再利用可能なHTTPユーティリティ関数の宣言
  - `url_decode()`: URLデコード
  - `validate_query()`: クエリ検証
  - `calculate_query()`: 計算処理

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
./http_client localhost 80 "GET /calc?query=5+3 HTTP/1.1"
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
