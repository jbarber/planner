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
    * Copyright (c) 2004 Imendio AB
    * Copyright (c) 2003 CodeFactory AB
    * Copyright (c) 2003 Daniel Lundin
    * Copyright (c) 2004 Chris Ladd (caladd@particlestorm.net)
    *
    *-->

<xsl:template name="create-week-row">
  <xsl:param name="days"/>
  <xsl:param name="date"/>
  <xsl:choose>
    <xsl:when test="date:day-in-week($date) = 2 and $days >= 7">
      <th align="center" colspan="7">
		Week <xsl:value-of select="date:week-in-year($date) + 1"/>,
		<xsl:value-of select="date:year($date)"/>
      </th>
      <xsl:if test="not($days = 7)">
        <xsl:call-template name="create-week-row">
          <xsl:with-param name="days" select="$days - 7"/>
	      <xsl:with-param name="date" select="date:add($date, date:duration(604800))"/>
	    </xsl:call-template>
      </xsl:if>
    </xsl:when>
	<xsl:when test="not($days >= 7)">
      <th colspan="{$days}"></th>
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
	  <th colspan="{$colspan}"></th>
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
  <th class="gantt-day-header" align="center" width="19px">
  <xsl:value-of select="date:day-in-month($date)"/>
  </th>
  <xsl:if test="$days > 1">
    <xsl:call-template name="create-day-row">
      <xsl:with-param name="days" select="$days - 1"/>
	  <xsl:with-param name="date" select="date:add($date, date:duration(86400))"/>
	</xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template name="gantt">
  <h2><a name="gantt">Gantt Chart</a></h2>

  <!-- Display for non-CSS browsers -->
  <div style="display: none">
    <hr/>
    <strong>*** Note: Gantt chart requires CSS capability. ***</strong>
    <hr/>
  </div>

  <xsl:variable name="tmpdays" select="ceiling($projlength div 86400)"/>

  <xsl:variable name="days">
    <xsl:choose>
      <xsl:when test="not($tmpdays >= 30)">
        30
       </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$tmpdays"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <div style="overflow: auto">
  <table cellspacing="0" cellpadding="0" border="1" style="margin-bottom: 0">
    <tr class="header" align="left">
      <th rowspan="2"><span>WBS</span></th>
      <th rowspan="2"><span>Name</span></th>
      <th rowspan="2"><span>Work</span></th>
      <xsl:call-template name="create-week-row">
        <xsl:with-param name="days" select="$days"/>
		<xsl:with-param name="date" select="$projstart"/>
	  </xsl:call-template>
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
		<td colspan="{$days}">
		  <div style="width: {$days * 20 + 1}px; white-space: nowrap;">
		    <xsl:if test="not (task)">
		      <xsl:if test="$task-start > 0">
			    <xsl:choose>
			      <xsl:when test="@type = 'milestone'">
				    <div class="gantt-empty-begin" style="width: {$task-start - 4}px;">
				    </div>
				  </xsl:when>
				  <xsl:otherwise>
				    <div class="gantt-empty-begin" style="width: {$task-start}px;">
					</div>
				  </xsl:otherwise>
			    </xsl:choose>
			  </xsl:if>
			  
              <xsl:if test="$task-end > 0">
			    <div class="gantt-complete-notdone" style="width: {$task-end}px;">
			      <xsl:if test="$task-complete > 0">
			        <div class="gantt-complete-done" style="width: {$task-complete}px;">
				    </div>
			      </xsl:if>
			    </div>
			  </xsl:if>
			  
			  <xsl:choose>
                <xsl:when test="@type = 'milestone'">
                  <span class="gantt-milestone">&#9670;</span>
                                  <span class="gantt-resources">
				  <xsl:variable name="task-id" select="@id"/>
				  <xsl:for-each select="/project/allocations/allocation[@task-id=$task-id]">
			        <xsl:sort data-type="number" select="@resource-id" order="descending"/>
				    <xsl:variable name="resource-id" select="@resource-id"/>
				    <xsl:value-of select="/project/resources/resource[@id=$resource-id]/@short-name"/>
				    <xsl:if test="not(position() = last())">
				      <xsl:text>, </xsl:text>
				    </xsl:if>
			      </xsl:for-each>
                                  </span>
				</xsl:when>
				<xsl:otherwise>
				  <div class="gantt-empty-end"></div>
                                  <span class="gantt-resources">
				  <xsl:variable name="task-id" select="@id"/>
			      <xsl:for-each select="/project/allocations/allocation[@task-id=$task-id]">
			        <xsl:sort data-type="number" select="@resource-id" order="descending"/>
				    <xsl:variable name="resource-id" select="@resource-id"/>
				    <xsl:value-of select="/project/resources/resource[@id=$resource-id]/@short-name"/>
				    <xsl:if test="not(position() = last())">
				      <xsl:text>, </xsl:text>
				    </xsl:if>
			      </xsl:for-each>
                                  </span>
				</xsl:otherwise>
			  </xsl:choose>
            </xsl:if>
	      </div>
        </td>
      </tr>
    </xsl:for-each>
  </table><br/>
  </div>
</xsl:template>
</xsl:stylesheet>
