param (
    [Parameter(Mandatory = $false)]
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",

    [Parameter(Mandatory = $false)]
    [ValidateSet("x86", "x64", "arm", "arm64", "arm64ec")]
    [string]$Arch = "x64",

    [Parameter(Mandatory = $false)]
    [ValidateSet("schannel", "openssl")]
    [string]$Tls = "openssl",

    [Parameter(Mandatory = $false)]
    [ValidateSet("static", "shared")]
    [string]$Link = "static",

    [Parameter(Mandatory = $false)]
    [switch]$Clean = $false,

    [Parameter(Mandatory = $false)]
    [switch]$Install = $false,

    [Parameter(Mandatory = $false)]
    [switch]$BuildInstaller = $false
)

Set-StrictMode -Version 'Latest'
$PSDefaultParameterValues['*:ErrorAction'] = 'Stop'

if ($Clean) {
    if (Test-Path "./build") { Remove-Item "./build" -Recurse -Force | Out-Null }
}

if (!(Test-Path "./build")) {
    New-Item -Path "./build" -ItemType Directory -Force | Out-Null
}

$Shared = "off"
if ($Link -ne "static") { $Shared = "on" }

function Execute([String]$Name, [String]$Arguments) {
    Write-Debug "$Name $Arguments"
    $process = Start-Process $Name $Arguments -PassThru -NoNewWindow -WorkingDirectory "./build"
    $handle = $process.Handle # Magic work around. Don't remove this line.
    $process.WaitForExit();
    if ($process.ExitCode -ne 0) {
        Write-Error "$Name exited with status code $($process.ExitCode)"
    }
}

if ($IsWindows) {

    if ($Arch -eq "x86") { $Arch = "Win32" }
    Execute "cmake" "-G ""Visual Studio 17 2022"" -A $Arch -DQUIC_TLS=$Tls -DQUIC_BUILD_SHARED=$Shared .."
    Execute "cmake" "--build . --config Release"

    if ($BuildInstaller) {
        Execute 'C:/Program Files (x86)/WiX Toolset v3.11/bin/candle.exe' "../src/installer.wxs -o src/Release/quicreach.wixobj"
        Execute 'C:/Program Files (x86)/WiX Toolset v3.11/bin/light.exe' "-b src/Release -o src/Release/quicreach.msi src/Release/quicreach.wixobj"
    }

} else {

    if ($Config -eq "Release") { $Config = "RelWithDebInfo" }
    Execute "cmake" "-G ""Unix Makefiles"" -DCMAKE_BUILD_TYPE=$Config -DQUIC_TLS=$Tls -DQUIC_BUILD_SHARED=$Shared .."
    Execute "cmake" "--build ."
}

if ($Install -eq "x86") { Execute "cmake" "--install . --config Release" }
