#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
; #Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.


;~ COM Object: Shell.Application
;~ Purpose: Retrieves file properties.
;~ System Requirements: General
;~ Documentation Link: GetDetailsOf, ParseName
;~ Other Links: Retrieving Extended File Properties
;~ Basic Code Example - this example demonstrates how to retrieve the properties of a specified file :
filepath := "C:\OneDrive\PPIE\PROPOSALS & TENDERS\2021\00. Design Assistance\18. Kranji Crescent (GNG)\Design & Qty\BLK B RF PTS X GL-D1 (300D, Load 10+3kPa) - Rev2.rpf"   ;set the target full file path

SplitPath, filepath , FileName, DirPath,

objShell :=   ComObjCreate("Shell.Application")
objFolder :=   objShell.NameSpace(DirPath)      ;set the directry path
objFolderItem :=   objFolder.ParseName(FileName)   ;set the file name

Loop 283   
   if   propertyitem :=   objFolder.GetDetailsOf(objFolderItem, A_Index)
      properties .=   A_Index . " " .  objFolder.GetDetailsOf(objFolder.Items, A_Index) . ":`t " . propertyitem . "`n"

msgbox %   properties