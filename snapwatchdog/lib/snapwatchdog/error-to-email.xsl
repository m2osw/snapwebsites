<?xml version="1.0"?>
<!--
Snap Websites Server == snapwatchdog error to email parser
Copyright (c) 2018-2019  Made to Order Software Corp.  All Rights Reserved

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

  <xsl:template match="watchdog/error">
    <!--message priority="90" plugin_name="cassandra">can't find Cassandra in the list of processes.</message-->
    <html>
      <head>
        <title>Watchdog Error Report</title>
      </head>
      <body>
        <xsl:for-each select="message">
          <h2><xsl:value-of select="@plugin_name"/> (<xsl:value-of select="@priority"/>)</h2>
          <blockquote><xsl:copy-of select="./node()"/></blockquote>
        </xsl:for-each>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2 et
-->
