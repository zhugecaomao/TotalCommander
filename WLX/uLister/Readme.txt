1. About
    uLister is a powerful document viewer which can open more than 500 file formats.
2. Supported formats
    Many popular word processing, spreadsheet, presentation, raster and vector image.
    Full list: http://www.oracle.com/technetwork/middleware/content-management/ds-oitfiles-133032.pdf
3. License
    This software are provided "as-is". No warranty provided.
    This plugin is freeware, but libraries from Oracle® is not, you must accept license http://www.oracle.com/technetwork/licenses/standard-license-152015.html before using.
4. Credits
    Outside In Viewer © 1991, 2011 Oracle.
    The software is based in part on the work of the Independent JPEG Group.
5. Configuration
    Use ulister.ini in plugin or  %COMMANDER_INI% or %APPDATA% dir.
    Parameters:
    "noloadtypes" allow disable plugin for some formats(see formats.txt). 
    "onlyloadtypes" allow enable plugin only for some formats. 
    "nopreviewtypes" allow disable create preview for some formats. 
    "onlypreviewtypes" allow enable create preview only for some formats. 
    "keepinmemory" allow keep Outside In Viewer library in memory, it reduce document load time but increase memory consumption (Default enable).
    "optionsdir" contain folder path for save settings(Environment variables now supported)
6. Installation
    Download Viewer Technology from: http://www.oracle.com/technetwork/middleware/content-management/downloads/oit-dl-otn-097435.html
    you need accept license and register. And copy "redist" folder to plugin folder.

    Documentation for this release is available online at http://docs.oracle.com/outsidein/854/index.html 

=========================================================
Copy from forum by ZGCM  (VC_2013_x64 is needed)

I did a (unofficial) recompile of uLister some time ago, which uses separate DLL dirs for 32- and 64-bit and therefore is able to work on a dual (32+64-bit) TC installation
http://wincmd.ru/files/9924355/wlx_uLister_4.0.0.1.rar
Just copy the OiT DLLs to the included sub-dir
redist32 for the 32-bit plugin
redist64 for the x64-Plugin

Additionally, if you use OiT DLLs before 8.5.3, my version will also work on Windows 2000, as I compiled with MSVC 2008.
(starting from 8.5.3 the OiT DLLs need the VC 2013 runtimes, and therefore requires XP SP3).

=========================================================