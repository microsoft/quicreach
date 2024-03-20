param (
    [Parameter(Mandatory = $false)]
    [ValidateSet("x86", "x64", "arm", "arm64", "arm64ec")]
    [string]$Arch = "x64",

    [Parameter(Mandatory = $false)]
    [ValidateSet("schannel", "openssl")]
    [string]$Tls = "schannel",

    [Parameter(Mandatory = $false)]
    [ValidateSet("static", "shared")]
    [string]$Link = "static"
)

Set-StrictMode -Version 'Latest'
$PSDefaultParameterValues['*:ErrorAction'] = 'Stop'

if (!(Test-Path "./build")) {
    New-Item -Path "./build" -ItemType Directory -Force | Out-Null
}

function Execute([String]$Name, [String]$Arguments) {
    Write-Debug "$Name $Arguments"
    $process = Start-Process $Name $Arguments -PassThru -NoNewWindow -WorkingDirectory "./build"
    $handle = $process.Handle # Magic work around. Don't remove this line.
    $process.WaitForExit();
    if ($process.ExitCode -ne 0) {
        Write-Error "$Name exited with status code $($process.ExitCode)"
    }
}

$_Arch = $Arch
if ($_Arch -eq "x86") { $_Arch = "Win32" }

$Shared = "off"
if ($Link -ne "static") { $Shared = "on" }

Execute "cmake" "-G ""Visual Studio 17 2022"" -A $_Arch -DREACH_ARCH=$_Arch -DQUIC_TLS=$Tls -DQUIC_BUILD_SHARED=$Shared .."
