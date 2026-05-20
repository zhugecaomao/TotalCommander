'filename = "C:\OneDrive\PPIE\MARKETING\1. Quotation\2022\2223 - Special Edu School at 4 Tampines Street 91\PT Demarcation - 07.05.2022.pdf"

Function getHash(filename)
    Set oShell = CreateObject("WScript.Shell")
    Set oShellExec = oShell.Exec("CertUtil -hashfile """ & filename & """ SHA256") 'Replace SHA256 with the algorithm you want, type "CertUtil -hashfile -?" in a command prompt for details
    If oShellExec.Status = 0 Then
        oShellExec.StdOut.ReadLine 'We don't need the first line, that's just an "explainer" you get in the console
        getHash = oShellExec.StdOut.ReadLine
    Else
        WScript.Echo "Failed with Error " & oShellExec.Status & vbNewLine & oShellExec.StdErr.ReadAll
    End If
End Function

Function getMD5(filename)
    Set oShell = CreateObject("WScript.Shell")
    Set oShellExec = oShell.Exec("CertUtil -hashfile """ & filename & """ MD5")
    If oShellExec.Status = 0 Then
        oShellExec.StdOut.ReadLine
        getMD5 = oShellExec.StdOut.ReadLine
    Else
        WScript.Echo "Failed with Error " & oShellExec.Status & vbNewLine & oShellExec.StdErr.ReadAll
    End If
End Function

Function MD5(filename)
    Dim oShell, sOut, sFileName, sCmd, oExec, strInput
    Set oShell = WScript.CreateObject("WScript.Shell")
    
    sFileName = """" & filename & """"
    
    sCmd = "cmd.exe /c certutil.exe -hashfile " & sFileName & " MD5" & _
    " | findstr /v " & chr(34) & "MD5 hash of" & "completed successfully" & Chr(34) & " | clip"

    oShell.Run sCmd, 0

    MD5 = CreateObject( "HTMLFile" ).parentWindow.clipboardData.getData( "Text" )
End Function

Content  = getMD5(filename)
Content1  = getHash(filename)

'MsgBox(content) 'For Debug