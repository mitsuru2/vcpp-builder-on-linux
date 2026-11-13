# Building SmallChatBot

SmallChatBot is a Windows C++17 application and is built with Visual Studio 2026
Community (Desktop development with C++) and the Windows 10 SDK.

Install vcpkg and set `VCPKG_ROOT` to its installation directory. From
`sample-app`, run `vcpkg install --triplet x64-windows` to restore
`nlohmann-json`, then open `sample-app/SmallChatBot.sln`, select `x64` and
`Debug` or `Release`, and build the solution in Visual Studio.

## Text encoding

Text files use UTF-8 without a BOM; ASCII-only files are also valid. The
solution passes `/utf-8` to the C++ compiler, setting both the source and
execution character sets to UTF-8. `.editorconfig` configures UTF-8 for editors,
and `.gitattributes` uses LF line endings for text files.

From the repository root, run the checker with Windows PowerShell 5.1 or
PowerShell 7:

```powershell
./scripts/Find-NonUtf8Files.ps1
```

The checker examines the text file types listed in the script and skips
generated directories (`bin`, `obj`, `out`, and `vcpkg_installed`), Visual
Studio and Git metadata, and symbolic links. It reports BOMs, invalid UTF-8,
and NUL bytes, and exits with a nonzero status on violations or read errors.
It cannot identify the original encoding or detect text that is valid UTF-8
but already corrupted. Review and convert such files manually.

Run `./scripts/Test-FindNonUtf8Files.ps1` to exercise the checker.

## Running SmallChatBot

The console application uses these defaults if the environment variables are
unset. Set the variables to override them with your Azure OpenAI resource and
deployment:

- `SMALLCHATBOT_AZURE_ENDPOINT`, default: `https://smallchatbot.openai.azure.com`
- `SMALLCHATBOT_AZURE_DEPLOYMENT`, default: `gpt-4o-mini`
- `SMALLCHATBOT_AZURE_API_VERSION`, optional; defaults to `2024-10-21`

Store the Azure OpenAI API key in Windows Credential Manager as a **Generic
Credential** with target `SmallChatBot/AzureOpenAI`. The console app reads that
credential at startup and never prints or logs the key. If the credential or
API key is missing, the app exits with an error. The endpoint and deployment
defaults are examples and must match an actual Azure OpenAI resource for chat
requests to succeed.

For HTTP errors, the console displays the status and any details returned by
the service. For a 404, check that the endpoint, deployment name, and API
version match your Azure OpenAI resource. Error details may contain information
from the service; review them before sharing logs.

Console input and output support Unicode. Redirected standard input must be
UTF-8; redirected standard output and standard error are written as UTF-8.

To run the service tests from a Visual Studio Developer PowerShell, after
restoring the vcpkg dependencies, run the following from the repository root:

```powershell
cl /utf-8 /EHsc /std:c++17 /Isample-app/ChatBot.Core/include /Isample-app/ChatBot.Model/include `
  /Isample-app/vcpkg_installed/x64-windows/include `
  sample-app/tests/AzureOpenAIChatServiceTests.cpp `
  sample-app/ChatBot.Core/src/AzureOpenAIChatService.cpp `
  sample-app/ChatBot.Model/src/ChatMessage.cpp /Fe:AzureOpenAIChatServiceTests.exe
./AzureOpenAIChatServiceTests.exe
```

The Azure OpenAI resource and model deployment are intentionally provisioned
manually; this repository does not contain infrastructure deployment templates
or workflows. The GitHub Actions workflow builds the Visual Studio solution on
Windows.

## Linux container cross-compilation

The Linux container builds the existing MSBuild solution for Windows x64; it
does not introduce a separate CMake build. Docker and the Windows/MSVC license
terms are required. The image contains Windows SDK and CRT files, including
non-redistributable Debug runtime files. Review and accept Microsoft's terms
before building or sharing the image.

From the repository root, build and run the Release configuration:

```sh
docker build --build-arg ACCEPT_MSVC_LICENSE=Y -t smallchatbot-linux .
docker run --rm -v "$PWD/artifacts:/workspace/sample-app/bin" smallchatbot-linux
```

The GitHub Actions workflow `.github/workflows/publish-docker.yml` publishes
the image to `ghcr.io/mitsuru2/vcpp-builder-on-linux` when the root
`Dockerfile` changes on `main`, or when started manually from the Actions tab.
It publishes `main` and `latest` tags for the default branch and uses
`GITHUB_TOKEN`, so no registry secret is required. Set the repository Actions
variable `ACCEPT_MSVC_LICENSE` to `Y` after reviewing and accepting Microsoft's
license terms; without it, the Docker build fails. The image includes Microsoft
SDK/CRT files, including non-redistributable Debug files.

The build pins the .NET SDK base image to 10.0.403 on Ubuntu 24.04, LLVM to
19.1.1, xwin to 0.10.0, Windows SDK to 10.0.26100, and the vcpkg baseline to
the commit recorded in `sample-app/vcpkg.json`. xwin installs only the x64
desktop SDK/CRT variant and includes Debug runtime libraries for Debug builds.
The image restores the manifest dependency using the pinned vcpkg baseline.

To build Debug instead, override the image command:

```sh
docker run --rm -v "$PWD/artifacts:/workspace/sample-app/bin" \
  smallchatbot-linux dotnet msbuild sample-app/SmallChatBot.sln -m:1 \
  -p:Configuration=Debug -p:Platform=x64 \
  -p:VCTargetsPath=/workspace/sample-app/msbuild/linux/
```

The resulting `.exe` files are Windows x64 binaries and cannot be run directly
in the Linux container. Run application tests on Windows. The x64 desktop
variant and the sample's current project settings are supported; other SDK
variants, architectures, and MSBuild properties are not validated by this
cross-build.

## Selecting and updating the cross-compilation toolchain

The Visual Studio solution remains the build definition on both platforms.
Changing Visual Studio does not automatically update the Linux toolchain:
review the selections below and validate them together.

### Current selections

| Component | Current selection and reason | Where to update |
| --- | --- | --- |
| Visual Studio and MSBuild on Windows | Windows CI uses Visual Studio 2026 (`[18.0,19.0)`). The three `.vcxproj` files select `v145` for `Debug|x64` and `Release|x64`; `Directory.Build.props` contains a `v143` default. Check the **evaluated** property for each configuration, not just the default. | `.github/workflows/build.yml`, `sample-app/Directory.Build.props`, and the `.vcxproj` files |
| Windows SDK | Projects specify `10.0`, not a particular SDK build. For the Linux trial we chose Windows 11 SDK `10.0.26100` from the VS 17 channel; it provided the headers and x64 libraries needed by the sample. This does not pin the SDK used by Visual Studio on Windows. | `Dockerfile` (`--sdk-version`); project `WindowsTargetPlatformVersion` if the Windows build must also be pinned |
| CRT and STL | The fixed VS 17.14.45 channel selected the 14.44.17.14 CRT packages, including the 14.44.35211 Debug runtime. These are VS 2022 components, not the Windows CI's v145 toolset. They were tested for this sample, not established as identical to v145. xwin defaults to the latest CRT **in the supplied manifest**; the Dockerfile does not pass `--crt-version`. | `Dockerfile` (channel URL and SHA-256, optionally `--crt-version`) |
| LLVM | Ubuntu LLVM 18 failed because the selected MSVC STL required Clang 19 or newer. LLVM 19.1.1 compiled and linked the sample; use matching clang, lld and llvm package versions. | `Dockerfile` (`LLVM_VERSION`, package names and tool symlinks) |
| xwin | Version 0.10.0 was used to obtain x64 desktop SDK and CRT files, with Debug libraries and runtime DLLs. The release archive and the channel manifest have separate SHA-256 checks in the Dockerfile. | `Dockerfile` (xwin URL, archive hash and flags) |
| .NET SDK | 10.0.403 on Ubuntu 24.04 was used for `dotnet msbuild` and the Linux C++ targets; the base image is pinned by digest. It is not the Windows Visual Studio MSBuild version. | `Dockerfile` (`FROM` tag and digest) |
| vcpkg | `sample-app/vcpkg.json` pins baseline `65886d79ccce671dc24ffbf9a348836beacb7f6f`, selecting `nlohmann-json` 3.12.0#2. Docker restores the header-only dependency for `x64-linux` and copies its headers into the path expected for `x64-windows`. This is not a general method for Windows binary dependencies. | `sample-app/vcpkg.json` and `Dockerfile` (`VCPKG_COMMIT`) together |

The Linux targets use `/MD` for Release and `/MDd` for Debug, `/imsvc` for
SDK/CRT headers and lowercase SDK library names when present. Their current
support is limited to the sample. In particular, the project declares
`WholeProgramOptimization` and `SDLCheck`, but the Linux targets do not
translate these settings; successful linking does not prove equivalence to a
Visual Studio build.

### When Visual Studio or the Windows SDK changes

1. On Windows, check the installed Visual Studio version and C++ workload,
   selected platform toolset, SDK, target architecture, configuration,
   runtime and project references. Inspect `.sln`, `.vcxproj` and
   `Directory.Build.props`, then use Visual Studio project properties or an
   MSBuild diagnostic log to verify evaluated values. For example:

   ```powershell
   msbuild sample-app/SmallChatBot.sln /m /p:Configuration=Release /p:Platform=x64 /v:diag /fl /flp:logfile=msbuild-release.log
   ```

   Repeat for Debug. Check the Windows runner and `vs-version` range in
   `.github/workflows/build.yml`; changing the Visual Studio version alone
   does not change `v145`, the SDK, or the Linux channel. Decide whether the
   Linux CRT must match the new toolset or whether a separately tested version
   is acceptable. Do not infer compatibility from version numbers alone.
2. Select a Microsoft Visual Studio channel appropriate to the desired CRT
   and confirm that xwin supports it. For VS 17 release, fetch only the small
   channel manifest first, record the final URL and hash, and inspect its
   product version and referenced package manifest:

   ```sh
   curl --http1.1 -fsSL -o channel.json \
     -w 'effective URL: %{url_effective}\n' https://aka.ms/vs/17/release/channel
   sha256sum channel.json
   ```

   Use the version-specific channel for the target Visual Studio release
   rather than assuming the VS 17 URL represents later releases. A local
   SHA-256 records the exact downloaded bytes; it does not by itself establish
   authenticity. Verify the origin and integrity against trusted Microsoft
   metadata before pinning it. During the original investigation, the
   referenced VS package manifest's declared size and hash did **not** match
   the bytes returned by Microsoft. Investigate such a mismatch before
   adopting an updated manifest; do not silently replace the expected hash.
3. Before downloading SDK payloads, use a verified xwin binary to inspect the
   candidate selection. Review its Microsoft license prompt first; after
   accepting the terms, a non-interactive example is:

   ```sh
   xwin --accept-license --temp --manifest channel.json \
     --arch x86_64 --variant desktop --sdk-version 10.0.26100 \
     --include-debug-runtime list
   ```

   Replace the SDK version with the candidate listed in that manifest. For
   xwin 0.10.0, the version key is `10.0.26100`, not `10.0.26100.0`.
   Compare the SDK version against the Windows project's requirement and
   inspect the selected CRT headers, x64 libraries, and Debug runtime versions.
   The list can include Store-labelled packages required for shared headers
   even with `desktop`; it is not a complete download-size estimate because
   associated CAB files are discovered from the MSI files later. It may also
   download the product manifest. Check free disk space and temporary cache
   locations before proceeding.
4. Choose the LLVM version supported by the selected CRT/STL, and verify
   actual compilation rather than assuming the newest package is compatible.
   If the STL rejects a compiler, inspect the diagnostic and STL's version
   checks. Check that the selected Ubuntu release provides matching versions
   of clang, lld and llvm; if the base image changes, update its tag **and**
   digest and re-check the package versions and binary paths. Keep `/imsvc`
   header ordering, resource headers, library name casing and SDK layout in
   mind when diagnosing compilation or linking failures.
5. Update the Dockerfile's xwin release URL and trusted checksum if xwin
   changes, the versioned channel URL and verified channel SHA-256, SDK
   version, and LLVM package names/version together. If explicitly selecting
   a CRT, use an exact `--crt-version` key accepted by the selected xwin
   manifest and confirm the list output; do not assume the displayed package
   version is the correct key. The current `--include-debug-runtime` and
   `--include-debug-libs` serve different purposes and are both needed for
   the tested Debug configuration. Check SDK/CRT licensing, especially before
   distributing an image containing non-redistributable Debug files.
6. If dependencies are updated, choose a vcpkg commit whose baseline contains
   the desired port versions. Update both `builtin-baseline` and
   `VCPKG_COMMIT` to that commit, inspect the selected port and patches, and
   restore on Windows and Linux. If a dependency is no longer header-only,
   implement and validate a Windows-targeted vcpkg triplet instead of copying
   Linux headers and assuming binary compatibility.

### Validate the update

* Run `sh scripts/Test-LinuxMsbuild.sh` for the Linux target settings and run
  the encoding checker tests with Windows PowerShell 5.1 and PowerShell 7.
  The Linux settings test uses command stubs; it is not a compiler test.
* Build the updated Docker image and run both `Release|x64` and `Debug|x64`
  using the commands in [Linux container cross-compilation](#linux-container-cross-compilation).
  Check that all three projects compile, both static libraries and the
  executable are produced, and the executable is PE/COFF x64. Inspect imports
  with `llvm-readobj --coff-imports`: confirm WinHTTP and Advapi32 plus the
  expected Release CRT or Debug CRT/UCRT. Verify that the Debug import
  libraries and runtime DLLs actually exist.
* Build and run the existing tests on Windows with the intended Visual Studio
  version and SDK. The Windows CI currently builds Release and runs its
  redirected Unicode test; also explicitly validate Debug when changing the
  toolchain. A cross-linked executable cannot be run directly on Linux.
* Compare configuration-specific options and project references, not just
  build success. Fix or explicitly document unsupported properties before
  claiming equivalent builds. If resources are introduced, also validate
  `llvm-rc` and the resulting resource link.

Record the selected versions, channel URL/hash, compatibility and test results
in the change. Use a disposable cache for trials, remove only artifacts created
for the trial, and avoid downloading multiple SDKs when disk space is limited.
