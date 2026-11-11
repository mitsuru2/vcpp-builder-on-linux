param(
    [string] $Root = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'
$encoding = New-Object System.Text.UTF8Encoding($false, $true)
$extensions = @(
    '.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx', '.inl',
    '.rc', '.rc2', '.sln', '.slnx', '.vcxproj', '.filters', '.props',
    '.targets', '.json', '.yml', '.yaml', '.xml', '.md', '.txt', '.ps1',
    '.sh', '.editorconfig', '.gitattributes', '.gitignore', '.dockerignore'
)
$names = @('LICENSE', 'Dockerfile', '.editorconfig', '.gitattributes', '.gitignore', '.dockerignore')
$excludedDirectories = @('.git', '.vs', 'bin', 'obj', 'out', 'vcpkg_installed')
$found = $false

function Find-Files([string] $Directory) {
    foreach ($item in Get-ChildItem -LiteralPath $Directory -Force) {
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            continue
        }
        if ($item.PSIsContainer) {
            if ($item.Name -notin $excludedDirectories) {
                Find-Files $item.FullName
            }
        } elseif ($item.Extension -in $extensions -or $item.Name -in $names) {
            $item
        }
    }
}

try {
    foreach ($file in Find-Files (Resolve-Path -LiteralPath $Root).Path) {
        $bytes = [IO.File]::ReadAllBytes($file.FullName)
        $reason = $null
        if ($bytes.Length -ge 4 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE -and $bytes[2] -eq 0 -and $bytes[3] -eq 0) {
            $reason = 'UTF-32 LE BOM'
        } elseif ($bytes.Length -ge 4 -and $bytes[0] -eq 0 -and $bytes[1] -eq 0 -and $bytes[2] -eq 0xFE -and $bytes[3] -eq 0xFF) {
            $reason = 'UTF-32 BE BOM'
        } elseif ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
            $reason = 'UTF-8 BOM'
        } elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) {
            $reason = 'UTF-16 LE BOM'
        } elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF) {
            $reason = 'UTF-16 BE BOM'
        } elseif ([Array]::IndexOf($bytes, [byte]0) -ge 0) {
            $reason = 'NUL byte in text file (possible UTF-16 or UTF-32 without BOM)'
        } else {
            try {
                $null = $encoding.GetString($bytes)
            } catch [System.Text.DecoderFallbackException] {
                $reason = "Invalid UTF-8 at byte $($_.Exception.Index)"
            } catch [System.Management.Automation.MethodInvocationException] {
                if ($_.Exception.InnerException -isnot [System.Text.DecoderFallbackException]) {
                    throw
                }
                $reason = "Invalid UTF-8 at byte $($_.Exception.InnerException.Index)"
            }
        }

        if ($reason) {
            Write-Output "$($file.FullName): $reason"
            $found = $true
        }
    }
} catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}

if ($found) {
    exit 1
}
exit 0
