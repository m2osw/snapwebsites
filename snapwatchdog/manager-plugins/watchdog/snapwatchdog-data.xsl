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
                xmlns:html="http://www.w3.org/TR/REC-html40">

  <xsl:output method="html" encoding="utf-8" indent="yes"/>

  <xsl:template match="/">
    <html>
      <head>
        <title>Snap! Watchdog Data</title>
      </head>
      <body>
        <!--
        some of the styles are here to actually cancel the defaults
        from the snapmanagercgi.css file
        -->
        <style>
          .watchdog-data
          {
            background-color: white;
            border: 1px solid #444;
            margin: 0 20px 10px 20px;
            padding: 10px;
          }
          #error-section h2
          {
            margin-top: 0;
          }
          table
          {
            border: none;
          }
          table th,
          table td
          {
            background-color: transparent;
            border: none;
          }
          table.name-value
          {
            border-spacing: 0;
            border-collapse: separate;
          }
          table.name-value th
          {
            text-align: right;
          }
          table.name-value th,
          table.name-value td
          {
            border-top: 1px solid black;
            margin: 0;
            padding: 5px;
          }
          table.name-value tr:first-child th,
          table.name-value tr:first-child td
          {
            border-top: none;
          }
          table.table-with-borders
          {
            border-left: 1px solid black;
            border-top: 1px solid black;
            border-spacing: 0;
            border-collapse: separate;
          }
          table.table-with-borders th
          {
            background-color: #e0d8e8;
          }
          table.table-with-borders th,
          table.table-with-borders td
          {
            border-right: 1px solid black;
            border-bottom: 1px solid black;
            margin: 0;
            padding: 5px;
          }
          table td.align-right
          {
            text-align: right;
          }
          table td.align-center
          {
            text-align: center;
          }
        </style>
        <h1>Snap! Watchdog Data (<xsl:value-of select="xs:dateTime('1970-01-01T00:00:00') + watchdog/@date div 1000 * xs:dayTimeDuration('PT0.001S')"/>)</h1>

        <div class="watchdog-data">

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
                  <xsl:for-each select="watchdog/error/message">
                    <tr>
                      <td><xsl:value-of select="@plugin_name"/></td>
                      <td class="align-right"><xsl:value-of select="@priority"/></td>
                      <td><xsl:copy-of select="node()"/></td>
                    </tr>
                  </xsl:for-each>
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
                  <th>Time of Boot:</th>
                  <td><xsl:value-of select="xs:dateTime('1970-01-01T00:00:00') + watchdog/cpu/@time_of_boot div 1 * xs:dayTimeDuration('PT1S')"/></td>
                </tr>
                <tr>
                  <th>Up Time:</th>
                  <td><xsl:value-of select="watchdog/cpu/@uptime"/></td>
                </tr>
                <tr>
                  <th>CPU Count:</th>
                  <td><xsl:value-of select="watchdog/cpu/@cpu_count"/></td>
                </tr>
                <tr>
                  <th>CPU Freq:</th>
                  <td><xsl:value-of select="watchdog/cpu/@cpu_freq"/></td>
                </tr>
                <tr>
                  <th>Idle:</th>
                  <td><xsl:value-of select="watchdog/cpu/@idle"/></td>
                </tr>
                <tr>
                  <th>Total CPU System:</th>
                  <td><xsl:value-of select="watchdog/cpu/@total_cpu_system"/></td>
                </tr>
                <tr>
                  <th>Total CPU User:</th>
                  <td><xsl:value-of select="watchdog/cpu/@total_cpu_user"/></td>
                </tr>
                <tr>
                  <th>Total CPU Wait:</th>
                  <td><xsl:value-of select="watchdog/cpu/@total_cpu_wait"/></td>
                </tr>
                <tr>
                  <th>Avg 1 min.:</th>
                  <td><xsl:value-of select="watchdog/cpu/@avg1"/></td>
                </tr>
                <tr>
                  <th>Avg 5 min.:</th>
                  <td><xsl:value-of select="watchdog/cpu/@avg5"/></td>
                </tr>
                <tr>
                  <th>Avg 15 min.:</th>
                  <td><xsl:value-of select="watchdog/cpu/@avg15"/></td>
                </tr>
                <tr>
                  <th>Page Cache In:</th>
                  <td><xsl:value-of select="watchdog/cpu/@page_cache_in"/></td>
                </tr>
                <tr>
                  <th>Page Cache Out:</th>
                  <td><xsl:value-of select="watchdog/cpu/@page_cache_out"/></td>
                </tr>
                <tr>
                  <th>Swap Cache In:</th>
                  <td><xsl:value-of select="watchdog/cpu/@swap_cache_in"/></td>
                </tr>
                <tr>
                  <th>Swap Cache Out:</th>
                  <td><xsl:value-of select="watchdog/cpu/@swap_cache_out"/></td>
                </tr>
                <tr>
                  <th>Total Processes:</th>
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
                  <th>Total Memory:</th>
                  <td><xsl:value-of select="watchdog/memory/@mem_total"/></td>
                </tr>
                <tr>
                  <th>Available Memory:</th>
                  <td><xsl:value-of select="watchdog/memory/@mem_available"/></td>
                </tr>
                <tr>
                  <th>Free Memory:</th>
                  <td><xsl:value-of select="watchdog/memory/@mem_free"/></td>
                </tr>
                <tr>
                  <th>Memory Buffers:</th>
                  <td><xsl:value-of select="watchdog/memory/@mem_buffers"/></td>
                </tr>
                <tr>
                  <th>Memory Cached:</th>
                  <td><xsl:value-of select="watchdog/memory/@mem_cached"/></td>
                </tr>
                <tr>
                  <th>Swap Total Space:</th>
                  <td><xsl:value-of select="watchdog/memory/@swap_total"/></td>
                </tr>
                <tr>
                  <th>Free Swap Space:</th>
                  <td><xsl:value-of select="watchdog/memory/@swap_free"/></td>
                </tr>
                <tr>
                  <th>Swap Space Cached:</th>
                  <td><xsl:value-of select="watchdog/memory/@swap_cached"/></td>
                </tr>
              </table>
            </div>
          </xsl:if>

          <!-- Disk info -->
          <xsl:variable name="disk_count" select="count(watchdog/disk)"/>
          <xsl:if test="$disk_count > 0">
            <div id="disk-section">
              <h2>Disk Partitions</h2>
<p>(TODO convert to readable date &amp; sizes)</p>
              <table class="table-with-borders">
                <thead>
                  <tr>
                    <th>Path</th>
                    <th>Free Space</th>
                    <th>Block Free</th>
                    <th>Blocks</th>
                    <th>Flags</th>
                    <th>Free Files</th>
                    <th>Files Available</th>
                    <th>Error</th>
                  </tr>
                </thead>
                <tbody>
                  <xsl:for-each select="watchdog/disk/partition">
                    <tr>
                      <td><xsl:value-of select="@dir"/></td>
                      <td class="align-right"><xsl:value-of select="@available"/></td>
                      <td class="align-right"><xsl:value-of select="@bfree"/></td>
                      <td class="align-right"><xsl:value-of select="@blocks"/></td>
                      <td class="align-right"><xsl:value-of select="@flags"/></td>
                      <td class="align-right"><xsl:value-of select="@ffree"/></td>
                      <td class="align-right"><xsl:value-of select="@favailable"/></td>
                      <td><xsl:value-of select="@error"/></td>
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

          <!-- firewall has a process when running -->
          <xsl:variable name="firewall_count" select="count(watchdog/firewall)"/>
          <xsl:if test="$firewall_count > 0">
            <div id="firewall-section">
              <h2>Firewall</h2>
              <table class="table-with-borders">
                <xsl:call-template name="process_table_header"/>
                <tbody>
                  <xsl:apply-templates select="watchdog/firewall/process"/>
                </tbody>
              </table>
            </div>
          </xsl:if>

          <!-- network has a process when running -->
          <xsl:variable name="network_count" select="count(watchdog/network)"/>
          <xsl:if test="$network_count > 0">
            <div id="network-section">
              <h2>Network</h2>
              <table class="table-with-borders">
                <xsl:call-template name="process_table_header"/>
                <tbody>
                  <xsl:apply-templates select="watchdog/network/process"/>
                </tbody>
              </table>
            </div>
          </xsl:if>

          <!-- processes has a process when running -->
          <xsl:variable name="processes_count" select="count(watchdog/processes)"/>
          <xsl:if test="$processes_count > 0">
            <div id="processes-section">
              <h2>Processes</h2>
              <table class="table-with-borders">
                <xsl:call-template name="process_table_header"/>
                <tbody>
                  <xsl:apply-templates select="watchdog/processes/process"/>
                </tbody>
              </table>
            </div>
          </xsl:if>

          <!-- watchdscripts a set of results, one per script -->
          <xsl:variable name="watchscripts_count" select="count(watchdog/watchscripts)"/>
          <xsl:if test="$watchscripts_count > 0">
            <div id="watchscripts-section">
              <h2>Watchscripts</h2>
              <table class="table-with-borders">
                <thead>
                  <tr>
                    <th>Script Path</th>
                    <th>Exit Code</th>
                  </tr>
                </thead>
                <tbody>
                  <xsl:for-each select="watchdog/watchscripts/script">
                    <tr>
                      <td>
                        <xsl:value-of select="@name"/>
                        <xsl:if test="output or error">
                          <br/>
                          <br/>
                          <xsl:for-each select="tokenize(output, '\n')">
                            <xsl:copy-of select="."/>
                            <br/>
                          </xsl:for-each>
                          <xsl:for-each select="tokenize(error, '\n')">
                            <xsl:copy-of select="."/>
                            <br/>
                          </xsl:for-each>
                        </xsl:if>
                      </td>
                      <td class="align-right"><xsl:value-of select="@exit_code"/></td>
                    </tr>
                  </xsl:for-each>
                </tbody>
              </table>
            </div>
          </xsl:if>

        </div>
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
        <th>Error</th>
      </tr>
    </thead>
  </xsl:template>

  <xsl:template match="process">
    <tr>
      <xsl:attribute name="class">
        process
        <xsl:if test="@error">
          error
        </xsl:if>
        <xsl:if test="@tty != 0">
          tty
        </xsl:if>
      </xsl:attribute>
      <td><xsl:value-of select="@name"/></td>
      <td class="align-right"><xsl:value-of select="@resident"/></td>
      <td class="align-right"><xsl:value-of select="@pcpu"/></td>
      <td class="align-right"><xsl:value-of select="@utime"/></td>
      <td class="align-right"><xsl:value-of select="@utime"/></td>
      <td class="align-right"><xsl:value-of select="@stime"/></td>
      <td class="align-right"><xsl:value-of select="@cstime"/></td>
      <td class="align-right"><xsl:value-of select="@total_size"/></td>
      <td class="align-center"><xsl:if test="@tty != 0">Yes</xsl:if></td>
      <td><xsl:value-of select="@error"/></td>
    </tr>
  </xsl:template>

</xsl:stylesheet>
<!--
vim: ts=2 sw=2 et
-->
