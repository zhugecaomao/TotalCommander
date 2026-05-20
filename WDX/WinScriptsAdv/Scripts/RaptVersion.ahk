#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.
#NoTrayIcon  ; No tray icon when AutoHotKey.exe is running with this script - because it blinks

; Script for WinScriptAdv content plugin

FileName := "C:\OneDrive\Apps\TotalCMD64\Plugins\Wdx\WinScriptsAdv\Scripts\test.rpf" ;For Debug

oFile := FileOpen(FileName, "r")
oFile.Position := 0
oFile.Length := 2
Hex := oFile.ReadUInt()

;MsgBox % Hex  ; For Debug: Default formatting.
;MsgBox % Format("{:08x}", Hex)  ; For Debug: 8-digit hex formatting without 0x prefix.

Content := SubStr(Hex, 1, 1) "." SubStr(Hex, 3, 1) "." Format("{:d}", SubStr(Hex, 4, 2))

;MsgBox % Content ;For Debug

oFile.Close()