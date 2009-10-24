<?xml version="1.0"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml"
             xmlns:date="http://exslt.org/dates-and-times"
             xmlns:I18N="http://www.gnu.org/software/gettext/" extension-element-prefixes="I18N">

<!--
  Copyright (c) 2004-2005 Imendio AB
  Copyright (c) 2003 CodeFactory AB
  Copyright (c) 2003 Daniel Lundin
  Copyright (c) 2004 Chris Ladd (caladd@particlestorm.net)
-->

<xsl:template name="create-week-row">
  <xsl:param name="days"/>
  <xsl:param name="date"/>
  <xsl:choose>
    <xsl:when test="date:day-in-week($date) = 2 and $days >= 7">
      <th class="gantt-week-header" align="center" colspan="7">
        <!-- A week that crosses a year boundary is associated with the year that its thursday is in.
             This means that the year of any thursday date is always equal to the year of the week number.
             Because the date at this point is always a monday, we can add 3 days to get to the year for this week. -->
        <xsl:value-of select="I18N:gettext('Week')"/>&nbsp;<xsl:value-of select="date:week-in-year($date)"/>, <xsl:value-of select="date:year(date:add($date, date:duration(86400 * 3)))"/>
      </th>
      <xsl:if test="not($days = 7)">
        <xsl:call-template name="create-week-row">
          <xsl:with-param name="days" select="$days - 7"/>
          <xsl:with-param name="date" select="date:add($date, date:duration(604800))"/>
        </xsl:call-template>
      </xsl:if>
    </xsl:when>
    <xsl:when test="not($days >= 7)">
      <th class="gantt-{$days}day-header" colspan="{$days}"></th>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="colspan">
        <xsl:choose>
          <xsl:when test="date:day-in-week($date) = 1">1</xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="9 - date:day-in-week($date)"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <th class="gantt-{$colspan}day-header" colspan="{$colspan}"></th>
      <xsl:if test="$days > 1">
        <xsl:call-template name="create-week-row">
          <xsl:with-param name="days" select="$days - $colspan"/>
          <xsl:with-param name="date" select="date:add($date, date:duration(86400 * $colspan))"/>
        </xsl:call-template>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="create-day-row">
  <xsl:param name="days"/>
  <xsl:param name="date"/>
  <th class="gantt-day-header" align="center">
  <xsl:value-of select="date:day-in-month($date)"/>
  </th>
  <xsl:choose>
    <xsl:when test="$days > 1">
    <xsl:call-template name="create-day-row">
      <xsl:with-param name="days" select="$days - 1"/>
        <xsl:with-param name="date" select="date:add($date, date:duration(86400))"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <th align="center">
      </th>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="gantt">
  <div class="gantt">
  <h2><a name="gantt"><xsl:value-of select="I18N:gettext('Gantt Chart')"/></a></h2>
 
  <!-- add 7 days to make room for at least part of the resource names of tasks
       that end close to the end of the project -->
  <xsl:variable name="tmpdays" select="ceiling($projlength div 86400) + 7"/>

  <xsl:variable name="days">
    <xsl:choose>
      <xsl:when test="not($tmpdays >= 30)">30</xsl:when>
      <xsl:otherwise><xsl:value-of select="$tmpdays"/></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <div class="gantt-tasklist">
  <table cellspacing="0" cellpadding="0" border="1">
    <tr class="header" align="left">
      <th><span><xsl:value-of select="I18N:gettext('WBS')"/></span></th>
      <th><span><xsl:value-of select="I18N:gettext('Name')"/></span></th>
      <th><span><xsl:value-of select="I18N:gettext('Work')"/></span></th>
    </tr>
    <tr class="header">
      <th>&nbsp;</th>
      <th>&nbsp;</th>
      <th>&nbsp;</th>
    </tr>
    <xsl:for-each select="//project//task">
      <xsl:variable name="rowclass">
        <xsl:choose>
          <xsl:when test="(position() mod 2) = 0">even</xsl:when>
          <xsl:otherwise>odd</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      
      <xsl:variable name="indent" select="count(ancestor::task)"/>
      <xsl:variable name="start">
        <xsl:call-template name="mrproj-parse-date">
          <xsl:with-param name="thedate" select="@work-start"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="end">
        <xsl:call-template name="mrproj-parse-date">
          <xsl:with-param name="thedate" select="@end"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="task-start" select="floor(20 * date:seconds(date:difference($projstart, $start)) div 86400)"/>
      <xsl:variable name="task-end" select="floor(20 * date:seconds(date:difference($projstart, $end)) div 86400) - $task-start"/>
      <xsl:variable name="task-complete" select="floor($task-end * (@percent-complete div 100))"/>
      
      <tr class="{$rowclass}">
        <td>
          <span>
            <xsl:for-each select="ancestor-or-self::task">
              <xsl:value-of select="count(preceding-sibling::task) + 1"/>
              <xsl:if test="not(position() = last())">
                <xsl:text>.</xsl:text>
              </xsl:if>
            </xsl:for-each>
          </span>
        </td>
        <td>
          <xsl:choose>
            <!-- Task has subtasks -->
            <xsl:when test="task">
              <a name="task-{@id}" style="white-space: nowrap; font-weight: bold; margin-left: {$indent*$task-indent-pixels}px;">
                <span>
                  <xsl:value-of select="@name"/>
                </span>
              </a>
            </xsl:when>
            <!-- Task is leaf -->
            <xsl:otherwise>
              <a name="gantt-{@id}" style="white-space: nowrap; margin-left: {$indent*$task-indent-pixels}px;">
                <span>
                  <xsl:value-of select="@name"/>
                </span>
              </a>
            </xsl:otherwise>
          </xsl:choose>
        </td>

        <td>
          <xsl:choose>
            <!-- Task has subtasks -->
            <xsl:when test="task">
              <span style="white-space: nowrap; font-weight: bold;">
                <xsl:call-template name="mrproj-duration">
                  <xsl:with-param name="duration-in-seconds" select="@work"/>
                </xsl:call-template>
               </span>
            </xsl:when>
                        
            <!-- Task is leaf -->
            <xsl:otherwise>
              <span>
                <xsl:call-template name="mrproj-duration">
                  <xsl:with-param name="duration-in-seconds" select="@work"/>
                </xsl:call-template>
              </span>
            </xsl:otherwise>
          </xsl:choose>
        </td>
      </tr>
    </xsl:for-each>
  </table>
  </div>

  <div class="gantt-chart">
  <table cellspacing="0" cellpadding="0" border="1" style="table-layout: fixed;">
    <tr class="header" align="left">
      <xsl:call-template name="create-week-row">
        <xsl:with-param name="days" select="$days"/>
        <xsl:with-param name="date" select="$projstart"/>
      </xsl:call-template>
      <th></th>
    </tr>

    <tr class="header" align="left">
      <xsl:call-template name="create-day-row">
        <xsl:with-param name="days" select="$days"/>
        <xsl:with-param name="date" select="$projstart"/>
      </xsl:call-template>
    </tr>

    <xsl:for-each select="//project//task">
      <xsl:variable name="rowclass">
        <xsl:choose>
          <xsl:when test="(position() mod 2) = 0">even</xsl:when>
          <xsl:otherwise>odd</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      
      <xsl:variable name="indent" select="count(ancestor::task)"/>
      <xsl:variable name="start">
        <xsl:call-template name="mrproj-parse-date">
          <xsl:with-param name="thedate" select="@work-start"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="end">
        <xsl:call-template name="mrproj-parse-date">
          <xsl:with-param name="thedate" select="@end"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="task-start" select="floor(20 * date:seconds(date:difference($projstart, $start)) div 86400)"/>
      <xsl:variable name="task-end" select="floor(20 * date:seconds(date:difference($projstart, $end)) div 86400) - $task-start"/>
      <xsl:variable name="task-complete" select="floor($task-end * (@percent-complete div 100))"/>
      
      <tr class="{$rowclass}">
        <td colspan="{$days + 1}">
          <div style="width: {$days * 20 + 1}px; white-space: nowrap;">
            <xsl:if test="not (task)">
              <xsl:if test="$task-start > 0">
                <xsl:choose>
                  <xsl:when test="@type = 'milestone'">
                    <div class="gantt-empty-begin" style="width: {$task-start - 4}px;"></div>
                  </xsl:when>
                  <xsl:otherwise>
                    <div class="gantt-empty-begin" style="width: {$task-start}px;"></div>
                  </xsl:otherwise>
                 </xsl:choose>
              </xsl:if>
                          
              <xsl:if test="$task-end > 0">
                <div class="gantt-complete-notdone" style="width: {$task-end}px;">
                  <xsl:if test="$task-complete > 0">
                    <div class="gantt-complete-done" style="width: {$task-complete}px;"></div>
                  </xsl:if>
                </div>
              </xsl:if>

              <xsl:choose>
                <xsl:when test="@type = 'milestone'">
                  <div class="gantt-milestone">&#9670;</div>
                </xsl:when>
                <xsl:otherwise>
                  <div class="gantt-empty-end"></div>
                </xsl:otherwise>
              </xsl:choose>
              <div class="gantt-resources">
                <xsl:variable name="task-id" select="@id"/>
                <xsl:call-template name="mrproj-assigned-resources">
                  <xsl:with-param name="task-id" select="$task-id"/>
                </xsl:call-template>
              </div>
            </xsl:if>
          </div>
        </td>
      </tr>
    </xsl:for-each>
  </table>
  </div>
  </div>


</xsl:template>
</xsl:stylesheet>
