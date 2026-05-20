'RAPT rpf file version detect by Liming
'
'Script for WinScriptAdv
' 
'This script returns the version of RAPT file

Dim oFSO, oTextStream
const bHex = True 
const nCharCnt = 6                                                      ' Read 6 byte of Hex code
'filename = "C:\OneDrive\test.rpf"                                      ' For Debug

Set oFSO = CreateObject("Scripting.FileSystemObject") 
Set oTextStream = oFSO.OpenTextFile(filename)                           ' WinScriptAdv sends to global variable 'filename'

If not oTextStream.AtEndOfStream Then 
  content_str = oTextStream.Read(nCharCnt)

  'MsgBox(Len(content_str))                                             'For Debug
  If bHex Then 
    For i = Len(content_str) To 3 Step -1                               'For i=1 To Len(content_str) 'Change to Reverse Order because Hex code sequence
      'Below 3 rows code is to add space between 2 byte of Hex
      'If hex_str <> "" Then 
      '  hex_str = hex_str & " " 
      'End If 

      hex_value = Hex(Asc(Mid(content_str,i,1))) 

      hex_str = hex_str & string(2 - len(hex_value), "0") 
      hex_str = hex_str & hex_value 
    Next

    RaptVerFullStr = CLng("&H" & hex_str)
    Content = Mid(RaptVerFullStr,1,1) & "." & Mid(RaptVerFullStr,3,1) & "." & Mid(RaptVerFullStr,4,2)
  Else
    Content = content_str
  End If
End If

'MsgBox(hex_str) 'For Debug
'MsgBox(RaptVerFullStr)                                                 ' For Debug, Return content
'MsgBox(content) 'For Debug

oTextStream.Close
Set oTextStream = Nothing 
Set oFSO = Nothing