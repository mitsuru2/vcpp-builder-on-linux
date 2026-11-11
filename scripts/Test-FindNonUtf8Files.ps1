$ErrorActionPreference = 'Stop'
$checker = Join-Path $PSScriptRoot 'Find-NonUtf8Files.ps1'
$shell = (Get-Process -Id $PID).Path
$root = Join-Path ([IO.Path]::GetTempPath()) ("encoding-test-" + [guid]::NewGuid().ToString('N'))

try {
    $null = New-Item -ItemType Directory -Path $root
    [IO.File]::WriteAllBytes((Join-Path $root 'ascii.cpp'), [byte[]](0x61, 0x0A))
    [IO.File]::WriteAllBytes((Join-Path $root 'utf8.h'), [byte[]](0xE6, 0x97, 0xA5))
    $output = & $shell -NoProfile -File $checker -Root $root 2>&1
    if ($LASTEXITCODE -ne 0 -or $output) {
        throw "Expected valid ASCII and UTF-8 to pass: $output"
    }

    $cases = @(
        @{ Name = 'utf8bom.cpp'; Bytes = [byte[]](0xEF, 0xBB, 0xBF, 0x61); Reason = 'UTF-8 BOM' },
        @{ Name = 'utf16.cpp'; Bytes = [byte[]](0xFF, 0xFE, 0x61, 0x00); Reason = 'UTF-16 LE BOM' },
        @{ Name = 'utf32.cpp'; Bytes = [byte[]](0xFF, 0xFE, 0x00, 0x00); Reason = 'UTF-32 LE BOM' },
        @{ Name = 'nobom.cpp'; Bytes = [byte[]](0x61, 0x00, 0x62, 0x00); Reason = 'NUL byte' },
        @{ Name = 'shiftjis.cpp'; Bytes = [byte[]](0x82, 0xA0); Reason = 'Invalid UTF-8' }
    )
    foreach ($case in $cases) {
        $path = Join-Path $root $case.Name
        [IO.File]::WriteAllBytes($path, $case.Bytes)
        $output = & $shell -NoProfile -File $checker -Root $root 2>&1
        if ($LASTEXITCODE -ne 1 -or "$output" -notlike "*$($case.Name)*$($case.Reason)*") {
            throw "Expected $($case.Name) to fail with $($case.Reason): $output"
        }
        Remove-Item -LiteralPath $path
    }

    $ignored = Join-Path $root 'vcpkg_installed'
    $null = New-Item -ItemType Directory -Path $ignored
    [IO.File]::WriteAllBytes((Join-Path $ignored 'ignored.cpp'), [byte[]](0xFF))
    $output = & $shell -NoProfile -File $checker -Root $root 2>&1
    if ($LASTEXITCODE -ne 0 -or $output) {
        throw "Expected generated dependencies to be excluded: $output"
    }

    $ErrorActionPreference = 'Continue'
    $output = & $shell -NoProfile -File $checker -Root (Join-Path $root 'missing') 2>&1
    $ErrorActionPreference = 'Stop'
    if ($LASTEXITCODE -eq 0) {
        throw 'Expected a missing root to fail.'
    }
    Write-Output 'Encoding checker tests passed.'
} finally {
    if (Test-Path -LiteralPath $root) {
        Remove-Item -LiteralPath $root -Recurse -Force
    }
}
