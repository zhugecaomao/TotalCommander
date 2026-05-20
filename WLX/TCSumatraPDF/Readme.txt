TC SumatraPDF Plugin 1.20

The TCSumatraPDF plugin is designed to view AI, AZW, AZW3, CB7, CBT, CBR, CBZ,
CHM, DJV, DJVU, EPUB, FB2, FB2Z, FBZ, KF8, MOBI, PDB, PDF, PRC, TCR, XPS, ZFB2
using SumatraPDF.
The plugin is written for a specific unofficial version of SumatraPDF 3.1.1.10417,
in which has additional functions for processing PDF files. But it works with
any versions and bit depths of SumatraPDF.
The main difference between the plug-in and analogues is the correct operation
of the Esc key for SumatraPDF x86 to Total Commander x64 and double use of '4',
'N' and 'P' which are essential for both TC Lister control and SumatraPDF
controls.
In the main plugin window, the '4' key works like in TC Lister and serves to
iteration of plugins for viewing. In the SumatraPDF tabs, the '4' key is used to
document navigation.
When viewing a single document, the 'N' and 'P' keys work as in SumatraPDF and
are used to navigate through the document. If run to view group
selected files, the 'N' and 'P' keys work as in TC Lister and serve to
scroll through the list of files.
ATTENTION: For the plugin to work properly, you must change the value of the
parameter UseTabs = true to UseTabs = false in SumatraPDF-settings.txt and
parameter RememberOpenedFiles = true to RememberOpenedFiles = false.

Installation.
The plugin is installed using standard Total Commander tools.
Automatic installation is supported.
The plugin can use portable or system-installed SumatraPDF.
To use the portable version of SumatraPDF in the TCSumatraPDF plugin folder
subfolder \SumatraPDF is automatically created.
SumatraPDF program https://www.
sumatrapdfreader.org/download-free-pdf-viewer
unzip to \SumatraPDF subfolder. Files from the unpacked archive, in the name
which specifies the version of a program like SumatraPDF-3.4.6-32.exe or
SumatraPDF-3.4.6-64.exe, must be renamed to SumatraPDF and SumatraPDF64
respectively. If you want to use the full file name,
you need to write the path to them in the sumatrapdf.ini file. Its description
is given below.
The plugin is ready to go. You can start viewing files by pressing F3 or
Ctrl+Q.
For the installed SumatraPDF, you must specify the paths to its executable files
SumatraPDF.exe in the sumatrapdf.ini file, which is automatically created in the
folder plugin after the first use.
If during the first installation of the plugin you need to immediately set the
path to SumatraPDF executable files, create the sumatrapdf.ini file manually, or
rename and edit the sumatrapdf.sample.ini file.
In the future, when updating the plugin, sumatrapdf.ini is not overwritten and
stores path settings for SumatraPDF. The sumatrapdf.sample.ini file lists
possible paths to SumatraPDF.
When installing SumatraPDF to protected folders, you need to make sure that
access to SumatraPDF files are open.
By default sumatrapdf.ini creates an empty [Options] section for using the
portable version of SumatraPDF.
In this case, the SumatraPDF files are searched in the \SumatraPDF subfolder of
the plugin.
Even for the portable version of SumatraPDF, you must specify the path to the
executable file if the bit depths of SumatraPDF and Total Commander do not
match.
The SumatraPDF settings file SumatraPDF-settings.txt is automatically created
when first use of the plugin.
The configuration file is common for both bit depths SumatraPDF.
The user can change the settings at his own discretion SumatraPDF, but the
UseTabs option, which defaults to true, should be changed to UseTabs = false.
Otherwise, normal operation of the plugin is not guaranteed.
Since you cannot use the UseTabs = true option in the plugin, for convenience
using the plugin it is possible to use different configuration files
SumatraPDF-settings.txt for each view mode. And a separate configuration file
for normal launch mode of SumatraPDF. Settings files SumatraPDF-settings.txt
for Lister and QuickView modes must be in separate folders, the path to
which is written in the variables DirIniL=, DirIniQ= of the plugin configuration
file sumatrapdf.ini. By default, the variables DirIniL=, DirIniQ= are not
created and the main file SumatraPDF-settings.txt is used for all modes.
When setting the path to SumatraPDF, it is allowed to use variables like
%COMMANDER_PATH%.

ATTENTION: For normal operation of the plugin it is necessary in
SumatraPDF-settings.txt change the value of the parameters UseTabs = true to
UseTabs = false and RememberOpenedFiles = true to RememberOpenedFiles = false.

Distribution conditions.
This software is distributed on an "AS IS"
basis and is free for non-commercial use. The author is not
gives no guarantees on the operation of this software and does not bear any
responsibility when using it.

History of changes.
Version 1.20:
 * internal changes.
Version 1.19:
* Support MS Windows XP SP2 and newer. The size of the plugin has decreased
significantly.
Version 1.18:
 * internal changes.
Version 1.17:
 * internal changes.
Version 1.16:
 * internal changes.
Version 1.15:
 * fixed focus behavior for password-protected files.
 * internal changes.
Version 1.14:
 * for correct operation of the plugin with SumatraPDF version 3.6.16761
pre-release. The configuration file from the plugin archive is no longer
required.
 * internal changes.
Version 1.13:
 * Fixed viewing chm files in version 3.6.16761 pre-release. Only works
with the SumatraPDF-settings.txt settings file from the folder attached to the
plugin archive SumatraPDF. The SumatraPDF-settings.txt file must be installed
read-only attribute.
 * internal changes.
Version 1.12:
 * internal changes.
Version 1.11:
 * internal changes.
Version 1.10:
 + switch between Total Commaner and application windows using Ctrl+TAB keys.
Version 1.9:
 * internal changes.
Version 1.8:
 * support for SumatraPDF v3.6.15944 Git-ca6bf0185, embedded in the archive.
With this version of SumatraPDF works correctly to view chm files.
 * internal changes.
Version 1.7:
 + new options have been added to the plugin configuration file sumatrapdf.ini
DirIniL=, DirIniQ=. Now you can configure your own file for each viewing mode
SumatraPDF-settings.txt.
 * internal changes.
Version 1.6:
 * fixed issue with focus in quick view mode when using plugin with SumatraPDF
version newer than 3.4.6.
 + added processing of keys 1..8.
 * internal changes.
Version 1.51:
* internal changes.
Please note that in SumatraPDF 3.4 the 'N', 'P' keys are not enabled by default.
work for navigating within files with some national keyboard layouts. If this
function is required, you should use SumatraPDF versions 3.1, 3.2, 3.3, or
switch to the English keyboard layout.
Version 1.5:
+ keys '4', 'N', 'P' are used in both SumatraPDF and TC Lister, depending
from the situation.
Version 1.4:
* internal changes.
Version 1.3:
* internal changes.
Version 1.2:
* The sumatrapdf.ini file is no longer overwritten when
the plugin is installed automatically. If it is missing, a new file with default
values is created. If the sumatrapdf.ini file exists, the values are taken from
it.
Version 1.1:
* increased the speed of work.
Version 1.0:
* Public version.