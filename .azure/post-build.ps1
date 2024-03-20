param (
    [Parameter(Mandatory = $false)]
    [ValidateSet("x86", "x64", "arm", "arm64", "arm64ec")]
    [string]$Arch = "x64",

    [Parameter(Mandatory = $false)]
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release"
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

$platform = "$($Arch)fre"
if ($Config -eq "Debug") {
    $platform = "$($Arch)chk"
}

Execute 'C:/Program Files (x86)/WiX Toolset v3.14/bin/candle.exe' "../src/installer.wxs -o bin/$platform/quicreach.wixobj"
Execute 'C:/Program Files (x86)/WiX Toolset v3.14/bin/light.exe' "-b bin/$($Arch)fre/Release -o bin/$platform/quicreach.msi bin/$($Arch)fre/quicreach.wixobj"
