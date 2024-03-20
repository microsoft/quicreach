param (
    [Parameter(Mandatory = $false)]
    [ValidateSet("x86", "x64", "arm", "arm64", "arm64ec")]
    [string]$Arch = "x64"
)

Set-StrictMode -Version 'Latest'
$PSDefaultParameterValues['*:ErrorAction'] = 'Stop'

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

Execute 'C:/Program Files (x86)/WiX Toolset v3.14/bin/candle.exe' "../src/installer.wxs -o bin/$($_Arch)fre/quicreach.wixobj"
Execute 'C:/Program Files (x86)/WiX Toolset v3.14/bin/light.exe' "-b bin/$($_Arch)fre/Release -o bin/$($_Arch)fre/quicreach.msi bin/$($_Arch)fre/quicreach.wixobj"
