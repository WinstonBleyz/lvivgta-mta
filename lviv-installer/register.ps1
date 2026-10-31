# GTA LVIV BETA: machine-wide registration that the MTA 1.6 loader expects from its installer
# (same as the official NSIS installer: HKLM key + ProgramData folder writable by Users).
# Must run elevated. Usage: register.ps1 -InstallDir "C:\Games\GTA LVIV BETA" [-Unregister]
param([Parameter(Mandatory)][string]$InstallDir, [switch]$Unregister)
$ErrorActionPreference = 'Stop'

# 32-bit client -> 32-bit registry view
$base = [Microsoft.Win32.RegistryKey]::OpenBaseKey('LocalMachine', 'Registry32')
$keyPath = 'SOFTWARE\GTA LVIV BETA'
$dataDir = Join-Path $env:ProgramData 'GTA LVIV BETA'

if ($Unregister) {
    $base.DeleteSubKeyTree($keyPath, $false)
    exit 0
}

$users = (New-Object Security.Principal.SecurityIdentifier 'S-1-5-32-545').Translate([Security.Principal.NTAccount])

$key = $base.CreateSubKey($keyPath)
# Users may write (the client stores its settings there, like official MTA)
$acl = $key.GetAccessControl()
$acl.AddAccessRule((New-Object Security.AccessControl.RegistryAccessRule($users, 'FullControl', 'ContainerInherit', 'None', 'Allow')))
$key.SetAccessControl($acl)
$version = $key.CreateSubKey('1.6')
$version.SetValue('Last Install Location', $InstallDir)
$key.CreateSubKey('Common').Close()
$version.Close(); $key.Close()

New-Item -ItemType Directory -Force $dataDir, (Join-Path $dataDir 'Common'), (Join-Path $dataDir '1.6') | Out-Null
$facl = Get-Acl $dataDir
$facl.AddAccessRule((New-Object Security.AccessControl.FileSystemAccessRule($users, 'Modify', 'ContainerInherit,ObjectInherit', 'None', 'Allow')))
Set-Acl $dataDir $facl
