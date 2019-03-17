/** @preserve
 * Name: flash-detection
 * Version: 0.0.1
 * Browsers: all
 * Copyright: Copyright (c) 2005-2019  Made to Order Software Corp.  All Rights Reserved
 * Depends: jquery (1.11.2)
 * License: GPL 2.0
 */


/////////////////////////////////////////////////////////////////////////////
//
// flash_detection.js -- detect what version of the Flash player is available
// Copyright (c) 2005-2019  Made to Order Software Corp.  All Rights Reserved
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
/////////////////////////////////////////////////////////////////////////////
//
// You can find a complete copy of the GPL at the following address:
//
//	http://gpl.m2osw.com
//
// Also, you can find original copies at the GNU foundation:
//
//	http://www.gnu.org
//


// Detect which version of flash the user has and save that information
// in a global variable.

var flash_version = 0;
var agent = null;


// Netscape 3+ and Opera 3+
if(navigator.plugins != null && navigator.plugins.length > 0) {
	var flash_plugin = navigator.plugins['Shockwave Flash'];
	if(typeof flash_plugin == 'object') {

document.write("<p>Description: " + flash_plugin.description + "</p>");

		if(flash_plugin.description.indexOf(' 8.') != -1) {
			flash_version = 8;
		}
		else if(flash_plugin.description.indexOf(' 7.') != -1) {
			flash_version = 7;
		}
		else if(flash_plugin.description.indexOf(' 6.') != -1) {
			flash_version = 6;
		}
		else if(flash_plugin.description.indexOf(' 5.') != -1) {
			flash_version = 5;
		}
		else if(flash_plugin.description.indexOf(' 4.') != -1) {
			flash_version = 4;
		}
		else if(flash_plugin.description.indexOf(' 3.') != -1) {
			flash_version = 3;
		}
	}
}

// Internet Explorer 4+ on Win32
if(flash_version == 0) {
	agent = navigator.userAgent.toLowerCase();
	if(agent.indexOf("msie") != -1
	&& parseInt(navigator.appVersion, 10) >= 4
	&& agent.indexOf("win") != -1
	&& agent.indexOf("16bit") == -1) {
		document.write('<scr' + 'ipt language="VBScript"\>\n');
		document.write('on error resume next\n');
		document.write('dim obFlash\n');
		document.write('flash_version = 0\n');
		document.write('set obFlash = CreateObject("ShockwaveFlash.ShockwaveFlash.8")\n');
		document.write('if IsObject(obFlash) then\n');
		document.write('flash_version = 8\n');
		document.write('else set obFlash = CreateObject("ShockwaveFlash.ShockwaveFlash.7") end if\n');
		document.write('if flash_version < 8 and IsObject(obFlash) then\n');
		document.write('flash_version = 7\n');
		document.write('else set obFlash = CreateObject("ShockwaveFlash.ShockwaveFlash.6") end if\n');
		document.write('if flash_version < 7 and IsObject(obFlash) then\n');
		document.write('flash_version = 6\n');
		document.write('else set obFlash = CreateObject("ShockwaveFlash.ShockwaveFlash.5") end if\n');
		document.write('if flash_version < 6 and IsObject(obFlash) then\n');
		document.write('flash_version = 5\n');
		document.write('else set obFlash = CreateObject("ShockwaveFlash.ShockwaveFlash.4") end if\n');
		document.write('if flash_version < 5 and IsObject(obFlash) then\n');
		document.write('flash_version = 4\n');
		document.write('else set obFlash = CreateObject("ShockwaveFlash.ShockwaveFlash.3") end if\n');
		document.write('if flash_version < 4 and IsObject(obFlash) then\n');
		document.write('flash_version = 3\n');
		document.write('end if');
		document.write('</scr' + 'ipt\> \n');
	}
}

// WebTV 2.5 supports flash 3
if(flash_version == 0) {
	agent = navigator.userAgent.toLowerCase();
	if(agent.indexOf("webtv/2.5") != -1) {
		flash_version = 3;
	}
}


//
// Usage: insert_flash_movie(flash, image, width, height, min_version, flash_vars)
//
// flash - URL to a flash animation
// image - URL to the image to use as a replacement when flash is not available
// width - the width of the flash animation and image
// height - the height of the flash animation and image
// min_version - the minimum version of flash you need for this movie
// parameters - variables to pass down to the flash plug-in
//
// This function inserts a flash movie, or if the flash movie is either
// too advanced or no plug-in is available, insert a corresponding image.
//
// Note that this may not be suited to your need if you want to have a
// link or a style on the image, for instance.
//
		
function insert_flash_movie(flash, image, width, height, min_version, parameters)
{ 
	if(min_version <= flash_version)
	{
		document.write('<object classid="clsid:D27CDB6E-AE6D-11cf-96B8-444553540000" ');
		document.write('codebase="https://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=' + min_version + ',0,0,0" ');
		document.write('width="' + width + '" height="' + height +'">');
		document.write('<param name="movie" value="' + flash + '">');
		document.write('<param name="flashvars" value="' + parameters + '">');
		document.write('<param name="quality" value="best">');
		document.write('<param name="play" value="true">');
		document.write('<embed pluginspage="https://www.macromedia.com/go/getflashplayer" ');
		document.write('src="' + flash + '" type="application/x-shockwave-flash" ');
		document.write('flashvars="' + parameters  + '" ');
		document.write('height="' + height +'" width="' + width + '" ');
		document.write('quality="best" play="true"></embed>');
		document.write('</object>');
	}
	else
	{
		document.write('<img src="' + image + '" width="' + width + '" height="' + height + '" border="0">');
	}
}

