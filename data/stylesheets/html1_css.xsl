<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml">
<xsl:comment>

/* 
 * CSS Stylesheet for Planner HTML output.
 * Copyright (c)2003 CodeFactory AB
 *
 * Daniel Lundin (daniel@edgewall.com)
 *
 */

HTML,BODY,TABLE { 
  font-family: Nimbus Sans L, Arial,Helvetica,sans-serif;
  font-size: 10pt;
}

H1 {
  font-size: 26pt;
  font-weight: bold;
  color: #ffb200;
}

H2 {
  font-size: 20pt;
  font-weight: bold;
  color: #000;
  margin-bottom: 2px;
}

H3 {
  font-size: 12pt;
  font-weight: bold;
  color: #666;
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
  margin: 50px;
}


/*
 * Project header
 */

DIV.proj-header {
	padding: 4px;
	right: 5px;
	top: 5px;
/*	background-color: #ddddee;
	border-style: solid;
	border-width: 1px;
*/
  float: left;
}

DIV.proj-header {
	font-size: 12pt;
        margin-bottom: 20px;
}

DIV.proj-header DIV .header {
	font-size: 9pt;
	font-weight: bold;
	margin-right: 5px;
}	

DIV.proj-header DIV .value {
	font-weight: normal;
}	


/*
 * Main menu
 */

DIV.mainmenu { 
  font-size: 10pt;
  background: #eff;
  border-style: solid;
  border-width: 1px 3px 3px 1px;
  border-color: #000;
  float: left;
  padding: 5px;
  position: fixed;
  top: 2px;
  right: 2px;
  z-index: 20;
}

DIV.mainmenu H2 { 
  font-size: 10pt;
  font-weight: bold;
  padding: 0px;
  margin: 0px 0px 2px 0px;
  border-style: solid;
  border-width: 0px 0px 1px 0px;
}

DIV.mainmenu .item { }


/*
 * Resources
 */

TABLE.resources {
  border-collapse: collapse;	
  border-spacing: 2px;
  border-style: solid;
  border-width: 1px;
  border-color: #213559;
  -moz-box-sizing: border-box;
}

TABLE TR.resources-hdr  {
  background-color: #213559;
  color: white;
  font-size: 9pt;
  font-weight: normal;
}

TR.resource { 
  font-size: 10pt; 
  border-style: dashed;
  border-width: 1px;
  border-color: #ccc;
}

TR.resource TD {
  padding: 1px 3px;
  vertical-align: top;
}

TR.resource-group { 
  font-size: 10pt; 
  border-color: #000;
  border-style: solid;
  border-width: 1px;
}

.res-group-admin {
  padding: 1px 3px;
  vertical-align: bottom;
  
}

.res-group-name {
  font-size: 12pt; 
  font-weight: bold;
  padding-top: 10px;
  border-style: solid;
  border-width: 0px 1px 0px 0px;
  border-color: #fff;
}

.res-group-admin {
  text-align: right;
}

TH.res-hdr { }

.res-name { 
  margin-left: 10px;
}

TD.res-short-name { }

TD.res-type { }

TD.res-email { }

TD.res-tasks { 
  padding: 0px;
}

TD.res-tasks UL {  
  list-style-type: none;
  padding: 0px;
  margin: 0px 0px 0px 1.5em;
}

LI.res-tasks-task,LI.res-tasks-task-ongoing,LI.res-tasks-task-done { 
  padding: 0px;
  margin: 0px;
  white-space:nowrap;
}

LI.res-tasks-task-ongoing { 
  list-style-type: circle;
  color: #213559;
}

LI.res-tasks-task-done { 
  list-style-type: disc;
  color: #579957;
}

TD.res-note { 
  font-size: 9pt; 
}



/*
 * Tasks
 */

TABLE.tasks {
  display: table;
  border-collapse: collapse;	
  border-spacing: 2px;
  border-style: solid;
  border-width: 1px;
  border-color: #213559;
  -moz-box-sizing: border-box;

}

TABLE TR.tasks-hdr  {
  background-color: #213559;
  color: white;
  font-size: 9pt;
  font-weight: normal;
}

TR.task, TR.task-odd, TR.task-parent, TR.task-milestone { 
  font-size: 10pt; 
  border-color: #ccd;
  border-style: dashed;
  border-width: 1px 0px 1px 0px;
}

TR.task-odd { 
  background-color: #eee;
}

TR.task TD {
  padding: 1px 3px;
  vertical-align: top;
  border-color: #ccd;
  border-style: solid;
  border-width: 0px 1px 0px 0px;

}

TH.task-hdr { }

TR.task-parent {
  margin-top: 15px;
  border-width: 0;
}

TD.task-name,TD.task-parent-name {
  white-space:nowrap;
}

TD.task-parent-name { 
  font-weight: bold;
}

TR.task-milestone {
  margin-top: 10px;
  background-color: #eef;
  border-width: 1px;
  font-weight: bold;
  color: #333;
}

TR.task-milestone TD .datetime { 
  font-weight: normal;
  color: #000;
  background-color: #ccc;
  border-color: #999;
  border-style: solid;
  border-width: 1px;
  padding: 0px 2px;
}


TD.task-progress { }

TD.task-dates, .task-duration { 
  white-space: nowrap;
  clear: both;
  float: none;                              
}

TD.task-date .datetime {
  display: block;
}

TD.task-date .date {  
   display: block; */
}

TD.task-date .time { 
  font-size: 8pt; 
}

TD.task-work { }

TD.task-note { 
  font-size: 9pt; 
}

TD.task-progress { 
  white-space:nowrap;
}

SPAN.task-progress-done,SPAN.task-progress-notdone { 
  height: 15px;
}

SPAN.task-progress-done { 
  display: block;
  position: relative;
  float: left;
  background-color: #579957;                 
  border-style: solid;
  border-width: 1px;
  border-color: black;
}

SPAN.task-progress-notdone { 
  display: block;
  position: relative;
  float: left;
  background-color: #dde;
  border-style: solid;
  border-width: 1px 1px 1px 0px;
  border-color: #000000 #000000 #000000 #223344;
  margin-right: 2px;
}

SPAN.task-progress-caption { 
  font-size: 7pt;
}

DIV.task-resource { 
  white-space:nowrap;
}


/*
 * Property 
 */

DIV.property {}

SPAN.property-name {
  color: #345;
  white-space:nowrap;
}

SPAN.property-value {
  background-color: #cce;
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
	font-size: 11pt;
	float: left;
	color: #666;
}

.footer-disclaimer {
	font-size: 9pt;
	text-align: right;
	color: #666;
}


/*
 * Gantt
 */

TABLE.gantt {
  display: table;
  border-collapse: collapse;	
  border-spacing: 2px;
  border-style: solid;
  border-width: 1px;
  border-color: #213559;
  -moz-box-sizing: border-box;
}

TABLE TR.gantt-hdr  {
  background-color: #213559;
  color: white;
  font-size: 9pt;
  font-weight: normal;
}

TR.gantt-even, TR.gantt-odd  {
  border-color: #ccd;
  border-style: dashed;
  border-width: 0px 0px 1px 0px;
  margin: 0;
  padding: 0;
}

TD.gantt-dates,SPAN.gantt-date-start,SPAN.gantt-date-end {
  font-size: 9pt;
}

TD.gantt-dates { 
  background-color: #213559;
  color: #fff;
  font-weight: bold;
}

TD.gantt-timeline { 
  background-color: #8ac;
}

DIV.gantt-date {
  position: relative;
  border-style: dashed solid;
  border-color: black;
  border-width: 1px 2px 0px 2px;
}
	
SPAN.gantt-date-start {
  padding-left: 3px;
  color: #256;
}

SPAN.gantt-date-end {
  padding-right: 3px;
  color: #256;
}

.gantt-today {
  position: absolute;
  height: 100%;
  z-index: 10;
  padding-right: 5px;
  color: #000;
  border-style: solid;
  border-width: 0px 0px 0px 2px;
  border-color: black black black #500;
  white-space:nowrap;
}

.gantt-name {
  margin: 0px;
  white-space:nowrap;
}

.gantt-name-parent {
  font-weight: bold;
}

DIV.gantt-empty-begin,DIV.gantt-empty-end,DIV.gantt-complete-done,DIV.gantt-complete-notdone,DIV.gantt-complete-undone
{
  clear: none;
  float: left; 
  height: 13px;
}

DIV.gantt-empty-begin { }

.gantt-empty-end { }

DIV.gantt-complete-done {
  background-color: #579957;
  border-style: solid;
  border-width: 1px;
  border-color: #000;
  float: left;
}

DIV.gantt-complete-notdone {
  background-color: #dde;
  border-style: solid;
  border-width: 1px;
  border-color: #000 #000 #000 #234;
}

DIV.gantt-complete-undone {
  background-color: #b20;
  border-style: solid;
  border-width: 1px;
  border-color: #000 #000 #000 #200;
}


SPAN.gantt-complete-caption {
  font-size: 8px;
  color: #efb;
  float: left;
  position: relative;
  top: 3px
  left: 3px;
}

SPAN.gantt-milestone-caption {
  display: list-item;
  list-style-type: disc;
  font-size: 9pt;
  font-weight: bold;
  color: #066;
  margin: 0px;
  padding: 0px;
  white-space: nowrap;
}


/*
 * Milestones
 */

TABLE.milestones {
  display: table;
  border-collapse: collapse;	
  border-spacing: 2px;
  border-style: solid;
  border-width: 1px;
  border-color: #213559;
  -moz-box-sizing: border-box;
}

TABLE TR.milestones-hdr  {
  background-color: #213559;
  color: white;
  font-size: 9pt;
  font-weight: normal;
}

TD.milestones-date { }

TD.milestones-name { 
  font-size: 12pt;
  font-weight: bold;
  white-space: nowrap;
  padding: 2px 5px;
}

TD.milestones-note { 
  font-size: 9pt;
}

</xsl:comment>
</xsl:stylesheet>
