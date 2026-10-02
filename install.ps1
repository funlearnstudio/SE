$ErrorActionPreference = 'Stop'

$Repo = 'funlearnstudio/SE'
$FallbackVersion = '0.7.5'

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
$Version = ([string]$Version).Trim()
if ($Version.StartsWith('v')) { $Version = $Version.Substring(1) }
if ($Version -notmatch '^\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$') {
    throw "SE installer: invalid version '$Version'. Use 0.7.5 or v0.7.5."
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
    $ChecksumPath = Join-Path $TempDir 'SHA256SUMS.txt'
    Invoke-WebRequest -Uri "https://github.com/$Repo/releases/download/$Tag/SHA256SUMS.txt" -OutFile $ChecksumPath
    $Expected = $null
    foreach ($Line in Get-Content $ChecksumPath) {
        if ($Line -match '^([0-9A-Fa-f]{64})\s+\*?(.+)$' -and $Matches[2] -eq $Archive) {
            $Expected = $Matches[1]
            break
        }
    }
    if (-not $Expected) { throw "SE installer: no valid checksum for $Archive." }
    $Actual = (Get-FileHash -Path $ZipPath -Algorithm SHA256).Hash
    if ($Actual -ne $Expected) { throw "SE installer: checksum verification failed for $Archive." }
    Write-Host "Verified SHA-256 checksum for $Archive."
    Expand-Archive -Path $ZipPath -DestinationPath $TempDir -Force
    $PackageExe = Join-Path (Join-Path $TempDir $Asset) 'bin\se.exe'
    if (-not (Test-Path -Path $PackageExe -PathType Leaf)) {
        throw "SE installer: the release package does not contain bin\se.exe."
    }

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

    $env:Path = "$BinDir;$env:Path"
    Write-Host "Installed: $Launcher"
    Write-Host "Release: https://github.com/$Repo/releases/tag/$Tag"
    & $Exe --version
    Write-Host 'No CMake, Git, or C++ compiler is required for se run/check/test.'
    Write-Host 'Native `se build` still requires a system C++20 compiler.'
}
finally {
    if (Test-Path $TempDir) {
        Remove-Item -Recurse -Force $TempDir
    }
}
