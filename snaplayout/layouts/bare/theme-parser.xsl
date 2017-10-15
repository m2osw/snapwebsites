<?xml version="1.0"?>
<!--
Snap Websites Server == bare layout theme setup
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
<!-- to install: snaplayout bare-theme-parser.xsl -->
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                              xmlns:xs="http://www.w3.org/2001/XMLSchema"
                              xmlns:fn="http://www.w3.org/2005/xpath-functions"
                              xmlns:snap="snap:snap">
  <!-- include system data -->
  <xsl:include href="functions"/>
  <xsl:include href="html-header"/>
  <xsl:include href="user-messages"/>

  <!-- some special variables to define the theme -->
  <xsl:variable name="layout-name">bare</xsl:variable>
  <xsl:variable name="layout-area">theme-parser</xsl:variable>
  <xsl:variable name="layout-modified">2016-04-02 22:00:25</xsl:variable>

  <xsl:template match="snap">
    <!--
      Notes:
      The "prefix" attribute is an HTML5 feature.
      The xmlns is not valid in HTML5.
      Also, an xmlns is confusing the XSLT parser like hell.
      attr: lang="{$lang}" xml:lang="{$lang}" prefix="og: http://ogp.me/ns#"
    -->
    <html>
      <xsl:call-template name="snap:html-attributes"/>
      <head>
        <xsl:call-template name="snap:html-header">
          <xsl:with-param name="theme-css" select="'/css/bare/style.css'"/>
        </xsl:call-template>
      </head>
      <xsl:apply-templates select="page/body"/>
    </html>
  </xsl:template>
  <!--xsl:template match="head">
    <head>Nothing</head>
  </xsl:template-->
  <xsl:template match="page/body">
    <body>
      <xsl:call-template name="snap:user-messages"/>
      <div>
        <xsl:choose>
          <xsl:when test="/snap/head/metadata/page_session">
            <xsl:attribute name="class">page editor-form</xsl:attribute>
            <xsl:attribute name="form_name">page_form</xsl:attribute>
            <xsl:attribute name="session"><xsl:value-of select="/snap/head/metadata/page_session"/></xsl:attribute>
          </xsl:when>
          <xsl:otherwise>
            <xsl:attribute name="class">page</xsl:attribute>
          </xsl:otherwise>
        </xsl:choose>
        <div class="header">
          <h1><xsl:value-of select="/snap/head/metadata/desc[@type='name']/data"/></h1>
        </div>
        <div class="inner-page">
          <div class="left">
            <xsl:for-each select="/snap/page/boxes/left">
              <div class="box">
                <!-- copy nodes under left -->
                <h2 class="box-title"><xsl:choose>
                <xsl:when test="descendant::node()/titles/short-title">
                  <xsl:value-of select="*/titles/short-title"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="*/titles/title"/>
                </xsl:otherwise>
                </xsl:choose></h2>
                <div class="box-content">
                  <xsl:copy-of select="descendant::node()/content/node()"/>
                </div>
              </div>
            </xsl:for-each>
          </div>
          <div class="content">
            <!--  contenteditable="true" -->
            <div class="breadcrumb"><xsl:copy-of select="/snap/page/body/breadcrumb/node()"/></div>
            <div class="page-title">
              <h2 name="title">
                <div field_type="text-edit" field_name="title">
                  <xsl:attribute name="class"><xsl:if test="$action = 'administer'">snap-editor</xsl:if></xsl:attribute>
                  <div class="editor-content" name="title">
                     <!-- TODO: on display we may choose the long or normal
                                title but at this time we only allow the
                                editing of the normal title -->
                    <xsl:choose>
                      <xsl:when test="/snap/page/body/titles/long-title">
                        <xsl:copy-of select="/snap/page/body/titles/long-title/node()"/>
                      </xsl:when>
                      <xsl:otherwise>
                        <xsl:copy-of select="/snap/page/body/titles/title/node()"/>
                      </xsl:otherwise>
                    </xsl:choose>
                  </div>
                </div>
              </h2>
            </div>
            <div class="body">
              <xsl:copy-of select="output/node()"/>
            </div>
          </div>
          <div class="clear-both"></div>
        </div>
        <div class="footer">
          <p>Powered by <a href="http://snapwebsites.com/">Snap! C++ Websites</a></p>
          <p>Copyright (c) 2012-<xsl:value-of select="$year"/> by <a href="http://www.m2osw.com">Made to Order Software Corp.</a> &#8212; All Rights Reserved</p>
        </div>
      </div>
    </body>
  </xsl:template>
  <xsl:template name="left">
    <xsl:apply-templates>
        <!-- this or the following both work:
         xsl:sort select="@priority" data-type="number" /-->
        <xsl:sort select="xs:decimal(@priority)"/>
    </xsl:apply-templates>
  </xsl:template>
  <xsl:template name="left-boxes">
    <!-- left area -->
    <div>
      <xsl:attribute name="class">left area</xsl:attribute>
      <div class="box">Box[<xsl:value-of select="position()"/>] = (<xsl:value-of select="@priority"/>) <xsl:value-of select="title"/></div>
    </div>
  </xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2 et
-->
