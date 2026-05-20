# Script for WinScriptAdv content plugin
# PowerShell script test

$filename = "D:\Desktop\test.rpf"

$file = new-object System.IO.StreamReader($filename)
$content = $file.ReadLine()
$content1 = $file.ReadLine()
$content2 = Format-Hex -Path $filename -Count 4


[System.Windows.MessageBox]::Show($content2)

$file.close()