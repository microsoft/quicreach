param (
    [Parameter(Mandatory = $false)]
    [ValidateSet("x86", "x64", "arm", "arm64", "arm64ec")]
    [string]$Arch = "x64",

    [Parameter(Mandatory = $false)]
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",

    [Parameter(Mandatory = $false)]
    [ValidateSet("schannel", "openssl")]
    [string]$Tls = "schannel",

    [Parameter(Mandatory = $false)]
    [ValidateSet("static", "shared")]
    [string]$Link = "static"
)

Set-StrictMode -Version 'Latest'
$PSDefaultParameterValues['*:ErrorAction'] = 'Stop'

# Root directory of the project.
$RootDir = Split-Path $PSScriptRoot -Parent
$BuildDir = Join-Path $RootDir "build"

if (!(Test-Path $BuildDir)) {
    New-Item -Path $BuildDir -ItemType Directory -Force | Out-Null
}

function Execute([String]$Name, [String]$Arguments) {
    Write-Debug "$Name $Arguments"
    $process = Start-Process $Name $Arguments -PassThru -NoNewWindow -WorkingDirectory $BuildDir
    $handle = $process.Handle # Magic work around. Don't remove this line.
    $process.WaitForExit();
    if ($process.ExitCode -ne 0) {
        Write-Error "$Name exited with status code $($process.ExitCode)"
    }
}

if ($env:ONEBRANCH_ARCH) {
    $Arch = $env:ONEBRANCH_ARCH
}
if ($env:ONEBRANCH_CONFIG) {
    $Config = $env:ONEBRANCH_CONFIG
}

$_Arch = $Arch
if ($_Arch -eq "x86") { $_Arch = "Win32" }

$Shared = "off"
if ($Link -ne "static") { $Shared = "on" }

Execute "cmake" "-G ""Visual Studio 17 2022"" -A $_Arch -DREACH_ARCH=$_Arch -DQUIC_TLS=$Tls -DQUIC_BUILD_SHARED=$Shared .."
