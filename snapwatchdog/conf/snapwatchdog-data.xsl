<?xml version="1.0" encoding="UTF-8"?>
<!--
Snap Websites Server == XSLT to transform our XML Watchdog data files to HTML
Copyright (c) 2018  Made to Order Software Corp.  All Rights Reserved

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
<xsl:stylesheet version="2.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:xs="http://www.w3.org/2001/XMLSchema"
                xmlns:fn="http://www.w3.org/2005/xpath-functions"
                xmlns:html="http://www.w3.org/TR/REC-html40"
                xmlns:watchdog="https://snapwebsites.org/schemas/watchdog/0.1">

  <xsl:output method="html" encoding="utf-8" indent="yes"/>

  <xsl:template match="/">
    <html>
      <head>
        <title>Snap! Watchdog Data</title>
      </head>
      <body>
        <h1>Snap! Watchdog Data</h1>

        <!-- handle the errors first so it's more prominent -->
        <xsl:variable name="error_count" select="count(watchdog/error)"/>
        <xsl:if test="$error_count > 0">
          <div id="error-section" style="border: 1px solid #880000; background-color: #fff0f0; padding: 10px;">
            <h2 style="color: red;">Found <xsl:value-of select="$error_count"/> Error<xsl:if test="$error_count != 1">s</xsl:if></h2>
            <table id="error-table" class="table-with-borders">
              <thead>
                <tr>
                  <th>Plugin</th>
                  <th>Priority</th>
                  <th>Error Message</th>
                </tr>
              </thead>
              <tbody>
                <xsl:for-each select="watchdog/error">
                  <tr>
                    <td><xsl:value-of select="message/@plugin_name"/></td>
                    <td><xsl:value-of select="message/@priority"/></td>
                    <td><xsl:value-of select="message/node()"/></td>
                  </tr>
                </xsl:for-each>
              </tbody>
            </table>
          </div>
        </xsl:if>

        <!-- cassandra has a process when running -->
        <xsl:variable name="cassandra_count" select="count(watchdog/cassandra)"/>
        <xsl:if test="$cassandra_count > 0">
          <div id="cassandra-section">
            <h2>Cassandra</h2>
            <table class="table-with-borders">
              <xsl:call-template name="process_table_header"/>
              <tbody>
                <xsl:apply-templates select="watchdog/cassandra/process"/>
              </tbody>
            </table>
          </div>
        </xsl:if>

        <!-- CPU info -->
        <xsl:variable name="cpu_count" select="count(watchdog/cpu)"/>
        <xsl:if test="$cpu_count > 0">
          <div id="cpu-section">
            <h2>CPU</h2>
<p>(TODO convert to readable date &amp; sizes)</p>
            <table class="name-value">
              <tr>
                <td>Time of Boot:</td>
                <td><xsl:value-of select="watchdog/cpu/@time_of_boot"/></td>
              </tr>
              <tr>
                <td>Up Time:</td>
                <td><xsl:value-of select="watchdog/cpu/@uptime"/></td>
              </tr>
              <tr>
                <td>CPU Count:</td>
                <td><xsl:value-of select="watchdog/cpu/@cpu_count"/></td>
              </tr>
              <tr>
                <td>CPU Freq:</td>
                <td><xsl:value-of select="watchdog/cpu/@cpu_freq"/></td>
              </tr>
              <tr>
                <td>Idle:</td>
                <td><xsl:value-of select="watchdog/cpu/@idle"/></td>
              </tr>
              <tr>
                <td>Total CPU System:</td>
                <td><xsl:value-of select="watchdog/cpu/@total_cpu_system"/></td>
              </tr>
              <tr>
                <td>Total CPU User:</td>
                <td><xsl:value-of select="watchdog/cpu/@total_cpu_user"/></td>
              </tr>
              <tr>
                <td>Total CPU Wait:</td>
                <td><xsl:value-of select="watchdog/cpu/@total_cpu_wait"/></td>
              </tr>
              <tr>
                <td>Avg 1 min.:</td>
                <td><xsl:value-of select="watchdog/cpu/@avg1"/></td>
              </tr>
              <tr>
                <td>Avg 5 min.:</td>
                <td><xsl:value-of select="watchdog/cpu/@avg5"/></td>
              </tr>
              <tr>
                <td>Avg 15 min.:</td>
                <td><xsl:value-of select="watchdog/cpu/@avg15"/></td>
              </tr>
              <tr>
                <td>Page Cache In:</td>
                <td><xsl:value-of select="watchdog/cpu/@page_cache_in"/></td>
              </tr>
              <tr>
                <td>Page Cache Out:</td>
                <td><xsl:value-of select="watchdog/cpu/@page_cache_out"/></td>
              </tr>
              <tr>
                <td>Swap Cache In:</td>
                <td><xsl:value-of select="watchdog/cpu/@swap_cache_in"/></td>
              </tr>
              <tr>
                <td>Swap Cache Out:</td>
                <td><xsl:value-of select="watchdog/cpu/@swap_cache_out"/></td>
              </tr>
              <tr>
                <td>Total Processes:</td>
                <td><xsl:value-of select="watchdog/cpu/@total_processes"/></td>
              </tr>
            </table>
          </div>
        </xsl:if>

        <!-- Memory info -->
        <xsl:variable name="memory_count" select="count(watchdog/memory)"/>
        <xsl:if test="$memory_count > 0">
          <div id="memory-section">
            <h2>Memory</h2>
<p>(TODO convert to readable date &amp; sizes)</p>
            <table class="name-value">
              <tr>
                <td>Total Memory:</td>
                <td><xsl:value-of select="watchdog/memory/@mem_total"/></td>
              </tr>
              <tr>
                <td>Available Memory:</td>
                <td><xsl:value-of select="watchdog/memory/@mem_available"/></td>
              </tr>
              <tr>
                <td>Free Memory:</td>
                <td><xsl:value-of select="watchdog/memory/@mem_free"/></td>
              </tr>
              <tr>
                <td>Memory Buffers:</td>
                <td><xsl:value-of select="watchdog/memory/@mem_buffers"/></td>
              </tr>
              <tr>
                <td>Memory Cached:</td>
                <td><xsl:value-of select="watchdog/memory/@mem_cached"/></td>
              </tr>
              <tr>
                <td>Swap Total Space:</td>
                <td><xsl:value-of select="watchdog/memory/@swap_total"/></td>
              </tr>
              <tr>
                <td>Free Swap Space:</td>
                <td><xsl:value-of select="watchdog/memory/@swap_free"/></td>
              </tr>
              <tr>
                <td>Swap Space Cached:</td>
                <td><xsl:value-of select="watchdog/memory/@swap_cached"/></td>
              </tr>
            </table>
          </div>
        </xsl:if>
      </body>
    </html>
  </xsl:template>

  <xsl:template name="process_table_header">
    <thead>
      <tr>
        <th>Name</th>
        <th>Resident</th>
        <th>PCPU</th>
        <th>UTime</th>
        <th>CUTime</th>
        <th>STime</th>
        <th>CSTime</th>
        <th>TSize</th>
        <th>TTY</th>
      </tr>
    </thead>
  </xsl:template>

  <xsl:template match="process">
    <tr>
      <td><xsl:value-of select="@name"/></td>
      <td><xsl:value-of select="@resident"/></td>
      <td><xsl:value-of select="@pcpu"/></td>
      <td><xsl:value-of select="@utime"/></td>
      <td><xsl:value-of select="@utime"/></td>
      <td><xsl:value-of select="@stime"/></td>
      <td><xsl:value-of select="@cstime"/></td>
      <td><xsl:value-of select="@total_size"/></td>
      <td><xsl:if test="@tty != 0">Yes</xsl:if></td>
    </tr>
  </xsl:template>

</xsl:stylesheet>
<!--
vim: ts=2 sw=2 et
-->
