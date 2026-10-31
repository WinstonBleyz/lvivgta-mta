# GTA LVIV BETA: compose the client release folder Release\GTA LVIV BETA (client only, no server, no GTA files)
# Mirrors utils/buildactions/compose_files.lua for the client part. Run after lviv-build.bat.
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$bin = Join-Path $root 'Bin'
$data = Join-Path $root 'Shared\data\MTA San Andreas'
$out = Join-Path $root 'Release\GTA LVIV BETA'

if (-not (Test-Path (Join-Path $bin 'GTA LVIV BETA.exe'))) { throw 'Build first: lviv-build.bat' }
if (Test-Path $out) { Remove-Item $out -Recurse -Force }
New-Item -ItemType Directory -Force $out | Out-Null

# Data files
foreach ($dir in 'MTA', 'skins', 'redist') {
    if (Test-Path (Join-Path $data $dir)) { Copy-Item (Join-Path $data $dir) $out -Recurse -Force }
}

# Compiled client binaries (exe/dll/pak/bin/dat), skipping server, tests and debug-only files
$skipDirs = '\\server\\', '\\tests\\', '\\lib\\', '\\MTA\\dumps\\', '\\MTA\\logs\\', '\\MTA\\config\\'
Get-ChildItem $bin -Recurse -File -Include *.exe, *.dll, *.pak, *.bin, *.dat, *.json | Where-Object {
    $rel = $_.FullName.Substring($bin.Length)
    -not ($skipDirs | Where-Object { $rel -match $_ }) -and $_.Name -notmatch '_d\.(dll|exe)$'
} | ForEach-Object {
    $dest = Join-Path $out $_.FullName.Substring($bin.Length + 1)
    New-Item -ItemType Directory -Force (Split-Path $dest) | Out-Null
    Copy-Item $_.FullName $dest -Force
}

# CEF resources (locales etc.) shipped as non-binary files
$cef = Join-Path $bin 'MTA\CEF'
if (Test-Path $cef) { Copy-Item $cef (Join-Path $out 'MTA') -Recurse -Force }

# Licences, setup helper, readme
Copy-Item (Join-Path $root 'LICENSE') (Join-Path $out 'LICENSE.txt')
Copy-Item (Join-Path $root 'lviv-installer\register.ps1') $out
Copy-Item (Join-Path $root 'lviv-installer\README_UA.txt') $out

$size = (Get-ChildItem $out -Recurse -File | Measure-Object Length -Sum).Sum / 1MB
Write-Host ("Release folder: {0} ({1:N0} MB)" -f $out, $size)
