# micro MSX2+ for M5Stack

- micro-msx2p を M5Stack Core2 での動作をサポートしたものです
- M5Stack の性能上の問題で MSX2/2+ の再現度の高いエミュレーションは難しいと判断したため MSX1 コアを使っています

## Pre-requests

- [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/index.html)
  - macOS: `brew install platformio`
- GNU Make

> Playform I/O は Visual Studio Code 経由で用いる方式が一般的には多いですが、本プロジェクトでは CLI (pioコマンド) のみ用いるので Visual Studio Code や Plugin のインストールは不要です。

## Build Support OS

- macOS
- Linux

> Windows でビルドしたい場合は WSL2 を使ってください。

### Upload ROM data to SPIFFS

```
make upload-roms
```

上記のコマンドを実行すると [./data](./data) 以下のファイル群を M5Stack の SPIFFS 領域へ書き込みます。[game.rom](./data/game.rom) や BIOS の差し替えをしたい場合、[./data](./data) ディレクトリ以下のファイルを更新後、上記コマンドを実行してください。

### Build and Upload Firmware

```
make
```

上記コマンドを実行すると MSX1 エミュレータコアモジュール [src1](../src1) を [include](include) に展開してから `pio run -t upload` で M5Stack 向けのビルドとアップロード（ファームウェア書き込み）を行います。

> ファームウェア書き込みを実行したくない（ファームウェアのビルドのみしたい）場合は、次のように実行してください。
>
> ```
> make build
> ```
