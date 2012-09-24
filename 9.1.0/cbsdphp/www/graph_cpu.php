#!/usr/local/bin/php -f
<?php
/*
	graph_cpu.php

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	All rights reserved.
	
	portions of m0n0wall (http://m0n0.ch/wall)
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met: 

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer. 
	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution. 

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	The views and conclusions contained in the software and documentation are those
	of the authors and should not be interpreted as representing official policies, 
	either expressed or implied, of the NAS4Free Project.
*/
require("auth.inc");
require("guiconfig.inc");

/********* Other conf *******/
$nb_plot=120;			//NB plot in graph
$time_interval=1; //Refresh time Interval
$fetch_link = "stats.php?cpu";

//Style
//SVG attributes
$attribs['bg']='fill="#000000" stroke="none" stroke-width="0" opacity="1"';
$attribs['axis']='fill="white" stroke="black"';
$attribs['cpu']='fill="#FF0000" font-family="Tahoma, Verdana, Arial, Helvetica, sans-serif" font-size="7"';
$attribs['graph_cpu']='fill="none" stroke="#CA6110" stroke-opacity="0.8"';
$attribs['legend']='fill="white" font-family="Tahoma, Verdana, Arial, Helvetica, sans-serif" font-size="4"';
$attribs['graphname']='fill="#435370" font-family="Tahoma, Verdana, Arial, Helvetica, sans-serif" font-size="8"';
$attribs['grid_txt']='fill="gray" font-family="Tahoma, Verdana, Arial, Helvetica, sans-serif" font-size="6"';
$attribs['grid']='stroke="#C3C3C3" stroke-opacity="0.5"';
$attribs['error']='fill="red" font-family="Arial" font-size="4"';
$attribs['collect_initial']='fill="gray" font-family="Tahoma, Verdana, Arial, Helvetica, sans-serif" font-size="4"';

$error_text = "Cannot get CPU load";

$height=100;		//SVG internal height : do not modify
$width=200;		//SVG internal width : do not modify

$encoding = system_get_language_codeset();

/********* Graph DATA **************/
header("Last-Modified: " . gmdate( "D, j M Y H:i:s" ) . " GMT");
header("Expires: " . gmdate( "D, j M Y H:i:s", time() ) . " GMT");
header("Cache-Control: no-store, no-cache, must-revalidate"); // HTTP/1.1
header("Cache-Control: post-check=0, pre-check=0", FALSE);
header("Pragma: no-cache"); // HTTP/1.0
header("Content-type: image/svg+xml");
echo "<?xml version=\"1.0\" encoding=\"{$encoding}\"?>\n";
?>
<svg width="100%" height="100%" viewBox="0 0 <?=$width?> <?=$height?>" preserveAspectRatio="none" xml:space="preserve" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" onload="init(evt)">
  <g id="graph">
    <rect id="bg" x1="0" y1="0" width="100%" height="100%" <?=$attribs['bg']?>/>
    <line id="axis_x" x1="0" y1="0" x2="0" y2="100%" <?=$attribs['axis']?>/>
    <line id="axis_y" x1="0" y1="100%" x2="100%" y2="100%" <?=$attribs['axis']?>/>
    <path id="graph_cpu"  d="M0 <?=$height?> L 0 <?=$height?>" <?=$attribs['graph_cpu']?>/>
    <text id="graph_in_lbl" x="5" y="8" <?=$attribs['cpu']?>><?=gettext("CPU load");?> <tspan id="graph_cpu_txt" <?=$attribs['cpu']?>> </tspan></text>
    <path id="grid"  d="M0 <?=$height/4*1?> L <?=$width?> <?=$height/4*1?> M0 <?=$height/4*2?> L <?=$width?> <?=$height/4*2?> M0 <?=$height/4*3?> L <?=$width?> <?=$height/4*3?>" <?=$attribs['grid']?>/>
    <text id="grid_txt1" x="<?=$width*0.99?>" y="<?=$height/4*1?>" <?=$attribs['grid_txt']?> text-anchor="end">75%</text>
    <text id="grid_txt2" x="<?=$width*0.99?>" y="<?=$height/4*2?>" <?=$attribs['grid_txt']?> text-anchor="end">50%</text>
    <text id="grid_txt3" x="<?=$width*0.99?>" y="<?=$height/4*3?>" <?=$attribs['grid_txt']?> text-anchor="end">25%</text>
    <text id="datetime" x="<?=$width*0.51?>" y="5" <?=$attribs['legend']?>> </text>

    <polygon id="axis_arrow_x" <?=$attribs['axis']?> points="<?=($width) . "," . ($height)?> <?=($width-2) . "," . ($height-2)?> <?=($width-2) . "," . $height?>"/>
    <text id="error" x="<?=$width*0.5?>" y="<?=$height*0.4?>"  visibility="hidden" <?=$attribs['error']?> text-anchor="middle"><?=$error_text?></text>
    <text id="collect_initial" x="<?=$width*0.5?>" y="<?=$height*0.4?>" visibility="hidden" <?=$attribs['collect_initial']?> text-anchor="middle"><?=gettext("Collecting initial data, please wait...");?></text>
  </g>
  <script type="text/ecmascript">
    <![CDATA[

/**
 * getURL is a proprietary Adobe function, but it's simplicity has made it very
 * popular. If getURL is undefined we spin our own by wrapping XMLHttpRequest.
 */
if (typeof getURL == 'undefined') {
  getURL = function(url, callback) {
    if (!url)
      throw 'No URL for getURL';

    try {
      if (typeof callback.operationComplete == 'function')
        callback = callback.operationComplete;
    } catch (e) {}
    if (typeof callback != 'function')
      throw 'No callback function for getURL';

    var http_request = null;
    if (typeof XMLHttpRequest != 'undefined') {
      http_request = new XMLHttpRequest();
    }
    else if (typeof ActiveXObject != 'undefined') {
      try {
        http_request = new ActiveXObject('Msxml2.XMLHTTP');
      } catch (e) {
        try {
          http_request = new ActiveXObject('Microsoft.XMLHTTP');
        } catch (e) {}
      }
    }
    if (!http_request)
      throw 'Both getURL and XMLHttpRequest are undefined';

    http_request.onreadystatechange = function() {
      if (http_request.readyState == 4) {
        callback( { success : true,
                    content : http_request.responseText,
                    contentType : http_request.getResponseHeader("Content-Type") } );
      }
    }
    http_request.open('GET', url, true);
    http_request.send(null);
  }
}

var SVGDoc = null;
var plot_cpu = new Array();

var max_num_points = <?=$nb_plot?>;  // maximum number of plot data points
var step = <?=$width?> / max_num_points ;

function formatString(x) {
  return (x < 0 || x > 9 ? "" : "0") + x;
}

function init(evt) {
  SVGDoc = evt.target.ownerDocument;
  fetch_data();
}

function fetch_data() {
  getURL('<?=$fetch_link?>', plot_data);
}

function plot_data(obj) {
  // Show datetimelegend
  var now = new Date();
  var datetime = (now.getMonth()+1) + "/" + now.getDate() + "/" + now.getFullYear() + ' ' +
    formatString(now.getHours()) + ":" + formatString(now.getMinutes()) + ":" + formatString(now.getSeconds());
  SVGDoc.getElementById('datetime').firstChild.data = datetime;

	if (!obj.success)
    return handle_error();  // getURL failed to get data

  var t = obj.content;
	var cpu = parseInt(t);
	var scale;

	if (!isNumber(cpu))
    return handle_error();

  switch (plot_cpu.length) {
  	case 0:
  		SVGDoc.getElementById("collect_initial").setAttributeNS(null, 'visibility', 'visible');
      plot_cpu[0] = cpu;
      setTimeout('fetch_data()',<?=1000*$time_interval?>);
      return;
	case 1:
    	SVGDoc.getElementById("collect_initial").setAttributeNS(null, 'visibility', 'hidden');
    	break;
  case max_num_points:
		// shift plot to left if the maximum number of plot points has been reached
		var i = 0;
		while (i < max_num_points) {
		  plot_cpu[i] = plot_cpu[++i];
		}
		plot_cpu.length--;
  }

	plot_cpu[plot_cpu.length] = cpu;
	var index_plot = plot_cpu.length - 1;

	SVGDoc.getElementById('graph_cpu_txt').firstChild.data = plot_cpu[index_plot] + '%';

	scale = <?=$height?> / 100;

  var path_cpu = "M 0 " + (<?=$height?> - (plot_cpu[0] * scale));
  for (i = 1; i < plot_cpu.length; i++)
  {
    var x = step * i;
    var y_cpu = <?=$height?> - (plot_cpu[i] * scale);
    path_cpu += " L" + x + " " + y_cpu;
  }

  SVGDoc.getElementById('error').setAttributeNS(null, 'visibility', 'hidden');
  SVGDoc.getElementById('graph_cpu').setAttributeNS(null, 'd', path_cpu);

	setTimeout('fetch_data()',<?=1000*$time_interval?>);
}

function handle_error() {
  SVGDoc.getElementById("error").setAttributeNS(null, 'visibility', 'visible');
  setTimeout('fetch_data()',<?=1000*$time_interval?>);
}

function isNumber(a) {
    return typeof a == 'number' && isFinite(a);
}
    ]]>
  </script>
</svg>
