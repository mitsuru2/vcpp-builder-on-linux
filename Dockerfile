# Build with: docker build --build-arg ACCEPT_MSVC_LICENSE=Y -t smallchatbot-linux .
# The image contains Microsoft SDK/CRT files, including non-redistributable debug files.
# Review Microsoft's license terms before building or distributing the image.
FROM mcr.microsoft.com/dotnet/sdk:10.0.403-noble@sha256:212fe4cb74627d5adf9817a4088906ae3edff1bd853bb27565d4b46731efe61e

# Pin the toolchain and vcpkg source used to build the Linux cross-compilation image.
ARG ACCEPT_MSVC_LICENSE
ARG LLVM_VERSION=1:19.1.1-1ubuntu1~24.04.2
ARG VCPKG_COMMIT=65886d79ccce671dc24ffbf9a348836beacb7f6f

# Require explicit acceptance of Microsoft's license before installing its SDK.
RUN test "$ACCEPT_MSVC_LICENSE" = Y || \
    (echo "Set ACCEPT_MSVC_LICENSE=Y after reviewing Microsoft's license terms." >&2; exit 1)

# Install the LLVM linker/compiler tools and base utilities required by the build.
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential ca-certificates curl git ninja-build pkg-config tar unzip zip \
    "clang-19=$LLVM_VERSION" "lld-19=$LLVM_VERSION" "llvm-19=$LLVM_VERSION" \
    && ln -s /usr/bin/clang-19 /usr/local/bin/clang-cl \
    && ln -s /usr/bin/lld-link-19 /usr/local/bin/lld-link \
    && ln -s /usr/bin/llvm-lib-19 /usr/local/bin/llvm-lib \
    && ln -s /usr/bin/llvm-rc-19 /usr/local/bin/llvm-rc \
    && rm -rf /var/lib/apt/lists/*

# Fetch and verify xwin, then install the Windows SDK and CRT needed for cross-compilation.
RUN set -eux; \
    curl --http1.1 -fsSL --retry 3 \
      https://github.com/Jake-Shadle/xwin/releases/download/0.10.0/xwin-0.10.0-x86_64-unknown-linux-musl.tar.gz \
      -o /tmp/xwin.tar.gz; \
    echo 'd870eb4b2f390878af6da1ccd3cf321d22fcb72720984853b4be732ae597fc88  /tmp/xwin.tar.gz' | sha256sum -c -; \
    mkdir /tmp/xwin; \
    tar -xzf /tmp/xwin.tar.gz -C /tmp/xwin --strip-components=1; \
    curl --http1.1 -fsSL --retry 3 \
      https://download.visualstudio.microsoft.com/download/pr/95995f57-8355-4ff5-8d90-a7e1669fb3b3/a788dfa3e6f50a33f7dee8396fd40d2946c69a776c0cfa5208f56d461fd1a825/VisualStudio.17.Release.chman \
      -o /tmp/channel.json; \
    echo '44a3e7dc44486c945abdc1fae97e7e22de4d05e80f03b00e9302ba4b72c3e0e3  /tmp/channel.json' | sha256sum -c -; \
    /tmp/xwin/xwin --accept-license --manifest /tmp/channel.json --arch x86_64 \
      --variant desktop --sdk-version 10.0.26100 --include-debug-runtime --temp \
      --timeout 120 splat --include-debug-libs --output /opt/xwin; \
    rm -rf /tmp/xwin /tmp/xwin.tar.gz /tmp/channel.json

# Install the project's pinned vcpkg dependencies for the Linux build.
COPY sample-app/vcpkg.json /tmp/vcpkg-project/vcpkg.json
RUN set -eux; \
    git init /tmp/vcpkg; \
    git -C /tmp/vcpkg remote add origin https://github.com/microsoft/vcpkg.git; \
    git -C /tmp/vcpkg fetch --depth 1 origin "$VCPKG_COMMIT"; \
    git -C /tmp/vcpkg checkout --detach FETCH_HEAD; \
    /tmp/vcpkg/bootstrap-vcpkg.sh -disableMetrics; \
    /tmp/vcpkg/vcpkg install --triplet x64-linux \
      --x-manifest-root=/tmp/vcpkg-project --x-install-root=/opt/vcpkg_installed; \
    rm -rf /tmp/vcpkg /tmp/vcpkg-project

# Copy the project into the image and expose the Linux-built dependency headers at the expected path.
WORKDIR /workspace
COPY . .
RUN mkdir -p sample-app/vcpkg_installed/x64-windows \
    && cp -a /opt/vcpkg_installed/x64-linux/include sample-app/vcpkg_installed/x64-windows/

# Build the solution using the Linux-specific MSBuild targets.
CMD ["dotnet", "msbuild", "sample-app/SmallChatBot.sln", "-m:1", "-p:Configuration=Release", "-p:Platform=x64", "-p:VCTargetsPath=/workspace/sample-app/msbuild/linux/"]
