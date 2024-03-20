
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

Execute 'C:/Program Files (x86)/WiX Toolset v3.14/bin/candle.exe' "../src/installer.wxs -o src/Release/quicreach.wixobj"
Execute 'C:/Program Files (x86)/WiX Toolset v3.14/bin/light.exe' "-b src/Release -o src/Release/quicreach.msi src/Release/quicreach.wixobj"
