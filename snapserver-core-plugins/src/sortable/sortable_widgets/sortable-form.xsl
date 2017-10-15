<?xml version="1.0"?>
<!--
Snap Websites Server == sortable form XSLT, editor widget extensions
Copyright (C) 2016-2017  Made to Order Software Corp.

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
                              xmlns:snap="http://snapwebsites.info/snap-functions">
  <!-- xsl:variable name="editor-name">editor</xsl:variable>
  <xsl:variable name="editor-modified">2016-01-22 23:21:20</xsl:variable -->

  <!-- SORTABLE WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:sortable">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <div field_type="sortable" field_name="{$name}">
      <xsl:attribute name="class"><xsl:value-of select="classes"/><xsl:if
        test="$action = 'edit'">snap-editor </xsl:if> snap-editor-sortable <xsl:value-of
        select="$name"/><xsl:if test="state = 'disabled'"> disabled</xsl:if></xsl:attribute>
      <div value="" class="editor-content" name="{$name}" data-container="{container}">
        <xsl:copy-of select="value/node()"/>
      </div>
      <xsl:call-template name="snap:common-parts"/>
    </div>
    <!-- TODO: I think we should look into a better place for these includes -->
    <javascript name="sortable-widgets"/>
    <css name="sortable"/>
  </xsl:template>
  <xsl:template match="widget[@type='sortable']">
    <widget path="{@path}">
      <xsl:call-template name="snap:sortable">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

</xsl:stylesheet>
<!-- vim: ts=2 sw=2 et
-->
