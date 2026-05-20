# Script for WinScriptAdv content plugin
# PowerShell script test


$md5 = New-Object -TypeName System.Security.Cryptography.MD5CryptoServiceProvider
$hash = [System.BitConverter]::ToString($md5.ComputeHash([System.IO.File]::ReadAllBytes($filename)))
$content = $hash -replace '-', ''