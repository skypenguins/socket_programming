# Socket Programming - HTTP Server & Client

このディレクトリには、C言語で実装されたシンプルなHTTPサーバーとクライアントのソケットプログラミング実装が含まれています。

## ファイル構成

- [http_server.c](http_server.c) - HTTPサーバー実装（計算機能付き）
- [http_client.c](http_client.c) - HTTPクライアント実装
- `http_server` - コンパイル済みサーバー実行ファイル
- `http_client` - コンパイル済みクライアント実行ファイル

## HTTPサーバー ([http_server.c](http_server.c))

ポート80で動作する簡易HTTPサーバー。計算機能を提供します。

### 主な機能

- **計算API**: `/calc?query=`エンドポイントで四則演算を実行
  - サポートする演算子: `+`, `-`, `*`, `/`
  - URLエンコードされたクエリをデコード
  - 計算結果をHTTPレスポンスとして返却

### 使用例

```bash
# サーバーを起動（ポート80なのでroot権限が必要）
sudo ./http_server

# curlでアクセス
curl "http://localhost/calc?query=5+3"  # 結果: 8
curl "http://localhost/calc?query=10-4"  # 結果: 6
curl "http://localhost/calc?query=6*7"  # 結果: 42
curl "http://localhost/calc?query=20/4"  # 結果: 5
```

### 実装の特徴

- ソケット作成時に`SO_REUSEADDR`オプションを設定し、即座の再起動を可能に
- 接続失敗時は自動リトライ（1秒間隔）
- URLデコード処理を実装
- シンプルなHTTP/1.1レスポンス生成

## HTTPクライアント ([http_client.c](http_client.c))

指定したサーバーにTCP接続してメッセージを送受信するクライアント。

### 使用方法

```bash
./http_client [server] [port] [message]
```

### パラメータ

- `server`: 接続先サーバー名またはIPアドレス（デフォルト: localhost）
- `port`: ポート名またはサービス名（デフォルト: http）
- `message`: 送信するメッセージ（デフォルト: "Hello, world!"）

### 使用例

```bash
# デフォルト設定で実行
./http_client

# 特定のサーバーに接続
./http_client example.com http "GET / HTTP/1.1"

# IPアドレスとポート番号を指定
./http_client 192.168.1.100 8080 "test message"
```

### 実装の特徴

- `getservbyname()`でサービス名からポート番号を解決
- `gethostbyname()`でホスト名からIPアドレスを解決
- IPアドレス直接指定にも対応（`inet_addr()`と`gethostbyaddr()`を使用）
- 構造化されたエラーハンドリング

## コンパイル方法

```bash
# サーバーのコンパイル
gcc -o http_server http_server.c

# クライアントのコンパイル
gcc -o http_client http_client.c
```

## 技術的詳細

### サーバー ([http_server.c](http_server.c))

- **ソケットAPI**: `socket()`, `bind()`, `listen()`, `accept()`
- **ポート**: 80（HTTPデフォルト）
- **バックログ**: 5接続
- **リクエスト処理**: 同期的に1接続ずつ処理
- **HTTPメソッド**: GET `/calc?query=` のみ対応

### クライアント ([http_client.c](http_client.c))

- **ソケットAPI**: `socket()`, `connect()`
- **名前解決**: `getservbyname()`, `gethostbyname()`, `gethostbyaddr()`
- **アドレスファミリ**: AF_INET (IPv4)
- **プロトコル**: TCP (SOCK_STREAM)

## 注意事項

- サーバーはポート80を使用するため、root権限が必要です
- 本実装は教育目的のシンプルな例であり、プロダクション環境での使用は想定していません
- エラーハンドリングは基本的なもののみ実装
- サーバーは単一スレッドで動作するため、同時接続は処理できません
