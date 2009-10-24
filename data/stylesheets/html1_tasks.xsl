<?xml version="1.0"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml"
             xmlns:date="http://exslt.org/dates-and-times"
             xmlns:I18N="http://www.gnu.org/software/gettext/" extension-element-prefixes="I18N">

<!--
  Copyright (C) 2004-2005 Imendio AB
  Copyright (c) 2003 Daniel Lundin
  Copyright (c) 2003 CodeFactory AB
  Copyright (c) 2004 Chris Ladd (caladd@particlestorm.net)
-->

<xsl:template name="calculate-cost">
  <xsl:param name="std-rates"/>
  <xsl:param name="units"/>
  <xsl:param name="level"/>
  <xsl:param name="work"/>
  
  <xsl:choose>
    <xsl:when test="$level = 0">0</xsl:when>
    <xsl:otherwise>
      <xsl:variable name="cost">
        <xsl:call-template name="calculate-cost">
          <xsl:with-param name="std-rates" select="$std-rates"/>
          <xsl:with-param name="units" select="$units"/>
          <xsl:with-param name="level" select="$level - 1"/>
          <xsl:with-param name="work" select="$work"/>
        </xsl:call-template>
      </xsl:variable>
      
      <xsl:variable name="std-rate" select="$std-rates[position()=$level]"/>
      <xsl:variable name="unit" select="$units[position()=$level] div 100"/>
      <xsl:variable name="totalunits" select="sum($units) div 100"/>
      <xsl:value-of select="($std-rate * $unit * $work) div $totalunits + $cost"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="tasks">
  <xsl:variable name="hasproperties" select="boolean (count(//task/properties/property[@value!='']))"/>
  <xsl:variable name="hasnotes" select="boolean (count(//task[@note!='']))"/>

  <div class="tasklist">
  <h2><a name="tasks"><xsl:value-of select="I18N:gettext('Tasks')"/></a></h2>

  <div class="tasklist-table">
  <table cellspacing="0" cellpadding="0" border="1">
    <tr class="header" align="left">
      <th><span><xsl:value-of select="I18N:gettext('WBS')"/></span></th>
      <th><span><xsl:value-of select="I18N:gettext('Name')"/></span></th>
      <th><span><xsl:value-of select="I18N:gettext('Start')"/></span></th>
      <th><span><xsl:value-of select="I18N:gettext('Finish')"/></span></th>
      <th><span><xsl:value-of select="I18N:gettext('Work')"/></span></th>
      <th><span><xsl:value-of select="I18N:gettext('Complete')"/></span></th>
      <th><span><xsl:value-of select="I18N:gettext('Cost')"/></span></th>
      <th><span><xsl:value-of select="I18N:gettext('Assigned to')"/></span></th>
      <xsl:if test="$hasnotes">
        <th class="note"><span>Notes</span></th>
      </xsl:if>
    </tr>
    <xsl:for-each select="//task">
      <xsl:sort data-type="number" 
        select="date:seconds (date:add (concat (substring (@end, 1, 4),'-',
                substring (@end, 5, 2),'-',
                substring (@end, 7, 2)), 
                date:duration (substring (@end, 10))))"
        order="descending"/>
        <xsl:variable name="tid" select="@id"/>
        <!-- Make a proper XSL date value from the start-date attribute -->
        <xsl:variable name="start_date">
          <xsl:call-template name="mrproj-parse-date">
            <xsl:with-param name="thedate" select="@work-start"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="end_date">
          <xsl:call-template name="mrproj-parse-date">
            <xsl:with-param name="thedate" select="@end"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="indent" select="count (ancestor::task)"/>
        <xsl:variable name="tname">        
        </xsl:variable>
        
        <xsl:variable name="rowclass">
          <xsl:choose>
            <xsl:when test="(position() mod 2) = 0">even</xsl:when>
            <xsl:otherwise>odd</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <xsl:variable name="rowstyle">
          <xsl:if test="task">
            font-weight: bold;
          </xsl:if>
        </xsl:variable>

        <tr class="{$rowclass}" style="{$rowstyle}">
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
            <a name="task{@id}" style="margin-left: {$indent*$task-indent-pixels}px">
              <span>
                <xsl:value-of select="@name"/>
              </span>
            </a>
          </td>
          <td>
            <span>
              <xsl:value-of select="date:month-abbreviation($start_date)"/>
              <xsl:text> </xsl:text>
              <xsl:value-of select="date:day-in-month($start_date)"/>
            </span>
          </td>
          <td>
            <span>
              <xsl:value-of select="date:month-abbreviation($end_date)"/>
              <xsl:text> </xsl:text>
              <xsl:value-of select="date:day-in-month($end_date)"/>
            </span>
          </td>
          <td>
            <xsl:if test="@type!='milestone'">
              <span>
                <xsl:call-template name="mrproj-duration">
                  <xsl:with-param name="duration-in-seconds" select="@work"/>
                </xsl:call-template>
              </span>
            </xsl:if>
          </td>
          <td>
            <!-- if the task has no children and isn't a milestone -->
            <xsl:if test="not(task) and @type!='milestone'">
              <span>
                <xsl:value-of select="@percent-complete"/>%
              </span>
            </xsl:if>
          </td>
          <td>
            <span>
              <xsl:variable name="std-rates" select="/project/resources/resource[@id=/project/allocations/allocation[@task-id=$tid]/@resource-id]/@std-rate"/>
              <xsl:variable name="units" select="/project/allocations/allocation[@task-id=$tid]/@units"/>
              <xsl:variable name="cost">
                <xsl:call-template name="calculate-cost">
                  <xsl:with-param name="std-rates" select="$std-rates"/>
                  <xsl:with-param name="units" select="$units"/>
                  <xsl:with-param name="level" select="count($std-rates)"/>
                  <xsl:with-param name="work" select="@work div 3600"/>
                </xsl:call-template>
              </xsl:variable>
              <xsl:if test="not($cost = 0)">
                <xsl:value-of select="format-number($cost, '###,###,###,###.##')"/>
              </xsl:if>
            </span>
          </td>
          <td>
            <xsl:call-template name="mrproj-assigned-resources">
              <xsl:with-param name="task-id" select="$tid"/>
            </xsl:call-template>
          </td>
          <xsl:if test="$hasnotes">
            <td class="note">
              <span>
                <xsl:value-of select="@note"/>
              </span>
            </td>
          </xsl:if>
        </tr>

    </xsl:for-each>
  </table>
  </div>
  </div>
</xsl:template>
</xsl:stylesheet>
