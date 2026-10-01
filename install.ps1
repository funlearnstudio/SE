$ErrorActionPreference = 'Stop'

$Repo = 'funlearnstudio/SE'
$FallbackVersion = '0.7.4'

if ($env:SE_VERSION) {
    $Version = $env:SE_VERSION
}
else {
    try {
        $Latest = Invoke-RestMethod -Uri "https://api.github.com/repos/$Repo/releases/latest" -Headers @{ 'User-Agent' = 'SE-Installer' }
        $Version = [string]$Latest.tag_name
        if ($Version.StartsWith('v')) { $Version = $Version.Substring(1) }
        if ([string]::IsNullOrWhiteSpace($Version)) { $Version = $FallbackVersion }
    }
    catch {
        $Version = $FallbackVersion
    }
}
$Tag = "v$Version"
$Asset = "se-$Version-windows-x64"
$Archive = "$Asset.zip"
$Url = "https://github.com/$Repo/releases/download/$Tag/$Archive"

$InstallRoot = if ($env:SE_INSTALL_ROOT) { $env:SE_INSTALL_ROOT } else { Join-Path $env:LOCALAPPDATA 'SE' }
$VersionDir = Join-Path $InstallRoot $Version
$BinDir = if ($env:SE_BIN_DIR) { $env:SE_BIN_DIR } else { Join-Path $env:LOCALAPPDATA 'SE\bin' }
$TempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("se-install-" + [guid]::NewGuid().ToString('N'))
$ZipPath = Join-Path $TempDir $Archive

Write-Host "Installing SE $Version for Windows x64..."
New-Item -ItemType Directory -Force -Path $TempDir | Out-Null

try {
    Invoke-WebRequest -Uri $Url -OutFile $ZipPath
    Expand-Archive -Path $ZipPath -DestinationPath $TempDir -Force

    New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null
    New-Item -ItemType Directory -Force -Path $BinDir | Out-Null

    if (Test-Path $VersionDir) {
        Remove-Item -Recurse -Force $VersionDir
    }
    Move-Item (Join-Path $TempDir $Asset) $VersionDir

    $Launcher = Join-Path $BinDir 'se.cmd'
    $Exe = Join-Path $VersionDir 'bin\se.exe'
    "@echo off`r`n`"$Exe`" %*`r`n" | Set-Content -Encoding ASCII $Launcher

    $UserPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    $Parts = @()
    if ($UserPath) { $Parts = $UserPath -split ';' }
    if ($Parts -notcontains $BinDir) {
        $NewPath = if ([string]::IsNullOrWhiteSpace($UserPath)) { $BinDir } else { "$UserPath;$BinDir" }
        [Environment]::SetEnvironmentVariable('Path', $NewPath, 'User')
        Write-Host "Added $BinDir to your user PATH. Open a new terminal after installation."
    }

    Write-Host "Installed: $Launcher"
    & $Exe --version
    Write-Host 'No CMake, Git, or C++ compiler is required for se run/check/test.'
    Write-Host 'Native `se build` still requires a system C++20 compiler.'
}
finally {
    if (Test-Path $TempDir) {
        Remove-Item -Recurse -Force $TempDir
    }
}
