# Quarothm

## 概要

学祭向けに作成した音ゲーです。

とはいえ、音ゲーそのものとして楽しめるような形で作成しています。

是非遊んでみてください〜

<!--TODO: Windows向けのビルドを早く準備する-->

また、このリポジトリはMITライセンスとして公開しています。

音ゲーを作ってみたい人などは参考にしていただいてもらって構いません。

(コメントアウトはかなり少なめで申し訳ない)

## 技術的な概要

- 言語: C++
- ライブラリ: Siv3D

ライブラリが非常に優秀だったので設計自体はかなり簡素にしました。

また、初めてECS設計を使用してみましたが音ゲーそのものにおいてはあまり恩恵を感じられませんでした。(私の実装が下手というのもあるかもしれない)

ただ、ゲーム全般として見るとUnityのように属性や今扱っているデータというのがはっきりするので非常にいい手法だと思っています。

あと、UIや譜面読み込みといった部分であまり設計を意識していないプログラムがありますが参考にする方は見なかったことにしてください...

## AI使用について

主に最初の基盤作成と雑務を任せました。

音ゲーの実装という美味しい部分は私がいただきました。

## ビルド
### 手順

WindowsではVisual Studioを用いてビルドを行ってください。

Linuxでは次のコマンドをプロジェクトルートで実行することでビルドできます。

```
$ cmake -S . -B build -DCMAKE_BUILD_TYPE:STRING=Release
$ cmake --build build
```

### コンパイラ

Visual StudioはMSVCのClangを想定しています。

LinuxではGCCを想定しています。

### 注意点

カレントディレクトリ下にassetsフォルダおよびsavesフォルダを必要とします。

また、自前ビルドの場合assetsフォルダにはデフォルトで楽曲の音源データが挿入されていません。

各自で公式サイトからダウンロードするなどしてください。(またその際、拡張子が合っていることをApp/Resource.rcから確認してください)

## プログラム構造

```
/
├── src
│   ├── Common.hpp
│   ├── ecs
│   │   ├── EcsWorld.hpp
│   │   └── RhythmComponents.hpp
│   ├── scenes
│   │   ├── InitializeScene.cpp
│   │   ├── InitializeScene.hpp
│   │   ├── PlayScene.cpp
│   │   ├── PlayScene.hpp
│   │   ├── ResultScene.cpp
│   │   ├── ResultScene.hpp
│   │   ├── SongSelectScene.cpp
│   │   ├── SongSelectScene.hpp
│   │   ├── TitleScene.cpp
│   │   └── TitleScene.hpp
│   ├── types
│   │   ├── AbstractScene.cpp
│   │   ├── AbstractScene.hpp
│   │   ├── ChartData.hpp
│   │   ├── ClearType.hpp
│   │   ├── Difficulty.hpp
│   │   ├── JudgementType.hpp
│   │   ├── NoteType.hpp
│   │   ├── ScoreRecorder.hpp
│   │   ├── SettingsStorage.hpp
│   │   ├── SongData.hpp
│   │   ├── SongSelection.hpp
│   │   └── SongsFolderSpecies.hpp
│   └── utilities
│       ├── FilePath.hpp
│       ├── ResponsiveValue.cpp
│       └── ResponsiveValue.hpp
└──Main.cpp
```

- Main.cpp
  - プログラムの開始及びループを行っています
- src/ecs/
  - ECS設計に基づいて作られたものが配置されています
- src/scenes/
  - 画面の遷移に関することや直接ゲームに関係するプログラムがあります
- src/types/
  - 基本的な型(メソッドを持たないことが多い)ものが入っています
- src/utilities/
  - 直接ゲームに関係するわけじゃない/それ単体で一つのライブラリになりそうな便利なものがあります

## その他

追記予定(この項目に限らない)
