# Builds Release\GTA_LVIV_BETA_Setup.exe from Release\GTA LVIV BETA (run lviv-package.ps1 first).
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$root = Split-Path $here
$release = Join-Path $root 'Release'
$folder = Join-Path $release 'GTA LVIV BETA'
$obj = Join-Path $here 'obj'
$csc = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\Roslyn\csc.exe'
$fw = 'C:\Windows\Microsoft.NET\Framework64\v4.0.30319'
if (-not (Test-Path $folder)) { throw 'Run lviv-package.ps1 first' }
New-Item -ItemType Directory -Force $obj | Out-Null

$icon = Join-Path $root 'Client\launch\resource\lviv.ico'
$logo = Join-Path $root 'Shared\data\MTA San Andreas\MTA\cgui\lviv\logo.png'
$refs = 'System.dll', 'System.Core.dll', 'System.Drawing.dll', 'System.Windows.Forms.dll', 'System.IO.Compression.dll',
        'System.IO.Compression.FileSystem.dll', 'Microsoft.CSharp.dll' | ForEach-Object { "-r:$fw\$_" }
$common = @('-nologo', '-optimize+', '-target:winexe', '-platform:anycpu', "-win32icon:$icon", "-win32manifest:$here\app.manifest", "-resource:$logo,logo.png") + $refs

# 1. Uninstall.exe (no payload)
$uninst = Join-Path $obj 'Uninstall.exe'
& $csc @common -define:UNINSTALLER "-out:$uninst" "$here\Setup.cs"
if ($LASTEXITCODE) { throw 'Uninstall.exe build failed' }

# 2. payload.zip
$zip = Join-Path $obj 'payload.zip'
if (Test-Path $zip) { Remove-Item $zip }
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($folder, $zip, [IO.Compression.CompressionLevel]::Optimal, $false)

# 3. Setup.exe
$setup = Join-Path $release 'GTA_LVIV_BETA_Setup.exe'
& $csc @common "-resource:$zip,payload.zip" "-resource:$uninst,Uninstall.exe" "-out:$setup" "$here\Setup.cs"
if ($LASTEXITCODE) { throw 'Setup.exe build failed' }
Write-Host ("Installer: {0} ({1:N0} MB)" -f $setup, ((Get-Item $setup).Length / 1MB))
