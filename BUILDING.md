# SmallChatBot のビルド

SmallChatBot は Windows 向けの C++17 アプリケーションで、Visual Studio 2026
Community（C++ によるデスクトップ開発）と Windows 10 SDK でビルドします。

vcpkg をインストールし、`VCPKG_ROOT` にそのインストールディレクトリを設定します。
`sample-app` から `vcpkg install --triplet x64-windows` を実行して
`nlohmann-json` を復元し、`sample-app/SmallChatBot.sln` を開いて `x64` と
`Debug` または `Release` を選択し、Visual Studio でソリューションをビルドします。

## テキストエンコーディング

テキストファイルは BOM なしの UTF-8 を使用します。ASCII のみのファイルも有効です。
ソリューションは C++ コンパイラに `/utf-8` を渡し、ソース文字セットと実行文字セットの
両方を UTF-8 に設定します。`.editorconfig` はエディターの UTF-8 を設定し、
`.gitattributes` はテキストファイルの改行を LF にします。

リポジトリのルートから、Windows PowerShell 5.1 または PowerShell 7 でチェッカーを
実行します。

```powershell
./scripts/Find-NonUtf8Files.ps1
```

チェッカーはスクリプトに記載されたテキストファイルの種類を検査し、生成ディレクトリ
（`bin`、`obj`、`out`、`vcpkg_installed`）、Visual Studio と Git のメタデータ、
シンボリックリンクをスキップします。BOM、不正な UTF-8、NUL バイトを報告し、違反または
読み取りエラーがある場合は 0 以外の終了コードで終了します。元のエンコーディングの特定や、
有効な UTF-8 だが既に文字化けしているテキストの検出はできません。そのようなファイルは
手動で確認して変換してください。

チェッカー自体のテストは `./scripts/Test-FindNonUtf8Files.ps1` で実行します。

## SmallChatBot の実行

コンソールアプリケーションは、環境変数が未設定の場合に次の既定値を使用します。
Azure OpenAI のリソースとデプロイメントに合わせて上書きするには、変数を設定します。

- `SMALLCHATBOT_AZURE_ENDPOINT`、既定値: `https://smallchatbot.openai.azure.com`
- `SMALLCHATBOT_AZURE_DEPLOYMENT`、既定値: `gpt-4o-mini`
- `SMALLCHATBOT_AZURE_API_VERSION`、省略可。既定値は `2024-10-21`

Azure OpenAI の API キーは、Windows 資格情報マネージャーに、ターゲット
`SmallChatBot/AzureOpenAI` の **汎用資格情報** として保存します。コンソールアプリは
起動時にその資格情報を読み取り、キーを出力やログに記録しません。資格情報または API キーが
ない場合、アプリはエラーで終了します。エンドポイントとデプロイメントの既定値は例であり、
チャットリクエストを成功させるには実際の Azure OpenAI リソースと一致させる必要があります。

HTTP エラーの場合、コンソールはステータスと、サービスが返した詳細を表示します。
404 の場合は、エンドポイント、デプロイメント名、API バージョンが Azure OpenAI
リソースと一致しているか確認してください。エラー詳細にはサービスからの情報が含まれる
場合があるため、ログを共有する前に内容を確認してください。

コンソールの入出力は Unicode に対応しています。リダイレクトされた標準入力は UTF-8
である必要があり、リダイレクトされた標準出力と標準エラーは UTF-8 で書き込まれます。

Visual Studio Developer PowerShell からサービスのテストを実行するには、vcpkg の
依存関係を復元した後、リポジトリのルートから次を実行します。

```powershell
cl /utf-8 /EHsc /std:c++17 /Isample-app/ChatBot.Core/include /Isample-app/ChatBot.Model/include `
  /Isample-app/vcpkg_installed/x64-windows/include `
  sample-app/tests/AzureOpenAIChatServiceTests.cpp `
  sample-app/ChatBot.Core/src/AzureOpenAIChatService.cpp `
  sample-app/ChatBot.Model/src/ChatMessage.cpp /Fe:AzureOpenAIChatServiceTests.exe
./AzureOpenAIChatServiceTests.exe
```

Azure OpenAI のリソースとモデルのデプロイメントは意図的に手動でプロビジョニングします。
このリポジトリにはインフラストラクチャのデプロイ用テンプレートやワークフローは
含まれません。GitHub Actions のワークフローは Windows 上で Visual Studio
ソリューションをビルドします。

## Linux コンテナでのクロスコンパイル

Linux コンテナは、既存の MSBuild ソリューションを Windows x64 向けにビルドします。
別の CMake ビルドは導入しません。Docker と、Windows/MSVC のライセンス条件が必要です。
イメージには、再配布不可の Debug ランタイムファイルを含む Windows SDK と CRT の
ファイルが含まれます。ビルドまたはイメージの共有の前に、Microsoft の条件を確認して
受諾してください。

リポジトリのルートから、Release 構成をビルドして実行します。

```sh
docker build --build-arg ACCEPT_MSVC_LICENSE=Y -t smallchatbot-linux .
docker run --rm -v "$PWD/artifacts:/workspace/sample-app/bin" smallchatbot-linux
```

GitHub Actions のワークフロー `.github/workflows/publish-docker.yml` は、ルートの
`Dockerfile` が `main` で変更された場合、または Actions タブから手動で開始された場合に、
イメージを `ghcr.io/mitsuru2/vcpp-builder-on-linux` に公開します。既定ブランチに対して
`main` と `latest` のタグを公開し、`GITHUB_TOKEN` を使用するため、レジストリ用の
シークレットは不要です。Microsoft のライセンス条件を確認して受諾した後、リポジトリの
Actions 変数 `ACCEPT_MSVC_LICENSE` を `Y` に設定してください。設定がない場合、Docker
ビルドは失敗します。イメージには、再配布不可の Debug ファイルを含む Microsoft の
SDK/CRT ファイルが含まれます。

ビルドでは、.NET SDK のベースイメージを Ubuntu 24.04 上の 10.0.403、LLVM を 19.1.1、
xwin を 0.10.0、Windows SDK を 10.0.26100 に固定し、vcpkg のベースラインを
`sample-app/vcpkg.json` に記録されたコミットに固定します。xwin は x64 の
デスクトップ向け SDK/CRT バリアントのみをインストールし、Debug ビルド用に Debug
ランタイムライブラリを含めます。イメージは、固定した vcpkg ベースラインを使って
マニフェストの依存関係を復元します。

代わりに Debug をビルドするには、イメージのコマンドを上書きします。

```sh
docker run --rm -v "$PWD/artifacts:/workspace/sample-app/bin" \
  smallchatbot-linux dotnet msbuild sample-app/SmallChatBot.sln -m:1 \
  -p:Configuration=Debug -p:Platform=x64 \
  -p:VCTargetsPath=/workspace/sample-app/msbuild/linux/
```

生成される `.exe` ファイルは Windows x64 バイナリであり、Linux コンテナ内で直接実行
することはできません。アプリケーションのテストは Windows で実行してください。
x64 デスクトップバリアントとサンプルの現在のプロジェクト設定のみがサポート対象です。
それ以外の SDK バリアント、アーキテクチャ、MSBuild プロパティは、このクロスビルドでは
検証されていません。

## クロスコンパイル用ツールチェーンの選定と更新

Visual Studio ソリューションは、両プラットフォームでビルド定義であり続けます。
Visual Studio を変更しても Linux のツールチェーンは自動では更新されません。以下の
選択内容を確認し、まとめて検証してください。

### 現在の選択

| コンポーネント | 現在の選択と理由 | 更新箇所 |
| --- | --- | --- |
| Windows 上の Visual Studio と MSBuild | Windows CI は Visual Studio 2026（`[18.0,19.0)`）を使用します。3 つの `.vcxproj` は `Debug|x64` と `Release|x64` に `v145` を選択し、`Directory.Build.props` には既定値として `v143` があります。既定値だけでなく、各構成で **評価された** プロパティを確認してください。 | `.github/workflows/build.yml`、`sample-app/Directory.Build.props`、各 `.vcxproj` |
| Windows SDK | プロジェクトは特定の SDK ビルドではなく `10.0` を指定します。Linux での試行では、VS 17 チャネルの Windows 11 SDK `10.0.26100` を選択しました。サンプルに必要なヘッダーと x64 ライブラリが含まれていました。Windows 上の Visual Studio が使用する SDK を固定するものではありません。 | `Dockerfile`（`--sdk-version`）。Windows ビルドも固定する場合はプロジェクトの `WindowsTargetPlatformVersion` |
| CRT と STL | 固定した VS 17.14.45 チャネルでは、14.44.35211 の Debug ランタイムを含む 14.44.17.14 の CRT パッケージが選択されました。これらは VS 2022 のコンポーネントであり、Windows CI の v145 ツールセットではありません。このサンプルでテストしたものであり、v145 と同一であることは確認されていません。xwin は、与えられたマニフェスト内の最新の CRT を既定で選択します。Dockerfile は `--crt-version` を渡しません。 | `Dockerfile`（チャネル URL と SHA-256、必要に応じて `--crt-version`） |
| LLVM | Ubuntu の LLVM 18 は、選択した MSVC STL が Clang 19 以降を要求したため失敗しました。LLVM 19.1.1 ではサンプルのコンパイルとリンクに成功しました。clang、lld、llvm のパッケージバージョンは揃えてください。 | `Dockerfile`（`LLVM_VERSION`、パッケージ名、ツールのシンボリックリンク） |
| xwin | x64 デスクトップ向け SDK と CRT のファイル、Debug ライブラリ、ランタイム DLL の取得にバージョン 0.10.0 を使用しました。リリースアーカイブとチャネルマニフェストには、Dockerfile 内で別々の SHA-256 検証があります。 | `Dockerfile`（xwin の URL、アーカイブのハッシュ、フラグ） |
| .NET SDK | `dotnet msbuild` と Linux 用 C++ ターゲットには、Ubuntu 24.04 上の 10.0.403 を使用しました。ベースイメージはダイジェストで固定されています。Windows の Visual Studio の MSBuild バージョンとは異なります。 | `Dockerfile`（`FROM` のタグとダイジェスト） |
| vcpkg | `sample-app/vcpkg.json` はベースライン `65886d79ccce671dc24ffbf9a348836beacb7f6f` を固定し、`nlohmann-json` 3.12.0#2 を選択します。Docker はヘッダーのみの依存関係を `x64-linux` 用に復元し、`x64-windows` で期待されるパスにそのヘッダーをコピーします。Windows のバイナリ依存関係に対する一般的な方法ではありません。 | `sample-app/vcpkg.json` と `Dockerfile`（`VCPKG_COMMIT`）を併せて更新 |

Linux 用ターゲットは、Release に `/MD`、Debug に `/MDd` を使用し、SDK/CRT ヘッダーには
`/imsvc`、SDK ライブラリがある場合は小文字の名前を使用します。現在のサポート範囲は
サンプルに限定されます。特に、プロジェクトは `WholeProgramOptimization` と
`SDLCheck` を宣言していますが、Linux 用ターゲットはこれらの設定を変換しません。
リンクに成功しても、Visual Studio ビルドと同等であることの証明にはなりません。

### Visual Studio または Windows SDK を変更する場合

1. Windows で、インストール済みの Visual Studio のバージョンと C++ ワークロード、
   選択されているプラットフォームツールセット、SDK、ターゲットアーキテクチャ、構成、
   ランタイム、プロジェクト参照を確認します。`.sln`、`.vcxproj`、
   `Directory.Build.props` を調べ、Visual Studio のプロジェクトプロパティまたは
   MSBuild の診断ログで評価後の値を確認します。例:

   ```powershell
   msbuild sample-app/SmallChatBot.sln /m /p:Configuration=Release /p:Platform=x64 /v:diag /fl /flp:logfile=msbuild-release.log
   ```

   Debug でも繰り返します。`.github/workflows/build.yml` の Windows ランナーと
   `vs-version` の範囲を確認してください。Visual Studio のバージョンだけを変更しても、
   `v145`、SDK、Linux のチャネルは変わりません。Linux の CRT を新しいツールセットに
   合わせる必要があるか、別途テストしたバージョンで許容できるかを判断してください。
   バージョン番号だけから互換性を推測しないでください。
2. 目的の CRT に適した Microsoft Visual Studio チャネルを選択し、xwin がそれを
   サポートしていることを確認します。VS 17 リリースの場合は、まず小さなチャネル
   マニフェストのみを取得し、最終的な URL とハッシュを記録して、製品バージョンと
   参照先のパッケージマニフェストを確認します。

   ```sh
   curl --http1.1 -fsSL -o channel.json \
     -w 'effective URL: %{url_effective}\n' https://aka.ms/vs/17/release/channel
   sha256sum channel.json
   ```

   VS 17 の URL が以降のリリースを表すと仮定せず、対象の Visual Studio リリースの
   バージョン固有のチャネルを使用してください。ローカルの SHA-256 はダウンロードした
   バイト列を記録するだけで、真正性を証明するものではありません。固定する前に、信頼できる
   Microsoft のメタデータで出所と完全性を確認してください。最初の調査では、参照先の
   VS パッケージマニフェストに宣言されたサイズとハッシュが、Microsoft から返された
   バイト列と一致 **しません** でした。更新したマニフェストを採用する前にこのような
   不一致を調査し、期待するハッシュを黙って置き換えないでください。
3. SDK ペイロードをダウンロードする前に、検証済みの xwin バイナリで候補の選択内容を
   確認します。まず Microsoft のライセンスのプロンプトを確認してください。条件を受諾した
   後の非対話式の例は次のとおりです。

   ```sh
   xwin --accept-license --temp --manifest channel.json \
     --arch x86_64 --variant desktop --sdk-version 10.0.26100 \
     --include-debug-runtime list
   ```

   SDK バージョンは、そのマニフェストに一覧表示された候補に置き換えてください。
   xwin 0.10.0 では、バージョンキーは `10.0.26100.0` ではなく `10.0.26100` です。
   SDK バージョンを Windows プロジェクトの要件と比較し、選択された CRT ヘッダー、
   x64 ライブラリ、Debug ランタイムのバージョンを確認します。`desktop` を指定しても、
   共有ヘッダーに必要な Store ラベル付きパッケージが一覧に含まれる場合があります。
   関連する CAB ファイルは後で MSI ファイルから検出されるため、この一覧は完全な
   ダウンロードサイズの見積もりではありません。製品マニフェストもダウンロードされる
   場合があります。進める前に、空きディスク容量と一時キャッシュの場所を確認してください。
4. 選択した CRT/STL がサポートする LLVM バージョンを選び、最新のパッケージが互換だと
   仮定せず、実際のコンパイルで確認します。STL がコンパイラを拒否した場合は、診断と
   STL のバージョンチェックを調べます。選択した Ubuntu リリースが一致する clang、lld、
   llvm のバージョンを提供することを確認し、ベースイメージを変更する場合はタグ **と**
   ダイジェストを更新して、パッケージバージョンとバイナリのパスを再確認してください。
   コンパイルやリンクの失敗を診断する際は、`/imsvc` のヘッダー順序、リソースヘッダー、
   ライブラリ名の大文字小文字、SDK のレイアウトに留意してください。
5. xwin が変わる場合は xwin のリリース URL と信頼できるチェックサム、バージョン固有の
   チャネル URL と検証済みのチャネル SHA-256、SDK バージョン、LLVM のパッケージ名と
   バージョンを、Dockerfile でまとめて更新します。CRT を明示的に選択する場合は、
   選択した xwin のマニフェストが受け付ける正確な `--crt-version` キーを使用し、
   一覧の出力で確認してください。表示されるパッケージバージョンが正しいキーだと
   仮定しないでください。現在の `--include-debug-runtime` と `--include-debug-libs` は
   目的が異なり、テスト済みの Debug 構成では両方が必要です。特に、再配布不可の Debug
   ファイルを含むイメージを配布する前に、SDK/CRT のライセンスを確認してください。
6. 依存関係を更新する場合は、目的のポートバージョンを含むベースラインの vcpkg コミットを
   選択します。`builtin-baseline` と `VCPKG_COMMIT` の両方をそのコミットに更新し、
   選択されたポートとパッチを確認して、Windows と Linux で復元してください。依存関係が
   ヘッダーのみでなくなった場合は、Linux のヘッダーをコピーしてバイナリ互換性を仮定する
   のではなく、Windows 向けの vcpkg トリプレットを実装して検証してください。

### 更新の検証

* Linux ターゲット設定には `sh scripts/Test-LinuxMsbuild.sh` を実行し、エンコーディング
  チェッカーのテストは Windows PowerShell 5.1 と PowerShell 7 で実行します。
  Linux 設定のテストはコマンドのスタブを使用するもので、コンパイラのテストではありません。
* 更新した Docker イメージをビルドし、
  [Linux コンテナでのクロスコンパイル](#linux-コンテナでのクロスコンパイル)
  のコマンドで `Release|x64` と `Debug|x64` の両方を実行します。3 つのプロジェクトが
  すべてコンパイルされ、2 つの静的ライブラリと実行ファイルが生成され、実行ファイルが
  PE/COFF x64 であることを確認します。`llvm-readobj --coff-imports` でインポートを調べ、
  WinHTTP と Advapi32、および想定される Release CRT または Debug CRT/UCRT を確認して
  ください。Debug のインポートライブラリとランタイム DLL が実際に存在することも確認
  します。
* 意図した Visual Studio のバージョンと SDK で、Windows 上の既存のテストをビルドして
  実行します。Windows CI は現在 Release をビルドし、リダイレクトされた Unicode テストを
  実行します。ツールチェーンを変更する場合は、Debug も明示的に検証してください。
  クロスリンクされた実行ファイルは Linux 上で直接実行できません。
* ビルドの成否だけでなく、構成ごとのオプションとプロジェクト参照を比較します。
  ビルドが同等だと主張する前に、サポートされていないプロパティを修正するか、明示的に
  文書化してください。リソースを導入する場合は、`llvm-rc` と、結果のリソースのリンクも
  検証してください。

選択したバージョン、チャネルの URL とハッシュ、互換性、テスト結果を変更内容に記録して
ください。試行には使い捨てのキャッシュを使い、試行のために作成した成果物のみを削除し、
ディスク容量が限られている場合は複数の SDK をダウンロードしないでください。
