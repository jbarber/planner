<?xml version="1.0"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml"
             xmlns:date="http://exslt.org/dates-and-times">

<!--**************************************************************************
    *
    * html1_tasks.xsl: Display tasks in a table with progress bars
    *
    * Copyright (c)2003 Daniel Lundin
    * Copyright (c)2003 CodeFactory AB
    * Copyright (c) 2004 Chris Ladd (caladd@particlestorm.net)
    *
    *-->

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
	  <xsl:value-of select="($std-rate * $unit * $work) + $cost"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="tasks">
  <xsl:variable name="hasproperties" select="boolean (count(//task/properties/property[@value!='']))"/>
  <xsl:variable name="hasnotes" select="boolean (count(//task[@note!='']))"/>
  <h2><a name="tasks">Tasks</a></h2>
  <table cellspacing="0" cellpadding="0" border="1">
    <tr class="header" align="left">
      <th><span>WBS</span></th>
      <th><span>Name</span></th>
      <th><span>Start</span></th>
      <th><span>Finish</span></th>
      <th><span>Work</span></th>
      <th><span>Priority</span></th>
      <th><span>Cost</span></th>
      <xsl:if test="$hasnotes">
        <th><span>Notes</span></th>
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
            <xsl:with-param name="thedate" select="@start"/>
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
      
		<xsl:choose>
          <xsl:when test="task">
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
                <a name="task{@id}" style="font-weight: bold; margin-left: {$indent*$task-indent-pixels}px">
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
                <span>
				  <xsl:call-template name="mrproj-duration">
                    <xsl:with-param name="duration-in-seconds" select="@work"/>
                  </xsl:call-template>
				</span>
              </td>
              <td>
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
              <xsl:if test="$hasnotes">
                <td>
                  <span>
				    <xsl:value-of select="@note"/>
				  </span>
                </td>
              </xsl:if>
            </tr>
		  </xsl:when>
		  <xsl:when test="@type='milestone'">
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
			  </td>
			  <td>
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
              <xsl:if test="$hasnotes">
                <td>
                  <span>
				    <xsl:value-of select="@note"/>
				  </span>
                </td>
              </xsl:if>
            </tr>
          </xsl:when>
          <xsl:otherwise>
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
                <span>
				  <xsl:call-template name="mrproj-duration">
                    <xsl:with-param name="duration-in-seconds" select="@work"/>
                  </xsl:call-template>
				</span>
              </td>
			  <td align="center">
			    <span>
				  <xsl:if test="not(@priority = 0)">
				    <xsl:value-of select="format-number(@priority, '0')"/>
				  </xsl:if>
				</span>
              </td>
              <td align="right">
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
              <xsl:if test="$hasnotes">
                <td>
                  <span>
				    <xsl:value-of select="@note"/>
				  </span>
                </td>
              </xsl:if>
            </tr>
          </xsl:otherwise>
        </xsl:choose>
    </xsl:for-each>
  </table>
</xsl:template>
</xsl:stylesheet>
