<?xml version="1.0"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml"
             xmlns:date="http://exslt.org/dates-and-times">

<!--**************************************************************************
    *
    * html1_gantt.xsl: Display a simplified (but useful?) gantt chart with CSS
    *
    * Copyright (c)2003 Daniel Lundin
    * Copyright (c)2003 CodeFactory AB
    *
    *-->

<xsl:template name="gantt">
  <xsl:param name='chartwidth' select="700"/> <!-- Default chart width -->

  <h2><a name="gantt">Simplified Gantt Chart</a></h2>

  <!-- Display for non-CSS browsers -->
  <div style="display: none">
    <hr/>
    <strong>*** Note: Gantt chart requires CSS capability. ***</strong>
    <hr/>
  </div>

  <xsl:variable name="sprojstart" select="-date:seconds ($projstart)"/>
  <xsl:variable name="s2px" select="$chartwidth div $projlength"/>
  <xsl:variable name="ttoday" select="$s2px * (date:seconds() + $sprojstart)"/>

  <table class="gantt" cellspacing="1" cellpadding="2" border="1">
    <tr class="gantt-hdr">
      <th align="left" class="tasks-hdr">Task</th>
      <th align="left" class="tasks-hdr">Schedule</th>
    </tr>

    <tr>
      <td align="right" class="gantt-dates">
          Dates:
      </td>
      <td class="gantt-timeline">
        <div class="gantt-date" style="width: {$chartwidth}px">
          <div class="gantt-today" style="left: {$ttoday}px">
            <span class="gantt-date-start">
              <xsl:text> </xsl:text>
<!--              <xsl:value-of select="date:day-in-month()"/> 
              <xsl:text> </xsl:text>
              <xsl:value-of select="date:month-abbreviation()"/>
              <xsl:text> </xsl:text>
              <xsl:value-of select="date:year()"/>
-->
            </span>          
          </div>
          <table border="0" cellspacing="0" cellpadding="0" width="{$chartwidth}">
            <tr>
              <td align="left">
                <span class="gantt-date-start">
                  <xsl:value-of select="date:day-in-month($projstart)"/> 
                  <xsl:text> </xsl:text>
                  <xsl:value-of select="date:month-abbreviation($projstart)"/>
                  <xsl:text> </xsl:text>
                  <xsl:value-of select="date:year($projstart)"/>
                </span>
              </td>
              <td align="right">
                <span class="gantt-date-end">
                  <xsl:variable name="penddate" 
                    select="date:add($projstart,date:duration ($projlength))"/>
                  <xsl:value-of select="date:day-in-month($penddate)"/> 
                  <xsl:text> </xsl:text>
                  <xsl:value-of select="date:month-abbreviation($penddate)"/>
                  <xsl:text> </xsl:text>
                  <xsl:value-of select="date:year($penddate)"/>
                </span>
              </td>
            </tr>
          </table>
        </div>
      </td>
    </tr>

    <xsl:for-each select="//project//task">
      <xsl:variable name="rowclass">
        <xsl:choose>
          <xsl:when  test="(position() mod 2) = 0">gantt-even</xsl:when>
          <xsl:otherwise>gantt-odd</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      
      <xsl:variable name="indent" select="count (ancestor::task)"/>
      
      <xsl:variable name="taskstart">
        <xsl:call-template name="mrproj-parse-date">
          <xsl:with-param name="thedate" select="@start"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="taskend">
        <xsl:call-template name="mrproj-parse-date">
          <xsl:with-param name="thedate" select="@end"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="tstart" 
        select="floor ($s2px * (date:seconds ($taskstart) + $sprojstart))"/>
      
      <xsl:variable name="tend" 
        select="floor ($s2px * (date:seconds ($taskend) + $sprojstart))"/>

      <xsl:variable name="tcomplete"
        select="floor ((($tend - $tstart) * @percent-complete) div 100)"/>

      <xsl:variable name="twidth" select="($tend - $tstart)"/>

      <tr class="{$rowclass}">
        <td>
          <span class="task-indent" />
            <xsl:choose>
              <!-- Task has subtasks -->
              <xsl:when test="task">
                <span class="gantt-name-parent">
                  <a name="task-{@id}" 
                    style="margin-left: {$indent*$task-indent-pixels}px">
                    <xsl:value-of select="@name"/>
                  </a>
                </span>
              </xsl:when>
              <!-- Milestones don't list names -->
              <xsl:when test="@type = 'milestone'">
              </xsl:when>
              <!-- Task is leaf -->
              <xsl:otherwise>
                <span class="gantt-name">
                  <a name="gantt-{@id}" 
                    style="margin-left: {$indent*$task-indent-pixels}px">
                    <xsl:value-of select="@name"/>
                  </a>
                </span>
              </xsl:otherwise>
            </xsl:choose>
          </td>

          <td>
            <xsl:if test="not (task)">
              <div style="white-space: nowrap">

                <xsl:if test="$tstart > 0">
                  <div class="gantt-empty-begin" 
                    style="width: {$tstart}px;">
                  </div>
                </xsl:if>
              
                <xsl:if test="$tcomplete > 0">
                  <div class="gantt-complete-done" 
                    style="width: {$tcomplete}px;">
                      <span class="gantt-complete-caption">
                        <xsl:value-of select="@percent-complete"/>%
                      </span>
                  </div>
                </xsl:if>

                <xsl:if test="$tend > $tstart + $tcomplete">
                  <xsl:choose>
                    <xsl:when test="($ttoday > $tstart + $tcomplete) 
                      and ($tend > $ttoday)">
                      <div class="gantt-complete-undone" 
                        style="width: {($ttoday - $tcomplete - $tstart)}px;">
                      </div>
                      <div class="gantt-complete-notdone" 
                        style="width: {($tend - $ttoday)}px;"></div>
                    </xsl:when>

                    <xsl:when test="($ttoday > $tcomplete) and ($ttoday > $tend)">
                      <div class="gantt-complete-undone" 
                        style="width: {($twidth - $tcomplete)-2}px;">
                      </div>
                    </xsl:when>
              
                    <xsl:otherwise>
                      <div class="gantt-complete-notdone" 
                        style="width: {($twidth - $tcomplete)}px;">
                      </div>
                    </xsl:otherwise>
                  </xsl:choose>
                </xsl:if>

                <xsl:text>  </xsl:text>

                <!-- Milestones are printed inside gantt chart needed -->
                <div class="gantt-empty-end">
                  <xsl:choose>
                    <xsl:when test="@type = 'milestone'">
                      <span class="gantt-milestone-caption">
                        <xsl:value-of select="@name"/>
                      </span>
                    </xsl:when>
                    <xsl:otherwise>
                    </xsl:otherwise>
                  </xsl:choose>
                </div>
              
                <div/>
              </div>
            </xsl:if>
          </td>

        </tr>
      </xsl:for-each>
    </table>

</xsl:template>


</xsl:stylesheet>
