DXF Viewer plugin 1.0.1.0 for Total Commander
===============================================

 * License:
-----------

Copyright (C) 2026 by Thomas Beutlich

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt


 * Installation:
----------------

Open the plugin archive using Total Commander and installation will start.


 * Description:
---------------

DXF Viewer is a lister plugin (WLX) for Total Commander to display DXF/DWG files
as interactive 2-D views using GDI+.

Supported file formats:
 o DXF (Drawing Exchange Format)
 o DWG (Drawing)

Features:
 o 2-D GDI+ rendering of DXF/DWG geometry
 o Mouse interaction: left-drag to pan, wheel to zoom
 o Double-click to fit view
 o Keyboard navigation: arrow keys for panning, +/- for zoom, Home to reset view
 o Copy to clipboard
 o Dark mode support
 o Background file loading


 * ChangeLog:
-------------

 o Version 1.0.1.0 (21.05.2026)
   - added DWG (Drawing) file format support
   - added font family matching for text rendering
   - added true color (24-bit) support for entities and layers
   - added solid fill rendering for hatches
   - fixed line weight rendering
   - fixed text encoding corruption when viewing multiple DWG files
   - fixed MTEXT formatting codes leaking into displayed text
   - fixed arc direction rendering
   - fixed text width estimation for multi-byte (CJK) characters
 o Version 1.0.0.1 (19.05.2026)
   - first public version
   - built with libdxfrw, win-iconv


 * References:
--------------

 o libdxfrw: C++ library to read and write DXF/DWG files 
   - https://github.com/codelibs/libdxfrw
 o win-iconv: Lightweight iconv implementation for Windows
   - https://github.com/win-iconv/win-iconv
 o WLX Writer's Reference by Christian Ghisler
   - https://ghisler.github.io/WLX-SDK


 * Trademark and Copyright Statements:
--------------------------------------

 o Total Commander is Copyright (C) 1993-2026 by Christian Ghisler, Ghisler Software GmbH.
   - https://www.ghisler.com


 * Feedback:
------------

If you have problems, questions, suggestions please contact Thomas Beutlich.
 o Email: support@tbeu.de
 o URL: http://tbeu.totalcmd.net
