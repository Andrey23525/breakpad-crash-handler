# find-vsdevshell.ps1
#
# Locates Launch-VsDevShell.ps1 of an installed Visual Studio with the C++
# workload and prints its full path on a single line. Uses vswhere.exe when it
# is present, then falls back to scanning the default install roots. The caller
# dot-sources the result to enter the MSVC environment:
#
#     $vsDevShell = & ".\scripts\find-vsdevshell.ps1"
#     & $vsDevShell -Arch amd64

$ErrorActionPreference = 'Stop'

$scriptName = 'Launch-VsDevShell.ps1'

function Resolve-LaunchScript {
    param([string]$toolsDir)

    $candidate = Join-Path $toolsDir $scriptName
    if (Test-Path -LiteralPath $candidate -PathType Leaf) {
        return $candidate
    }
    return $null
}

# 1) vswhere.exe, installed together with Visual Studio.
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path -LiteralPath $vswhere -PathType Leaf) {
    $installPath = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath
    if ($installPath) {
        $found = Resolve-LaunchScript (Join-Path $installPath 'Common7\Tools')
        if ($found) {
            Write-Output $found
            exit 0
        }
    }
}

# 2) Scan <root>\<version>\<edition>\Common7\Tools across the default roots.
$roots = @(
    "${env:ProgramFiles}\Microsoft Visual Studio"
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio"
)
foreach ($root in $roots) {
    if (-not (Test-Path -LiteralPath $root)) {
        continue
    }
    $matches = Get-ChildItem -Path "$root\*\*\Common7\Tools\$scriptName" -File `
        -ErrorAction SilentlyContinue | Sort-Object FullName
    foreach ($match in $matches) {
        Write-Output $match.FullName
        exit 0
    }
}

Write-Error "Launch-VsDevShell.ps1 not found. Install Visual Studio with the 'Desktop development with C++' workload."
exit 1
