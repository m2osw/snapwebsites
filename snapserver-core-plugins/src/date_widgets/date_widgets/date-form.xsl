<?xml version="1.0"?>
<!--
Snap Websites Server == date form XSLT, editor widget extensions
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

  <!-- DATE EDIT WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:date-edit">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <div field_type="date-edit">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor </xsl:if>editable date-edit <xsl:value-of
        select="$name"/><xsl:if test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
        test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:value-of
        select="concat(' ', classes)"/><xsl:if test="state = 'disabled'"> disabled</xsl:if><xsl:if
        test="state = 'read-only'"> read-only</xsl:if><xsl:if
        test="state = 'auto-hide'"> auto-hide</xsl:if></xsl:attribute>
      <xsl:if test="state = 'read-only' or state = 'disabled'">
        <!-- avoid spellcheck of non-editable widgets -->
        <xsl:attribute name="spellcheck">false</xsl:attribute>
      </xsl:if>
      <xsl:if test="background-value">
        <!-- by default "snap-editor-background" has "display: none"
             a script shows them on load once ready AND if the value is empty
             also it is a "pointer-event: none;" -->
        <div class="snap-editor-background zordered">
          <div class="snap-editor-background-content">
            <!-- this div is placed OVER the next div -->
            <xsl:copy-of select="background-value/node()"/>
          </div>
        </div>
      </xsl:if>
      <div>
        <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class">editor-content<xsl:if test="@no-toolbar or /editor-form/no-toolbar"> no-toolbar</xsl:if></xsl:attribute>
        <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
          <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="tooltip">
          <xsl:attribute name="title"><xsl:copy-of select="tooltip"/></xsl:attribute>
        </xsl:if>
        <xsl:call-template name="snap:text-field-filters"/>

        <!-- now the actual value of this line -->
        <xsl:choose>
          <xsl:when test="post">
            <!-- use the post value when there is one, it has priority -->
            <xsl:copy-of select="post/node()"/>
          </xsl:when>
          <xsl:when test="value">
            <!-- use the current value when there is one -->
            <xsl:copy-of select="value/node()"/>
          </xsl:when>
          <xsl:when test="value/@default">
            <!-- transform the system default if one was defined -->
            <xsl:choose>
              <xsl:when test="value/@default = 'today'">
                <!-- US format & GMT... this should be a parameter, probably a variable we set in the editor before running the parser? -->
                <xsl:value-of select="month-from-date(current-date())"/>/<xsl:value-of select="day-from-date(current-date())"/>/<xsl:value-of select="year-from-date(current-date())"/>
              </xsl:when>
            </xsl:choose>
          </xsl:when>
        </xsl:choose>
      </div>
      <xsl:call-template name="snap:common-parts"/>
    </div>
    <!-- TODO: I think we should look into a better place for these includes -->
    <javascript name="date-widgets"/>
    <css name="date-widgets"/>
  </xsl:template>
  <xsl:template match="widget[@type='date-edit']">
    <widget path="{@path}">
      <xsl:call-template name="snap:date-edit">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- DROPDOWN DATE EDIT WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:dropdown-date-edit">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <div field_type="dropdown-date-edit">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor </xsl:if>editable composite transparent dropdown-date-edit <xsl:value-of
        select="$name"/><xsl:if test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
        test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:value-of
        select="concat(' ', classes)"/><xsl:if test="state = 'disabled'"> disabled</xsl:if><xsl:if
        test="state = 'read-only'"> read-only</xsl:if><xsl:if
        test="state = 'auto-hide'"> auto-hide</xsl:if></xsl:attribute>
      <xsl:if test="state = 'read-only' or state = 'disabled'">
        <!-- avoid spellcheck of non-editable widgets -->
        <xsl:attribute name="spellcheck">false</xsl:attribute>
      </xsl:if>

      <xsl:variable name="date_value">
        <!-- now the actual value of this line -->
        <xsl:choose>
          <xsl:when test="post">
            <!-- use the post value when there is one, it has priority -->
            <xsl:copy-of select="post/node()"/>
          </xsl:when>
          <xsl:when test="value">
            <!-- use the current value when there is one -->
            <xsl:copy-of select="value/node()"/>
          </xsl:when>
          <xsl:when test="value/@default">
            <!-- transform the system default if one was defined -->
            <xsl:choose>
              <xsl:when test="value/@default = 'today'">
                <!-- YYYY/MM/DD format -->
                <xsl:value-of select="year-from-date(current-date())"/>/<xsl:value-of select="month-from-date(current-date())"/>/<xsl:value-of select="day-from-date(current-date())"/>
              </xsl:when>
            </xsl:choose>
          </xsl:when>
        </xsl:choose>
      </xsl:variable>

      <div value="{$date_value}">
        <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class">editor-content<xsl:if test="@no-toolbar or /editor-form/no-toolbar"> no-toolbar</xsl:if></xsl:attribute>
        <!-- TODO: tabindex will be in the children, here we may have some form of "group-tabindex" stuff
        <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
          <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/></xsl:attribute>
        </xsl:if-->
        <xsl:if test="tooltip">
          <xsl:attribute name="title"><xsl:copy-of select="tooltip"/></xsl:attribute>
        </xsl:if>
        <xsl:call-template name="snap:text-field-filters"/>

        <div class="labels">
          <xsl:if test="dropdown-date-edit/include-month">
            <label class="dropdown-date">
              <xsl:attribute name="for"><xsl:value-of select="$name"/>_month</xsl:attribute>
              Month:
            </label>
          </xsl:if>
          <xsl:if test="dropdown-date-edit/include-day">
            <label class="dropdown-date">
              <xsl:attribute name="for"><xsl:value-of select="$name"/>_day</xsl:attribute>
              Day:
            </label>
          </xsl:if>
          <xsl:if test="dropdown-date-edit/include-year">
            <label class="dropdown-date">
              <xsl:attribute name="for"><xsl:value-of select="$name"/>_year</xsl:attribute>
              Year:
            </label>
          </xsl:if>
        </div>

        <xsl:if test="dropdown-date-edit/include-month">
          <xsl:variable name="dd_month">
            <editor-form>
              <xsl:copy-of select="/editor-form/@*"/>
              <xsl:copy-of select="/editor-form/node()[name() != 'widget']"/>
              <widget path="{$path}">
                <xsl:copy-of select="@*[name() != 'type' and name() != 'path' and name() != 'auto-save']"/>
                <xsl:attribute name="type">dropdown</xsl:attribute>
                <xsl:attribute name="auto-save">no</xsl:attribute>
                <xsl:copy-of select="*[name() = 'tooltip' or name() = 'help']"/>
                <xsl:if test="*[name() = 'background-month']">
                  <background-value><xsl:value-of select="*[name() = 'background-month']"/></background-value>
                </xsl:if>
                <dropdown-algorithm>bottom top right bottom-vertical-columns=2-3 top-vertical-columns=2-3</dropdown-algorithm>
                <preset>
                  <item value="1">Jan</item>
                  <item value="2">Feb</item>
                  <item value="3">Mar</item>
                  <item value="4">Apr</item>
                  <item value="5">May</item>
                  <item value="6">Jun</item>
                  <item value="7">Jul</item>
                  <item value="8">Aug</item>
                  <item value="9">Sep</item>
                  <item value="10">Oct</item>
                  <item value="11">Nov</item>
                  <item value="12">Dec</item>
                </preset>
                <xsl:if test="string-length($date_value) > 0">
                  <value><xsl:value-of select="string(number(tokenize($date_value, '/')[2]))"/></value>
                </xsl:if>
              </widget>
            </editor-form>
          </xsl:variable>
          <xsl:for-each select="$dd_month/editor-form">
            <xsl:for-each select="widget">
              <xsl:call-template name="snap:dropdown">
                <xsl:with-param name="path" select="$path"/>
                <xsl:with-param name="name" select="concat($name, '_month')"/>
                <xsl:with-param name="value" select="value/node()"/>
              </xsl:call-template>
            </xsl:for-each>
          </xsl:for-each>
        </xsl:if>

        <xsl:if test="dropdown-date-edit/include-day">
          <xsl:variable name="dd_day">
            <editor-form>
              <xsl:copy-of select="/editor-form/@*"/>
              <xsl:copy-of select="/editor-form/node()[name() != 'widget']"/>
              <widget path="{$path}">
                <xsl:copy-of select="@*[name() != 'type' and name() != 'path' and name() != 'auto-save']"/>
                <xsl:attribute name="type">dropdown</xsl:attribute>
                <xsl:attribute name="auto-save">no</xsl:attribute>
                <xsl:copy-of select="*[name() = 'tooltip' or name() = 'help']"/>
                <xsl:if test="*[name() = 'background-day']">
                  <background-value><xsl:value-of select="*[name() = 'background-day']"/></background-value>
                </xsl:if>
                <dropdown-algorithm>bottom top right bottom-horizontal-columns=7 top-horizontal-columns=7</dropdown-algorithm>
                <preset>
                  <item>1</item>
                  <item>2</item>
                  <item>3</item>
                  <item>4</item>
                  <item>5</item>
                  <item>6</item>
                  <item>7</item>
                  <item>8</item>
                  <item>9</item>
                  <item>10</item>
                  <item>11</item>
                  <item>12</item>
                  <item>13</item>
                  <item>14</item>
                  <item>15</item>
                  <item>16</item>
                  <item>17</item>
                  <item>18</item>
                  <item>19</item>
                  <item>20</item>
                  <item>21</item>
                  <item>22</item>
                  <item>23</item>
                  <item>24</item>
                  <item>25</item>
                  <item>26</item>
                  <item>27</item>
                  <item>28</item>
                  <item>29</item>
                  <item>30</item>
                  <item>31</item>
                </preset>
                <xsl:if test="string-length($date_value) > 0">
                  <value><xsl:value-of select="string(number(tokenize($date_value, '/')[3]))"/></value>
                </xsl:if>
              </widget>
            </editor-form>
          </xsl:variable>
          <xsl:for-each select="$dd_day/editor-form">
            <xsl:for-each select="widget">
              <xsl:call-template name="snap:dropdown">
                <xsl:with-param name="path" select="$path"/>
                <xsl:with-param name="name" select="concat($name, '_day')"/>
                <xsl:with-param name="value" select="value/node()"/>
              </xsl:call-template>
            </xsl:for-each>
          </xsl:for-each>
        </xsl:if>

        <xsl:if test="dropdown-date-edit/include-year">
          <xsl:variable name="dd_year">
            <editor-form>
              <xsl:copy-of select="/editor-form/@*"/>
              <xsl:copy-of select="/editor-form/node()[name() != 'widget']"/>
              <widget path="{$path}">
                <xsl:copy-of select="@*[name() != 'type' and name() != 'path' and name() != 'auto-save']"/>
                <xsl:attribute name="type">dropdown</xsl:attribute>
                <xsl:attribute name="auto-save">no</xsl:attribute>
                <xsl:copy-of select="*[name() = 'tooltip' or name() = 'help']"/>
                <xsl:if test="*[name() = 'background-year']">
                  <background-value><xsl:value-of select="*[name() = 'background-year']"/></background-value>
                </xsl:if>
                <dropdown-algorithm>bottom top right bottom-vertical-columns=2 top-vertical-columns=2 bottom-vertical-columns=3 top-vertical-columns=3 bottom-vertical-columns-with-scrollbar=* top-vertical-columns-with-scrollbar=*</dropdown-algorithm>
                <xsl:variable name="from" select="dropdown-date-edit/include-year/@from"/>
                <xsl:variable name="to" select="dropdown-date-edit/include-year/@to"/>
                <xsl:if test="string-length($from) > 0 and string-length($to) > 0">
                  <preset>
                    <xsl:for-each select="$from to $to">
                      <item><xsl:value-of select="."/></item>
                    </xsl:for-each>
                  </preset>
                </xsl:if>
                <xsl:if test="string-length($date_value) > 0">
                  <value><xsl:value-of select="string(number(tokenize($date_value, '/')[1]))"/></value>
                </xsl:if>
              </widget>
            </editor-form>
          </xsl:variable>
          <xsl:for-each select="$dd_year/editor-form">
            <xsl:for-each select="widget">
              <xsl:call-template name="snap:dropdown">
                <xsl:with-param name="path" select="$path"/>
                <xsl:with-param name="name" select="concat($name, '_year')"/>
                <xsl:with-param name="value" select="value/node()"/>
              </xsl:call-template>
            </xsl:for-each>
          </xsl:for-each>
        </xsl:if>

        <div class="clear-both"/>

      </div>
      <xsl:call-template name="snap:common-parts"/>
    </div>
    <!-- TODO: I think we should look into a better place for these includes -->
    <javascript name="date-widgets"/>
    <css name="date-widgets"/>
  </xsl:template>
  <xsl:template match="widget[@type='dropdown-date-edit']">
    <widget path="{@path}">
      <xsl:call-template name="snap:dropdown-date-edit">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

</xsl:stylesheet>
<!-- vim: ts=2 sw=2 et
-->
