<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml">
<xsl:comment>

/* 
 * CSS Stylesheet for Planner HTML output.
 * 
 * Copyright (C) 2003 CodeFactory AB
 * Copyright (C) 2003 Daniel Lundin (daniel@edgewall.com)
 * Copyright (C) 2004 Chris Ladd (caladd@particlestorm.net)
 * Copyright (C) 2004 Imendio HB
 *
 */

HTML,BODY,TABLE { 
  font-family: Vera Sans,Arial,Helvetica,sans-serif;
  font-size: 10pt;
  white-space: nowrap;
}

H1 {
  font-size: 14pt;
  font-weight: bold;
  color: #000000;
}

H2 {
  font-size: 12pt;
  font-weight: bold;
  color: #000000;
  margin-bottom: 2px;
}

A:link, A:visited, A:hover[href] { 
  color: #213559;       
  text-decoration: none;
  font-weight: bold;       
}

A:hover[href] { 
  color: #992525;
}

/*
 * Separator (decorative)
 */

DIV.separator {
  margin: 40px;
}

/*
 * Project header
 */

DIV.proj-header {
	padding: 4px;
	right: 5px;
	top: 5px;
	float: left;
	font-size: 12pt;
	margin-bottom: 20px;
}

DIV.proj-header DIV .header {
	font-size: 10pt;
	font-weight: bold;
	margin-right: 5px;
}	

DIV.proj-header DIV .value {
	font-size: 10pt;
	font-weight: normal;
}	

/*
 * Footer 
 */

.footer {
	margin-top: 50px;
	border-style: solid;
	border-width: 1px 0px 0px 0px;
	border-color: #999;
	clear: both;
}

.footer-date {
	font-size: 10pt;
	float: left;
	color: #666;
}

.footer-disclaimer {
	font-size: 10pt;
	text-align: right;
	color: #666;
}

/*
 * Gantt
 */

DIV.gantt-empty-begin,DIV.gantt-empty-end,DIV.gantt-complete-done,DIV.gantt-complete-notdone
{
  clear: none;
  float: left; 
  height: 13px;
}

DIV.gantt-complete-done {
  background-color: #495f6b;
  border-style: solid;
  border-width: 0px;
  border-color: #000000;
  vertical-align: middle;
  height: 7px;
  margin-top: 3px
}

DIV.gantt-complete-notdone {
  background-color: #8db6cd;
  border-style: solid;
  border-width: 1px;
  border-color: #000000;
}

DIV.gantt-empty-end {
  margin-left: 0px;
}

SPAN.gantt-milestone {
  font-size: 9pt;
  color: #000000;
  vertical-align: middle;
  position: relative;
  margin-left: 0px;
  padding-left: 0px;
}

SPAN.gantt-resources {
  margin-left: 4px;
}

TH.gantt-day-header {
  margin: 0px;
  padding: 0px;
  font-size: 8pt;
  white-space: nowrap;
}
/*
 * Table Style
 */

TABLE {
  display: table;
  border-collapse: collapse;	
  border-style: solid;
  border-width: 1px; 1px; 1px; 1px;
  border-color: #000000;
  white-space: nowrap;
  margin: 2px;
  padding: 2px;
}

TR.header {
  background-color: #aaaaaa;
  border-style: solid;
  border-width: 1px; 0px; 0px; 0px;
  color: #ffffff;
  border-color: #000000;
  font-size: 9pt;
  white-space: nowrap;
}

TR.even {
  background-color: #eeeeee;
  border-style: solid;
  border-width: 0px; 0px; 0px; 0px;
  margin: 0;
  padding: 0;
  white-space: nowrap;
}

TR.odd {
  background-color: #ffffff;
  border-style: solid;
  border-width: 0px; 0px; 0px; 0px;
  margin: 0;
  padding: 0;
  white-space: nowrap;
}

TH SPAN, TR SPAN {
  margin-left: 10px;
  margin-right: 20px;
  white-space: nowrap;
}

</xsl:comment>
</xsl:stylesheet>
