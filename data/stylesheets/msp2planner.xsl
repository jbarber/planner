<?xml version="1.0"?> 
<xsl:stylesheet version="1.0"
        xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
        xmlns:ms="http://schemas.microsoft.com/project"
        exclude-result-prefixes="ms">

<!-- ===============================================================
        msp2planner.xsl

        Conversion between Microsoft Project and Planner XML files

        Copyright (c) 2004 Kurt Maute (kurt@maute.us)
     ===============================================================-->

<xsl:output method="xml" indent="yes"/>

<!-- ===============================================================
        Main  -->

<xsl:template match="ms:Project">

<!--  Project Tag  -->
  <xsl:variable name="projstartdate">
    <xsl:call-template name="ms2pdate">
      <xsl:with-param name="thedate" select="ms:StartDate"/>
    </xsl:call-template>
  </xsl:variable>
  <project name="{ms:Title}" manager="{ms:Manager}" 
  project-start="{$projstartdate}" mrproject-version="2" calendar="1">


<!--  Properties  -->
    <properties>
      <property name="cost" type="cost" owner="resource" label="Cost" 
      description="standard cost for a resource"/>
    </properties>

<!--  Phases  - not implemented  -->

<!--  Calendars  - only writing standard Planner calendar right now,
        since the MS Project implementation is painfully dissimilar  -->
  <calendars>
    <day-types>
      <day-type id="0" name="Working" description="A default working day"/>
      <day-type id="1" name="Nonworking" description="A default non working day"/>
      <day-type id="2" name="Use base" description="Use day from base calendar"/>
    </day-types>
    <calendar id="1" name="Default">
      <default-week mon="0" tue="0" wed="0" thu="0" fri="0" sat="1" sun="1"/>
      <overridden-day-types>
        <overridden-day-type id="0">
          <interval start="0800" end="1200"/>
          <interval start="1300" end="1700"/>
        </overridden-day-type>
      </overridden-day-types>
      <days/>
    </calendar>
  </calendars>

<!--  Tasks    -->
    <xsl:apply-templates select="ms:Tasks"/>

<!--  Resources    -->
    <xsl:apply-templates select="ms:Resources"/>

<!--  Allocations    -->
    <xsl:apply-templates select="ms:Assignments"/>

  </project>
</xsl:template>

<!-- ===============================================================
        Task Handling Templates  -->

<!-- Main Task loop 
     This template loops thru the 1st outline level of task elements
     and calls the recursive template 'task' to handle the subsequent
     levels.  Couldn't just use the recursive routine for all since
     starts-with() can't match on an empty variable - so we need this
     to get going.  -->
<xsl:template match="ms:Tasks">
  <tasks>
  <xsl:for-each select="ms:Task[ms:OutlineLevel=1]">
    <task>
    <xsl:call-template name="write-task"/>
    <xsl:if test="ms:OutlineLevel &lt; following::ms:Task/ms:OutlineLevel">
      <xsl:call-template name="task">
        <xsl:with-param name="wbs" select="ms:OutlineNumber"/>
        <xsl:with-param name="lvl" select="ms:OutlineLevel+1"/>
      </xsl:call-template>
    </xsl:if>
    </task>
  </xsl:for-each>
  </tasks>
</xsl:template>

<!-- Recursive Task Loop
     This template loops recursively thru all but the top outline level of 
     tasks.  -->
<xsl:template name="task">
  <xsl:param name="wbs"/>
  <xsl:param name="lvl"/>
  <xsl:for-each select="//ms:Tasks/ms:Task[starts-with(ms:OutlineNumber,$wbs)][ms:OutlineLevel=$lvl]">
    <task>
    <xsl:call-template name="write-task"/>
    <!-- recursion implemented here:  -->
    <xsl:if test="ms:OutlineLevel &lt; following::ms:Task/ms:OutlineLevel">
      <xsl:call-template name="task">
        <xsl:with-param name="wbs" select="ms:OutlineNumber"/>
        <xsl:with-param name="lvl" select="ms:OutlineLevel+1"/>
      </xsl:call-template>
    </xsl:if>
    </task>
  </xsl:for-each>
</xsl:template>

<!-- Write Task
     This template writes out all the task info, except for the task start
     and end tags.  -->
<xsl:template name="write-task">
  <xsl:attribute name="id"><xsl:value-of select="ms:ID"/></xsl:attribute>
  <xsl:attribute name="name"><xsl:value-of select="ms:Name"/></xsl:attribute>
  <xsl:attribute name="note"><xsl:value-of select="ms:Notes"/></xsl:attribute>
  <xsl:attribute name="work">
    <xsl:call-template name="ms2pduration">
      <xsl:with-param name="dur" select="ms:Work"/>
    </xsl:call-template>
  </xsl:attribute>
  <xsl:attribute name="duration">
    <xsl:call-template name="ms2pduration">
      <xsl:with-param name="dur" select="ms:Duration"/>
    </xsl:call-template>
  </xsl:attribute>
  <xsl:attribute name="start">
    <xsl:call-template name="ms2pdate">
      <xsl:with-param name="thedate" select="ms:Start"/>
    </xsl:call-template>
  </xsl:attribute>
  <xsl:attribute name="end">
    <xsl:call-template name="ms2pdate">
      <xsl:with-param name="thedate" select="ms:Finish"/>
    </xsl:call-template>
  </xsl:attribute>
  <xsl:attribute name="percent-complete">
    <xsl:value-of select="ms:PercentComplete"/>
  </xsl:attribute>
  <xsl:attribute name="priority">
    <xsl:value-of select="ms:Priority"/>
  </xsl:attribute>
  <xsl:attribute name="type">
    <xsl:choose>
      <xsl:when test="ms:Milestone=0">normal</xsl:when>
      <xsl:otherwise>milestone</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
  <xsl:attribute name="scheduling">
    <xsl:choose>
      <xsl:when test="ms:Type=0">fixed-duration</xsl:when> <!-- should be fixed-units but we don't have that -->
      <xsl:when test="ms:Type=1">fixed-duration</xsl:when>
      <xsl:otherwise>fixed-work</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>

  <!-- Constraints handled here -->
  <xsl:if test="ms:ConstraintType>0">
    <constraint>
    <xsl:attribute name="type">
      <xsl:choose>
        <xsl:when test="ms:ConstraintType=2">must-start-on</xsl:when>
        <xsl:otherwise>start-no-earlier-than</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:attribute name="time">
      <xsl:call-template name="ms2pdate">
        <xsl:with-param name="thedate" select="ms:ConstraintDate"/>
      </xsl:call-template>
    </xsl:attribute>
    </constraint>  
  </xsl:if>

  <!-- Predecessors handled here -->
  <predecessors>
  <xsl:for-each select="ms:PredecessorLink">
    <predecessor id="{position()}">
      <xsl:attribute name="predecessor-id">
        <xsl:call-template name="get-task-id">
          <xsl:with-param name="uid" select="ms:PredecessorUID"/>
        </xsl:call-template>
      </xsl:attribute>
      <xsl:attribute name="type">
        <xsl:choose>
          <xsl:when test="ms:Type=0">FF</xsl:when>
          <xsl:when test="ms:Type=1">FS</xsl:when>
          <xsl:when test="ms:Type=2">SF</xsl:when>
          <xsl:otherwise>SS</xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <xsl:attribute name="lag">
        <xsl:value-of select="ms:LinkLag*6"/>
      </xsl:attribute>
    </predecessor>
  </xsl:for-each>
  </predecessors>
</xsl:template>

<!-- Get task id from UID -->
<xsl:template name="get-task-id">
  <xsl:param name="uid"/>
  <xsl:value-of select="//ms:Tasks/ms:Task[ms:UID=$uid]//ms:ID"/>
</xsl:template>

<!-- ===============================================================
        Resource Handling Template  -->

<!-- Resource loop   -->
<xsl:template match="ms:Resources">
  <resources>
    <xsl:for-each select="ms:Resource[ms:ID>0]">
      <resource id="{ms:ID}" name="{ms:Name}" short-name="{ms:Initials}">
        <xsl:attribute name="type">
          <xsl:choose>
            <xsl:when test='ms:Type=0'>2</xsl:when>
            <xsl:otherwise>1</xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
        <xsl:attribute name="units">
          <xsl:value-of select='ms:MaxUnits'/>
        </xsl:attribute>
        <xsl:attribute name="email">
          <xsl:value-of select='ms:EmailAddress'/>
        </xsl:attribute>
        <xsl:attribute name="note">
          <xsl:value-of select='ms:Notes'/>
        </xsl:attribute>
        <xsl:attribute name="std-rate">
          <xsl:value-of select='ms:StandardRate'/>
        </xsl:attribute>
        <xsl:attribute name="ovt-rate">
          <xsl:value-of select='ms:OvertimeRate'/>
        </xsl:attribute>
        <properties>
          <property name="cost" value="{ms:Cost}"/>
        </properties>
      </resource>
    </xsl:for-each>
  </resources>
</xsl:template>

<!-- ===============================================================
        Allocations Handling Template  -->

<!-- Allocations loop   -->
<xsl:template match="ms:Assignments">
  <allocations>
    <xsl:for-each select="ms:Assignment[ms:ResourceUID>0]">
      <allocation>
      <xsl:attribute name="task-id">
        <xsl:call-template name="get-task-id">
          <xsl:with-param name="uid" select="ms:TaskUID"/>
        </xsl:call-template>
      </xsl:attribute>
      <xsl:attribute name="resource-id">
        <xsl:call-template name="get-resource-id">
          <xsl:with-param name="uid" select="ms:ResourceUID"/>
        </xsl:call-template>
      </xsl:attribute>
      <xsl:attribute name="units">
        <xsl:value-of select="ms:Units*100"/>
      </xsl:attribute>
      </allocation>
    </xsl:for-each>
  </allocations>
</xsl:template>

<!-- Get resource id from UID -->
<xsl:template name="get-resource-id">
  <xsl:param name="uid"/>
  <xsl:value-of select="//ms:Resources/ms:Resource[ms:UID=$uid]//ms:ID"/>
</xsl:template>

<!-- ===============================================================
        Global Functions  -->

<!-- Convert date from MSP to Planner format -->
<xsl:template name="ms2pdate">
  <xsl:param name="thedate"/>
  <xsl:variable name="formatted" select="concat(translate(substring($thedate, 
   1, 11),'-',''),translate(substring-after($thedate,'T'),':',''),'Z')"/>
  <xsl:value-of select="$formatted"/>
</xsl:template>

<!-- Convert duration from MSP to Planner format -->
<xsl:template name="ms2pduration">
  <xsl:param name="dur"/>
  <xsl:variable name="rslt" 
    select="substring-after(substring-before($dur,'H'),'T')*3600
      +substring-after(substring-before($dur,'M'),'H')*60
      +substring-after(substring-before($dur,'S'),'M')"/>
  <xsl:value-of select="$rslt"/>
</xsl:template>


<!-- ===============================================================
        End  -->

</xsl:stylesheet>
