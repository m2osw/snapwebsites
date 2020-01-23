<?xml version="1.0"?>
<!--
Snap Websites Server == snapmanager.cgi parser
Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
-->
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                              xmlns:xs="http://www.w3.org/2001/XMLSchema"
                              xmlns:fn="http://www.w3.org/2005/xpath-functions"
                              xmlns:snap="snap:snap">

  <xsl:template match="manager">
    <html>
      <head>
        <meta charset="utf-8"/>
        <title><xsl:copy-of select="title/node()"/></title>
        <meta name="generator" content="Snap! Manager CGI"/>
        <meta name="viewport" content="width=device-width, initial-scale=1"/>
        <meta name="robots" content="noindex,nofollow"/>
        <link rel="bookmark" type="text/html" title="Generator" href="https://snapwebsites.org/project/snap-manager"/>
        <link rel="shortcut icon" href="/favicon.ico" type="image/x-icon"/>
        <link rel="stylesheet" href="/jquery-ui.css"/>
        <link rel="stylesheet" href="/snapmanagercgi.css"/>
        <script type="text/javascript" src="/jquery.js"/>
        <script type="text/javascript" src="/jquery-ui.js"/>
        <script type="text/javascript" src="/snapmanagercgi.js"/>
      </head>
      <body>
        <header>
          <ul id="menu" class="menu">
            <li class="menu-item"><div><a href="/snapmanager">Home</a></div></li>
            <!-- more entries... -->
            <xsl:for-each select="menu/item">
                <li class="menu-item"><div><a href="{@href}"><xsl:copy-of select="./node()"/></a></div></li>
            </xsl:for-each>
            <li class="menu-item"><div><a href="/snapmanager?logout">Log Out</a></div></li>
          </ul>
          <div class="snap-version">
            <xsl:copy-of select="snap-version/node()"/>
            <br/>
            <span class="menu-status">
              <xsl:for-each select="menu/status">
                <xsl:copy-of select="./node()"/>
              </xsl:for-each>
            </span>
          </div>
          <div class="wait">
            <img id="globe" src="/globe_still.png" width="50" height="50"/>
          </div>
          <div style="clear: both"></div>
        </header>
        <xsl:copy-of select="output/node()"/>
        <div id='feedback'>
          <div class="titlebar"><div class="title">Feedback Window</div><div class="close-button">X</div></div>
          <div class="message-list"></div>
        </div>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2 et
-->
