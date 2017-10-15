<?xml version="1.0"?>
<!--
Snap Websites Server == password form XSLT, editor widget extensions
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
  <xsl:variable name="editor-modified">2015-11-04 20:45:48</xsl:variable -->

  <!-- PASSWORD WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:password">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <div field_type="password">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor </xsl:if>editable line-edit <xsl:value-of
        select="$name"/><xsl:if test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
        test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:value-of
        select="concat(' ', classes)"/><xsl:if test="state = 'disabled'"> disabled</xsl:if><xsl:if
        test="state = 'read-only'"> read-only</xsl:if><xsl:if
        test="@no-paste"> no-paste</xsl:if></xsl:attribute>
        <!-- always prevent spellcheck on password widgets -->
        <xsl:attribute name="spellcheck">false</xsl:attribute>
      <xsl:if test="background-value">
        <!-- by default "snap-editor-background" has "display: none"
             a script shows them on load once ready AND if the value is
             empty also it is a "pointer-event: none;" -->
        <div class="snap-editor-background zordered">
          <div class="snap-editor-background-content">
            <!-- this div is placed OVER the next div -->
            <xsl:copy-of select="background-value/node()"/>
          </div>
        </div>
      </xsl:if>
      <div value="">
        <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class">editor-content<xsl:if test="@no-toolbar or /editor-form/no-toolbar"> no-toolbar</xsl:if></xsl:attribute>
        <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
          <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="tooltip">
          <xsl:attribute name="title"><xsl:copy-of select="tooltip"/></xsl:attribute>
        </xsl:if>
        <xsl:call-template name="snap:text-field-filters"/>
        <!-- now the actual value ... which for password is always empty on a load -->
      </div>
      <xsl:call-template name="snap:common-parts"/>
    </div>
    <!-- TODO: I think we should look into a better place for these includes -->
    <javascript name="password"/>
  </xsl:template>
  <xsl:template match="widget[@type='password']">
    <widget path="{@path}">
      <xsl:call-template name="snap:password">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- PASSWORD-CONFIRM WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:password_confirm">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <div field_type="password_confirm">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <!-- TBD: the classes will be duplicated here and in the password sub-widgets -->
      <xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor </xsl:if>editable line-edit <xsl:value-of
        select="$name"/><xsl:if test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
        test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:value-of
        select="concat(' ', classes)"/><xsl:if test="state = 'disabled'"> disabled</xsl:if><xsl:if
        test="state = 'read-only'"> read-only</xsl:if><xsl:if
        test="@no-paste"> no-paste</xsl:if></xsl:attribute>

      <div class="editor-content no-hover" value="">
        <label class="password-confirm-password">
          <xsl:attribute name="for"><xsl:value-of select="$name"/>_password</xsl:attribute>
          Password:
        </label>
        <xsl:variable name="pc_password">
          <editor-form>
            <xsl:copy-of select="/editor-form/@*"/>
            <xsl:copy-of select="/editor-form/node()[name() != 'widget']"/>
            <widget path="{$path}">
              <xsl:copy-of select="@*[name() != 'type' and name() != 'path' and name() != 'auto-save']"/>
              <xsl:attribute name="type">password</xsl:attribute>
              <xsl:attribute name="auto-save">no</xsl:attribute>
              <xsl:copy-of select="*[name() = 'background-value' or name() = 'tooltip' or name() = 'help']"/>
            </widget>
          </editor-form>
        </xsl:variable>
        <xsl:for-each select="$pc_password/editor-form">
          <xsl:for-each select="widget">
            <xsl:call-template name="snap:password">
              <xsl:with-param name="path" select="$path"/>
              <xsl:with-param name="name" select="concat($name, '_password')"/>
            </xsl:call-template>
          </xsl:for-each>
        </xsl:for-each>

        <label class="password-confirm-confirm">
          <xsl:attribute name="for"><xsl:value-of select="$name"/>_confirm</xsl:attribute>
          Confirm:
        </label>
        <xsl:variable name="pc_confirm">
          <editor-form>
            <xsl:copy-of select="/editor-form/@*"/>
            <xsl:copy-of select="/editor-form/node()[name() != 'widget']"/>
            <widget path="{$path}">
              <xsl:copy-of select="@*[name() != 'type' and name() != 'path' and name() != 'auto-save']"/>
              <xsl:attribute name="type">password</xsl:attribute>
              <xsl:attribute name="auto-save">no</xsl:attribute>
              <xsl:copy-of select="*[name() = 'tooltip' or name() = 'help']"/>
              <xsl:if test="*[name() = 'background-confirm']">
                <background-value><xsl:value-of select="*[name() = 'background-confirm']"/></background-value>
              </xsl:if>
            </widget>
          </editor-form>
        </xsl:variable>
        <xsl:for-each select="$pc_confirm/editor-form">
          <xsl:for-each select="widget">
            <xsl:call-template name="snap:password">
              <xsl:with-param name="path" select="$path"/>
              <xsl:with-param name="name" select="concat($name, '_confirm')"/>
            </xsl:call-template>
          </xsl:for-each>
        </xsl:for-each>

        <div class="password-status-details good">
          <p>Password status:</p>
          <ul class="password-status">
            <li>Please enter a password twice.</li>
          </ul>
        </div>
      </div>
    </div>
    <!-- TODO: I think we should look into a better place for these includes -->
    <javascript name="password"/>
    <css name="password"/>
  </xsl:template>
  <xsl:template match="widget[@type='password_confirm']">
    <widget path="{@path}">
      <xsl:call-template name="snap:password_confirm">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

</xsl:stylesheet>
<!-- vim: ts=2 sw=2 et
-->
