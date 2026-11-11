#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
sdk="$tmp/sdk"
um="$sdk/sdk/lib/um/x86_64"
mkdir -p "$um"
touch "$um/winhttp.lib" "$um/advapi32.lib"

project="$root/sample-app/ChatBot.Console/ChatBot.Console.vcxproj"
targets="$root/sample-app/msbuild/linux/"
for config in Debug Release; do
    output=$(dotnet msbuild "$project" -t:Compile,LinkApplication \
        "-p:Configuration=$config" -p:Platform=x64 \
        "-p:VCTargetsPath=$targets" "-p:SolutionDir=$tmp/" "-p:XwinRoot=$sdk" \
        -p:ClangCl=echo -p:LldLink=echo -v:normal -nologo)
    printf '%s\n' "$output" | grep -F "/imsvc\"$sdk/crt/include\"" >/dev/null
    printf '%s\n' "$output" | grep -F ' "src/main.cpp"' >/dev/null
    printf '%s\n' "$output" | grep -F '"winhttp.lib" "advapi32.lib"' >/dev/null
    if [ "$config" = Debug ]; then
        printf '%s\n' "$output" | grep -F ' /MDd ' >/dev/null
    else
        printf '%s\n' "$output" | grep -F ' /MD ' >/dev/null
    fi
done

rm "$um/winhttp.lib" "$um/advapi32.lib"
output=$(dotnet msbuild "$project" -t:LinkApplication \
    -p:Configuration=Release -p:Platform=x64 \
    "-p:VCTargetsPath=$targets" "-p:SolutionDir=$tmp/" "-p:XwinRoot=$sdk" \
    -p:LldLink=echo -v:normal -nologo)
printf '%s\n' "$output" | grep -F '"Winhttp.lib" "Advapi32.lib"' >/dev/null
echo "Linux MSBuild settings tests passed."
