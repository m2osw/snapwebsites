<?xml version="1.0"?>
<!--
Snap Websites Server == locale form XSLT, editor widget extensions
Copyright (C) 2014-2017  Made to Order Software Corp.

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
  <xsl:variable name="editor-modified">2014-09-14 17:51:48</xsl:variable -->

  <!-- TIMEZONE WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:locale_timezone">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <xsl:param name="value"/>
    <div field_type="locale_timezone">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor </xsl:if>editable composite transparent <xsl:value-of
        select="classes"/> locale_timezone <xsl:value-of select="$name"/><xsl:if
        test="@immediate or /editor-form/immediate"> immediate</xsl:if></xsl:attribute>

      <div class="editor-content">
        <!-- TODO: implement proper tabindex -->
        <!-- WARNING: the order is VERY important -->
        <xsl:choose>
          <!-- search for one @value that matches $value, this is the preferred method of selection -->
          <xsl:when test="$value != '' and preset/item[$value = @value]">
            <xsl:attribute name="value"><xsl:copy-of select="$value"/></xsl:attribute>
          </xsl:when>
          <!-- value is defined, use it... -->
          <xsl:when test="$value != ''">
            <!-- if we did not match an @value, then we assume there are none (TBD?) -->
            <xsl:attribute name="value"><xsl:copy-of select="$value"/></xsl:attribute>
          </xsl:when>
          <!-- programmer specified a default -->
          <xsl:when test="preset/item[@default = 'default']">
            <xsl:choose>
              <xsl:when test="preset/item[@default = 'default']/@value">
                <xsl:attribute name="value"><xsl:copy-of select="preset/item[@default = 'default']/@value"/></xsl:attribute>
              </xsl:when>
              <xsl:otherwise>
                <xsl:attribute name="value"><xsl:copy-of select="preset/item[@default = 'default']/node()"/></xsl:attribute>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:when>
          <xsl:otherwise>
            <!-- otherwise stick the default value in there if there is one
                 this can be something completely different from what
                 appears in the list of items -->
            <xsl:attribute name="value"><xsl:copy-of select="default/node()"/></xsl:attribute>
          </xsl:otherwise>
        </xsl:choose>

        <label class="locale-timezone-continent">Continent/Country:</label>
        <xsl:variable name="continent">
          <editor-form>
            <xsl:copy-of select="/editor-form/@*"/>
            <xsl:copy-of select="/editor-form/node()[name() != 'widget']"/>
            <widget path="{$path}">
              <xsl:copy-of select="@*[name() != 'type' and name() != 'path' and name() != 'auto-save']"/>
              <xsl:attribute name="type">dropdown</xsl:attribute>
              <xsl:attribute name="auto-save">no</xsl:attribute>
              <xsl:copy-of select="*[name() = 'background-value' or name() = 'tooltip' or name() = 'help']"/>
              <xsl:if test="default/node()">
                <default><xsl:copy-of select="replace(substring-before(string(default/node()), '/'), '_', ' ')"/></default>
              </xsl:if>
              <preset><xsl:copy-of select="preset_continent/node()"/></preset>
            </widget>
          </editor-form>
        </xsl:variable>
        <xsl:for-each select="$continent/editor-form">
          <xsl:for-each select="widget">
            <xsl:call-template name="snap:dropdown">
              <xsl:with-param name="path" select="$path"/>
              <xsl:with-param name="name" select="concat($name, '_continent')"/>
              <!-- No need to do this at all, since we've already selected the value as default. The control can handle it.
              <xsl:with-param name="value" select="replace(substring-before(string($value), '/'), '_', ' ')"/>
              -->
            </xsl:call-template>
          </xsl:for-each>
        </xsl:for-each>

        <label class="locale-timezone-city">City:</label>
        <xsl:variable name="city">
          <editor-form>
            <xsl:copy-of select="/editor-form/@*"/>
            <xsl:copy-of select="/editor-form/node()[name() != 'widget']"/>
            <widget path="{$path}">
              <xsl:copy-of select="@*[name() != 'type' and name() != 'path' and name() != 'auto-save']"/>
              <xsl:attribute name="type">dropdown</xsl:attribute>
              <xsl:attribute name="auto-save">no</xsl:attribute>
              <xsl:copy-of select="*[name() = 'background-value' or name() = 'tooltip' or name() = 'help']"/>
              <xsl:if test="default/node()">
                <default><xsl:copy-of select="replace(substring-after(string(default/node()), '/'), '_', ' ')"/></default>
              </xsl:if>
              <preset><xsl:copy-of select="preset_city/node()"/></preset>
            </widget>
          </editor-form>
        </xsl:variable>
        <xsl:for-each select="$city/editor-form">
          <xsl:for-each select="widget">
            <xsl:call-template name="snap:dropdown">
              <xsl:with-param name="path" select="$path"/>
              <xsl:with-param name="name" select="concat($name, '_city')"/>
              <!-- No need to do this at all, since we've already selected the value as default. The control can handle it.
              <xsl:with-param name="value" select="replace(substring-after(string($value), '/'), '_', ' ')"/>
              -->
            </xsl:call-template>
          </xsl:for-each>
        </xsl:for-each>
      </div>
    </div>
    <!-- TODO: I think we should look into a better place for these includes -->
    <javascript name="locale-timezone"/>
    <!--css name="locale-timezone"/-->
  </xsl:template>
  <xsl:template match="widget[@type='locale_timezone']">
    <xsl:variable name="value">
      <xsl:choose>
        <!-- use the post value when there is one, it has priority -->
        <xsl:when test="post != ''"><xsl:copy-of select="post/node()"/></xsl:when>
        <!-- use the current value when there is one -->
        <xsl:when test="value != ''"><xsl:copy-of select="value/node()"/></xsl:when>
        <xsl:otherwise></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <widget path="{@path}">
      <xsl:call-template name="snap:locale_timezone">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
        <xsl:with-param name="value" select="$value"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

</xsl:stylesheet>
<!-- vim: ts=2 sw=2 et
-->
