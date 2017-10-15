<?xml version="1.0"?>
<!--
Snap Websites Server == editor form XSLT, generate HTML from editor widgets
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
  <xsl:variable name="editor-name">editor</xsl:variable>
  <xsl:variable name="editor-modified">2015-11-04 20:45:48</xsl:variable>

  <!-- COMMAND PARTS -->
  <xsl:template name="snap:common-parts">
    <xsl:param name="type" select="@type"/>
    <xsl:if test="help">
      <!-- make this hidden by default because it is expected to be -->
      <div class="editor-help {$type}-help" style="display: none;">
        <xsl:copy-of select="help/node()"/>
      </div>
    </xsl:if>
  </xsl:template>

  <!-- TEXT FIELDS FILTERS -->
  <xsl:template name="snap:text-field-filters">
    <xsl:if test="sizes/absolute-min"><xsl:attribute name="data-absoluteminlength"><xsl:value-of select="sizes/absolute-min"/></xsl:attribute></xsl:if>
    <xsl:if test="sizes/min"><xsl:attribute name="data-minlength"><xsl:value-of select="sizes/min"/></xsl:attribute></xsl:if>
    <xsl:if test="sizes/absolute-max"><xsl:attribute name="data-absolutemaxlength"><xsl:value-of select="sizes/absolute-max"/></xsl:attribute></xsl:if>
    <xsl:if test="sizes/max"><xsl:attribute name="data-maxlength"><xsl:value-of select="sizes/max"/></xsl:attribute></xsl:if>
  </xsl:template>

  <!-- DROPPED FILE WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:dropped-file">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <xsl:param name="type"/>
    <div field_type="dropped-file">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <!--
        IMPORTANT: In case of the dropped-file widgets, we always include
                   the snap-editor class, meaning that it always looks like
                   it is editable; however, there will be no input file,
                   no "Upload" and "Reset" buttons so the functionality
                   will still be limited to just downloading.

        WARNING: at this point we check whether we are in "edit" mode before
                 adding the 'drop' class and also below we only offer the
                 Upload and Reset buttons when in edit mode; only in our
                 regular system (not forms) we do not have such a mode on
                 loading... that being said, I do not thing we can re-use
                 this widget as is there anyway.
      -->
      <xsl:attribute name="class">snap-editor editable no-hover dropped-file-box <xsl:value-of
          select="$name"/><xsl:if test="(@drop or /editor-form/drop) and $action = 'edit'"> drop</xsl:if><xsl:if
          test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
          test="$name = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:value-of
          select="concat(' ', classes)"/></xsl:attribute>
      <xsl:if test="background-value">
        <!-- by default "snap-editor-background" objects have "display: none"
             a script shows them on load once ready AND if the value is empty
             also it is a "pointer-event: none;" -->
        <div class="snap-editor-background zordered">
          <div class="snap-editor-background-content">
            <!-- this div is placed OVER the next div -->
            <xsl:copy-of select="background-value/node()"/>
          </div>
        </div>
      </xsl:if>
      <xsl:variable name="uri">
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
        </xsl:choose>
      </xsl:variable>
      <div value="{$uri}">
        <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
        <!-- TBD: should we use "image" instead of "attachment" since we show images? -->
        <xsl:attribute name="class">editor-content dropped-file attachment no-toolbar<xsl:if
          test="state = 'disabled'"> disabled</xsl:if></xsl:attribute>
        <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
          <!-- put two attributes, that way we can set the tabindex to -1
               when disabling a screen -->
          <xsl:variable name="tabindex" select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/>
          <xsl:attribute name="tabindex"><xsl:value-of select="$tabindex"/></xsl:attribute>
          <xsl:attribute name="original_tabindex"><xsl:value-of select="$tabindex"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="tooltip">
          <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="sizes/min"><xsl:attribute name="min-sizes"><xsl:value-of select="sizes/min"/></xsl:attribute></xsl:if>
        <xsl:if test="sizes/max"><xsl:attribute name="max-sizes"><xsl:value-of select="sizes/max"/></xsl:attribute></xsl:if>
        <div class="dropped-file-icon">
          <img src="/images/editor/drag-n-drop-64x64.png" data-original="/images/editor/drag-n-drop-64x64.png"/>
        </div>
        <div class="dropped-file-filename-and-buttons">
          <div class="dropped-file-filename">
            <xsl:value-of select="tokenize($uri, '/')[last()]"/>
          </div>
          <div class="dropped-file-buttons">
            <xsl:if test="$action = 'edit'">
              <div class='hidden file-input'>
                <input type='file'>
                  <xsl:if test="(@capture or /editor-form/capture) and $action = 'edit'">
                    <xsl:attribute name="capture">capture</xsl:attribute>
                  </xsl:if>
                  <xsl:if test="filters/mime-types">
                    <xsl:attribute name="accept"><xsl:value-of select="string-join(filters/mime-types/mime, ',')"/></xsl:attribute>
                  </xsl:if>
                </input>
              </div>
              <div class="button upload"><u>U</u>pload</div>
            </xsl:if>
            <div><xsl:attribute name="class">button download<xsl:if test="not($uri != '')"> disabled</xsl:if></xsl:attribute><u>D</u>ownload</div>
            <xsl:if test="$action = 'edit'"><div class="button reset"><u>R</u>eset</div></xsl:if>
          </div>
        </div>
        <div class="end-dropped-file"/>
      </div>
      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>
  <xsl:template match="widget[@type='dropped-file']">
    <widget path="{@path}">
      <xsl:call-template name="snap:dropped-file">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
        <xsl:with-param name="type" select="@type"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- DROPPED FILE WITH PREVIEW WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:dropped-file-with-preview">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <xsl:param name="type"/>
    <div field_type="dropped-file-with-preview">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <!--
        Note:
        by default browse is expected to say yes, the programmer has to
        add browse="no" to not get the browse button
      -->
      <xsl:attribute name="class"><xsl:if
          test="$action = 'edit'">snap-editor </xsl:if>editable dropped-file-with-preview-box <xsl:value-of
          select="$name"/><xsl:if test="@drop or /editor-form/drop"> drop</xsl:if><xsl:if
          test="not(attachment[@browse='no'])"> browse</xsl:if><xsl:if
          test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
          test="$name = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:value-of
          select="concat(' ', classes)"/></xsl:attribute>
      <xsl:if test="background-value">
        <!-- by default "snap-editor-background" objects have "display: none"
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
        <!-- TBD: should we use "image" instead of "attachment" since we show images? -->
        <xsl:attribute name="class">editor-content dropped-file-with-preview<xsl:if
          test="$type = 'dropped-file-with-preview' or $type = 'dropped-any-with-preview'"> attachment</xsl:if><xsl:if
          test="$type = 'dropped-image-with-preview' or $type = 'dropped-any-with-preview'"> image</xsl:if><xsl:if
          test="@no-toolbar or /editor-form/no-toolbar"> no-toolbar</xsl:if><xsl:if
          test="state = 'disabled'"> disabled</xsl:if></xsl:attribute>
        <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
          <!-- put two attributes, that way we can set the tabindex to -1
               when disabling a screen -->
          <xsl:variable name="tabindex" select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/>
          <xsl:attribute name="tabindex"><xsl:value-of select="$tabindex"/></xsl:attribute>
          <xsl:attribute name="original_tabindex"><xsl:value-of select="$tabindex"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="tooltip">
          <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="sizes/min"><xsl:attribute name="min-sizes"><xsl:value-of select="sizes/min"/></xsl:attribute></xsl:if>
        <xsl:if test="sizes/resize"><xsl:attribute name="resize-sizes"><xsl:value-of select="sizes/resize"/></xsl:attribute></xsl:if>
        <xsl:if test="sizes/max"><xsl:attribute name="max-sizes"><xsl:value-of select="sizes/max"/></xsl:attribute></xsl:if>
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
        </xsl:choose>
      </div>
      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>
  <xsl:template match="widget[@type='dropped-file-with-preview' or @type='dropped-image-with-preview' or @type='dropped-any-with-preview']">
    <widget path="{@path}">
      <xsl:call-template name="snap:dropped-file-with-preview">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
        <xsl:with-param name="type" select="@type"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- IMAGE BOX WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:image-box">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <xsl:param name="value"/>
    <div field_type="image-box">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class"><xsl:if
          test="$action = 'edit'">snap-editor </xsl:if>editable image-box <xsl:value-of
          select="$name"/><xsl:if test="@drop or /editor-form/drop"> drop</xsl:if><xsl:if
          test="not(attachment[@browse='no'])"> browse</xsl:if><xsl:if
          test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
          test="$name = /editor-form/focus/@refid"> auto-focus</xsl:if> <xsl:value-of
          select="concat(' ', classes)"/></xsl:attribute>
      <xsl:attribute name="style"><xsl:if
          test="geometry/@width">width:<xsl:value-of select="geometry/@width"/>px;</xsl:if><xsl:if
          test="geometry/@height">height:<xsl:value-of select="geometry/@height"/>px;</xsl:if></xsl:attribute>
      <xsl:if test="background-value">
        <!-- by default "snap-editor-background" objects have "display: none"
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
        <xsl:attribute name="class">editor-content image no-toolbar<xsl:if test="state = 'disabled'"> disabled</xsl:if></xsl:attribute>
        <xsl:if test="geometry"><xsl:attribute name="style"><xsl:if
            test="geometry/@width">width:<xsl:value-of select="geometry/@width"/>px;</xsl:if><xsl:if
            test="geometry/@height">height:<xsl:value-of select="geometry/@height"/>px;</xsl:if></xsl:attribute></xsl:if>
        <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
          <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="tooltip">
          <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="sizes/min"><xsl:attribute name="min-sizes"><xsl:value-of select="sizes/min"/></xsl:attribute></xsl:if>
        <xsl:if test="sizes/resize"><xsl:attribute name="resize-sizes"><xsl:value-of select="sizes/resize"/></xsl:attribute></xsl:if>
        <xsl:if test="sizes/max"><xsl:attribute name="max-sizes"><xsl:value-of select="sizes/max"/></xsl:attribute></xsl:if>
        <!-- now the actual value of this line -->
        <xsl:copy-of select="$value"/>
      </div>
      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>
  <xsl:template match="widget[@type='image-box']">
    <xsl:variable name="value">
      <xsl:choose>
        <xsl:when test="post">
          <!-- use the post value when there is one, it has priority -->
          <xsl:copy-of select="post/node()"/>
        </xsl:when>
        <xsl:when test="value">
          <!-- use the current value when there is one -->
          <xsl:copy-of select="value/node()"/>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>
    <widget path="{@path}">
      <xsl:call-template name="snap:image-box">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
        <xsl:with-param name="value" select="$value"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- DROPDOWN WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:dropdown">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <xsl:param name="value"/>
    <div field_type="dropdown">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor </xsl:if>editable <xsl:value-of
        select="classes"/> dropdown <xsl:value-of select="$name"/><xsl:if
        test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
        test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:if
        test="state = 'disabled'"> disabled</xsl:if><xsl:if
        test="not(@mode) or @mode = 'select-only'"> read-only</xsl:if></xsl:attribute>
      <xsl:if test="dropdown-algorithm">
        <xsl:attribute name="data-algorithm"><xsl:value-of select="dropdown-algorithm"/></xsl:attribute>
      </xsl:if>
      <div class="snap-editor-dropdown-reset-value">
        <xsl:if test="default[@not-a-value]"><xsl:attribute name="data-not-a-value">not-a-value</xsl:attribute></xsl:if>
        <xsl:copy-of select="default/node()"/>
      </div>
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
      <div>
        <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class">editor-content<xsl:if
              test="@no-toolbar or ../no-toolbar"> no-toolbar</xsl:if></xsl:attribute>
        <xsl:if test="../taborder/tabindex[@refid=$name]">
          <xsl:variable name="tabindex" select="../taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/>
          <xsl:attribute name="tabindex"><xsl:value-of select="$tabindex"/></xsl:attribute>
          <xsl:attribute name="original_tabindex"><xsl:value-of select="$tabindex"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="tooltip">
          <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="not(@mode) or @mode = 'select-only'">
          <!-- avoid spellcheck of non-editable widgets -->
          <xsl:attribute name="spellcheck">false</xsl:attribute>
        </xsl:if>
        <xsl:call-template name="snap:text-field-filters"/>

        <!-- WARNING: the order of this xsl:choose is VERY important -->
        <xsl:choose>
          <!-- search for one @value that matches $value, this is the preferred method of selection -->
          <xsl:when test="$value != '' and preset/item[$value = @value]">
            <xsl:attribute name="value"><xsl:copy-of select="$value"/></xsl:attribute>
            <xsl:copy-of select="preset/item[$value = @value]/node()"/>
          </xsl:when>
          <!-- value is defined, use it... -->
          <xsl:when test="$value != ''">
            <!-- if we did not match an @value, then we assume there are none (TBD?) -->
            <xsl:copy-of select="$value"/>
          </xsl:when>
          <!-- programmer specified a default -->
          <xsl:when test="preset/item[@default = 'default']">
            <xsl:if test="preset/item[@default = 'default']/@value">
              <xsl:attribute name="value"><xsl:copy-of select="preset/item[@default = 'default']/@value"/></xsl:attribute>
            </xsl:if>
            <xsl:copy-of select="preset/item[@default = 'default']/node()"/>
          </xsl:when>
          <xsl:otherwise>
            <!-- otherwise stick the default value in there if there is one
                 this can be something completely different from what
                 appears in the list of items -->
            <xsl:copy-of select="default/node()"/>
          </xsl:otherwise>
        </xsl:choose>
      </div>
      <div>
        <xsl:choose>
          <xsl:when test="count(preset/item)">
            <!-- there are items, put them in a list for the dropdown -->
            <xsl:attribute name="class">dropdown-items zordered</xsl:attribute>
            <ul class="dropdown-selection">

              <!-- WARNING: the order of this xsl:choose is VERY important -->
              <xsl:choose>
                <!-- search for one @value that matches $value, this is the preferred method of selection -->
                <xsl:when test="$value != '' and preset/item[$value = @value]">
                  <xsl:for-each select="preset/item">
                    <li>
                      <xsl:attribute name="class">dropdown-item<xsl:if
                          test="$value = @value"> selected</xsl:if><xsl:if
                          test="@class"> dropdown-item-classes <xsl:value-of select="@class"/></xsl:if></xsl:attribute>
                      <xsl:if test="@value"><xsl:attribute name="value"><xsl:value-of select="@value"/></xsl:attribute></xsl:if>
                      <xsl:copy-of select="./node()"/>
                    </li>
                  </xsl:for-each>
                </xsl:when>
                <!-- value is defined, use it... -->
                <xsl:when test="$value != ''">
                  <!-- if we did not match an @value, then we assume there are none (TBD?) -->
                  <xsl:for-each select="preset/item">
                    <li>
                      <xsl:attribute name="class">dropdown-item<xsl:if
                          test="@class"> dropdown-item-classes <xsl:value-of select="@class"/></xsl:if></xsl:attribute>
                      <xsl:if test="@value"><xsl:attribute name="value"><xsl:value-of select="@value"/></xsl:attribute></xsl:if>
                      <xsl:copy-of select="./node()"/>
                    </li>
                  </xsl:for-each>
                </xsl:when>
                <!-- programmer specified a default -->
                <xsl:when test="preset/item[@default = 'default']">
                  <xsl:for-each select="preset/item">
                    <li>
                      <xsl:attribute name="class">dropdown-item<xsl:if
                          test="@default = 'default'"> selected</xsl:if><xsl:if
                          test="@class"> dropdown-item-classes <xsl:value-of select="@class"/></xsl:if></xsl:attribute>
                      <xsl:if test="@value"><xsl:attribute name="value"><xsl:value-of select="@value"/></xsl:attribute></xsl:if>
                      <xsl:copy-of select="./node()"/>
                    </li>
                  </xsl:for-each>
                </xsl:when>
                <xsl:otherwise>
                  <!-- otherwise use the default node -->
                  <xsl:for-each select="preset/item">
                    <li>
                      <xsl:attribute name="class">dropdown-item<xsl:if
                          test="$value = text() or ../../default/node() = node()"> selected</xsl:if><xsl:if
                          test="@class"> dropdown-item-classes <xsl:value-of select="@class"/></xsl:if></xsl:attribute>
                      <xsl:if test="@value"><xsl:attribute name="value"><xsl:value-of select="@value"/></xsl:attribute></xsl:if>
                      <xsl:copy-of select="./node()"/>
                    </li>
                  </xsl:for-each>
                </xsl:otherwise>
              </xsl:choose>

              <!--xsl:for-each select="preset/item">
                <li>
                  <xsl:attribute name="class">dropdown-item<xsl:if
                      test="@default = 'default' or $value = @value or $value = node() or ../../default/node() = node()"> selected</xsl:if><xsl:if
                      test="@class"> dropdown-item-classes <xsl:value-of select="@class"/></xsl:if></xsl:attribute>
                  <xsl:if test="@value"><xsl:attribute name="value"><xsl:value-of select="@value"/></xsl:attribute></xsl:if>
                  <xsl:copy-of select="./node()"/>
                </li>
              </xsl:for-each-->
            </ul>
          </xsl:when>
          <xsl:otherwise>
            <!-- there are no items, create a default placeholder for the dropdown -->
            <xsl:attribute name="class">dropdown-items zordered disabled</xsl:attribute>
            <!-- TODO: (1) translation, (2) let user choose what to put in there -->
            <div class="no-selection">No selection...</div>
          </xsl:otherwise>
        </xsl:choose>
      </div>
      <!--xsl:if test="required = 'required'"> <span class="required">*</span></xsl:if-->

      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>
  <xsl:template match="widget[@type='dropdown']">
    <xsl:variable name="value">
      <xsl:choose>
        <!-- use the post value when there is one, it has priority -->
        <xsl:when test="post"><xsl:copy-of select="post/node()"/></xsl:when>
        <!-- use the current value when there is one -->
        <xsl:when test="value"><xsl:copy-of select="value/node()"/></xsl:when>
      </xsl:choose>
    </xsl:variable>
    <widget path="{@path}">
      <xsl:call-template name="snap:dropdown">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
        <xsl:with-param name="value" select="$value"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- CHECKMARK WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:checkmark">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <xsl:param name="value"/>
    <div field_type="checkmark">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor </xsl:if>editable <xsl:value-of
        select="classes"/> checkmark <xsl:value-of select="$name"/><xsl:if
        test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
        test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:if
        test="state = 'disabled'"> disabled</xsl:if></xsl:attribute>
      <!-- never use toolbar with checkmarks -->
      <div>
        <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class">editor-content no-toolbar</xsl:attribute>
        <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
          <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="tooltip">
          <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
        </xsl:if>

        <div class="checkmark-flag">
          <div class="flag-box"></div>
          <!-- the actual value of a checkmark is used to know whether the checkmark is shown or not -->
          <div><xsl:attribute name="class">checkmark-area<xsl:if test="$value != '0'"> checked</xsl:if></xsl:attribute></div>
        </div>

        <xsl:copy-of select="label/node()"/>
        <xsl:if test="required = 'required'"> <span class="required">*</span></xsl:if>
      </div>
      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>
  <xsl:template match="widget[@type='checkmark']">
    <xsl:variable name="value">
      <xsl:choose>
        <xsl:when test="post">
          <!-- use the post value when there is one, it has priority -->
          <xsl:copy-of select="post/node()"/>
        </xsl:when>
        <xsl:when test="value">
          <!-- use the current value when there is one -->
          <xsl:copy-of select="value/node()"/>
        </xsl:when>
        <xsl:otherwise>0</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <widget path="{@path}">
      <xsl:call-template name="snap:checkmark">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
        <xsl:with-param name="value" select="$value"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- RADIO BUTTONS WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:radio_buttons">
    <xsl:param name="name"/>
    <xsl:param name="pos"/>
    <ul class="radion-buttons">
      <xsl:for-each select="preset/item">
        <li>
          <!-- the actual value of a radio is used to know whether the radio is selected or not -->
          <!-- TODO: we would need to determine the default FIRST by testing the @value then by testing the node() in two separate loops -->
          <xsl:attribute name="class">radio-button<xsl:if test="position() = number($pos)"> selected</xsl:if><xsl:if
              test="@class"> radio-button-classes <xsl:value-of select="@class"/></xsl:if></xsl:attribute>
          <xsl:if test="@value"><xsl:attribute name="value"><xsl:value-of select="@value"/></xsl:attribute></xsl:if>
          <!-- TODO: here we've got a problem since this is viewed as
                     ONE unit but if we have 10 items, we would need
                     this to be viewed as 10... -->
          <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
            <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/></xsl:attribute>
          </xsl:if>
          <div class="radio-flag-box"></div>
          <div class="radio-area">
            <xsl:copy-of select="./node()"/>
          </div>
          <!-- in most cases we use the end-radio block to do a "clear: both;" -->
          <div class="end-radio-item"></div>
        </li>
      </xsl:for-each>
    </ul>
  </xsl:template>
  <xsl:template name="snap:radio">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <xsl:param name="value"/>
    <div field_type="radio">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor </xsl:if>editable <xsl:value-of
        select="classes"/> radio <xsl:value-of select="$name"/><xsl:if
        test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
        test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:if
        test="state = 'disabled'"> disabled</xsl:if></xsl:attribute>
      <!-- never use toolbar with radio buttons -->
      <div>
        <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class">editor-content no-toolbar</xsl:attribute>
        <xsl:if test="tooltip">
          <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
        </xsl:if>
        <xsl:choose>
          <xsl:when test="$value != ''">
            <xsl:attribute name="value"><xsl:value-of select="$value"/></xsl:attribute>
          </xsl:when>
          <xsl:when test="preset/item[@default = 'default']">
            <!-- use the default value when there is one -->
            <xsl:copy-of select="preset/item[@default = 'default']/@value"/>
          </xsl:when>
        </xsl:choose>

        <!-- WARNING: the order is VERY important -->
        <xsl:variable name="default_position">
          <xsl:for-each select="preset/item">
            <!-- search for one @value that matches $value, this is the preferred method of selection -->
            <xsl:if test="$value != '' and $value = @value">
              <p><xsl:value-of select="position()"/></p>
            </xsl:if>
          </xsl:for-each>
          <xsl:for-each select="preset/item">
            <!-- maybe it is the current value -->
            <xsl:if test="$value != '' and $value = node()">
              <p><xsl:value-of select="position()"/></p>
            </xsl:if>
          </xsl:for-each>
          <xsl:for-each select="preset/item">
            <!-- programmer specified a default -->
            <xsl:if test="@default = 'default'">
              <p><xsl:value-of select="position()"/></p>
            </xsl:if>
          </xsl:for-each>
          <p>-1</p>
        </xsl:variable>

        <xsl:call-template name="snap:radio_buttons">
          <xsl:with-param name="name" select="@id"/>
          <xsl:with-param name="pos" select="$default_position/p[1]/node()"/>
        </xsl:call-template>
        <!-- in most cases we use the end-radio block to do a "clear: both;" -->
        <div class="end-radio"></div>

      </div>
      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>
  <xsl:template match="widget[@type='radio']">
    <xsl:variable name="value">
      <xsl:choose>
        <xsl:when test="post">
          <!-- use the post value when there is one, it has priority -->
          <xsl:copy-of select="post/node()"/>
        </xsl:when>
        <xsl:when test="value">
          <!-- use the current value when there is one -->
          <xsl:copy-of select="value/node()"/>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>
    <widget path="{@path}">
      <xsl:call-template name="snap:radio">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
        <xsl:with-param name="value" select="$value"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- TEXT EDIT WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:text-edit">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <div field_type="text-edit">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class"><xsl:if
            test="$action = 'edit'">snap-editor </xsl:if>editable text-edit <xsl:value-of select="$name"/><xsl:if
            test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
            test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:value-of
            select="concat(' ', classes)"/><xsl:if
            test="state = 'disabled'"> disabled</xsl:if><xsl:if
            test="state = 'read-only'"> read-only</xsl:if></xsl:attribute>
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
          <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
        </xsl:if>
        <xsl:call-template name="snap:text-field-filters"/>
        <!-- now the actual value of this text widget -->
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
  </xsl:template>
  <xsl:template match="widget[@type='text-edit']">
    <widget path="{@path}">
      <xsl:call-template name="snap:text-edit">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- LINE EDIT WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:line-edit">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <div field_type="line-edit">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor </xsl:if>editable line-edit <xsl:value-of
        select="$name"/><xsl:if test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
        test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:value-of
        select="concat(' ', classes)"/><xsl:if test="state = 'disabled'"> disabled</xsl:if><xsl:if
        test="state = 'read-only'"> read-only</xsl:if></xsl:attribute>
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
          <xsl:when test="default">
            <!-- use the default value when there is one -->
            <xsl:copy-of select="default/node()"/>
          </xsl:when>
        </xsl:choose>
      </div>
      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>
  <xsl:template match="widget[@type='line-edit']">
    <widget path="{@path}">
      <xsl:call-template name="snap:line-edit">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- BUTTON WIDGET (an anchor) -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:button">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <a field_type="button">
      <!-- name required as a field name? xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute-->
      <xsl:attribute name="class"><xsl:value-of select="classes"/> button <xsl:value-of
        select="$name"/><xsl:if test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:if
        test="state = 'disabled'"> disabled</xsl:if></xsl:attribute>
      <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
        <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="tooltip">
        <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
      </xsl:if>
      <xsl:attribute name="href"><xsl:copy-of select="value/node()"/></xsl:attribute>
      <!-- use the label as the anchor text -->
      <xsl:copy-of select="label/node()"/>
    </a>
    <xsl:call-template name="snap:common-parts"/>
  </xsl:template>
  <xsl:template match="widget[@type='button']">
    <widget path="{@path}">
      <xsl:call-template name="snap:button">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- CUSTOM ("user" data, we just copy the <value> tag over) -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:custom">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <div field_type="custom" style="display: none;">
      <xsl:attribute name="class"><xsl:value-of select="classes"/> snap-editor-custom <xsl:value-of select="$name"/></xsl:attribute>
      <div class="snap-content">
        <xsl:copy-of select="value/node()"/>
      </div>
    </div>
  </xsl:template>
  <xsl:template match="widget[@type='custom']">
    <widget path="{@path}">
      <xsl:call-template name="snap:custom">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- SILENT (a value such as the editor form timeout and auto-reset,
              which is not returned to the editor on a Save) -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:silent">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <div field_type="silent">
      <xsl:attribute name="class"><xsl:value-of select="classes"/> snap-editor-silent <xsl:value-of select="$name"/></xsl:attribute>
      <div class="snap-content">
        <xsl:copy-of select="value/node()"/>
      </div>
    </div>
    <xsl:call-template name="snap:common-parts"/>
  </xsl:template>
  <xsl:template match="widget[@type='silent']">
    <widget path="{@path}">
      <xsl:call-template name="snap:silent">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- HIDDEN (a value hidden from the user and always returned on Save) -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:hidden">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <div field_type="hidden">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class"><xsl:value-of select="classes"/> snap-editor snap-editor-hidden <xsl:value-of select="$name"/></xsl:attribute>
      <div class="editor-content" name="{$name}">
        <xsl:copy-of select="value/node()"/>
      </div>
    </div>
    <xsl:call-template name="snap:common-parts"/>
  </xsl:template>
  <xsl:template match="widget[@type='hidden']">
    <widget path="{@path}">
      <xsl:call-template name="snap:hidden">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

  <!-- THE EDITOR FORM AS A WHOLE -->
  <xsl:template match="editor-form">
    <!--
      WARNING: remember that this transformation generates many tags in the
               output, each of which goes in a different place in the XML
               document representing your current page.
    -->
    <xsl:apply-templates select="widget"/>
  </xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2 et
-->
