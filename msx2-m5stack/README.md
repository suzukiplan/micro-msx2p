# micro MSX2+ for M5Stack

## Pre-requests

- Platform I/O
- GNU Make

## How to Build and Update firmware

```
make
```

> [Makefile](Makefile) は エミュレータコアモジュールを [src](src) と [include](include) に展開してから `pio run -t upload` で M5Stack 向けのビルドとアップロード（ファームウェア書き込み）を実行します。
>
> ファームウェア書き込みを実行したくない（ファームウェアのビルドのみしたい）場合は、次のように実行してください。
>
> ```
> make copy-core
> pio run
> ```
