#!/usr/local/bin/php
<?php
/*
	diag_traceroute.php

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	All rights reserved.
	
	Portions of m0n0wall (http://m0n0.ch/wall).
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

$pgtitle = array(gettext("Diagnostics"), gettext("Traceroute"));

if ($_POST) {
	unset($input_errors);
	unset($do_traceroute);

	// Input validation.
	$reqdfields = explode(" ", "host ttl");
	$reqdfieldsn = array(gettext("Host"), gettext("Max. TTL"));

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

	if (!$input_errors) {
		$do_traceroute = true;
		$host = $_POST['host'];
		$ttl = $_POST['ttl'];
		$resolve = $_POST['resolve'];
	}
}

if (!isset($do_traceroute)) {
	$do_traceroute = false;
	$host = '';
	$ttl = 18;
	$resolve = false;
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav">
				<li class="tabinact"><a href="diag_ping.php"><span><?=gettext("Ping");?></span></a></li>
				<li class="tabact"><a href="diag_traceroute.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Traceroute");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="diag_traceroute.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_inputbox("host", gettext("Host"), $host, gettext("Destination host name or IP number."), true, 20);?>
					<?php $a_ttl = array(); for ($i = 1; $i <= 64; $i++) { $a_ttl[$i] = $i; }?>
					<?php html_combobox("ttl", gettext("Max. TTL"), $ttl, $a_ttl, gettext("Max. time-to-live (max. number of hops) used in outgoing probe packets."), true);?>
					<?php html_checkbox("resolve", gettext("Resolve"), $resolve ? true : false, gettext("Resolve IP addresses to hostnames"), "", false);?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Traceroute");?>" />
				</div>
				<?php if ($do_traceroute) {
				echo(sprintf("<div id='cmdoutput'>%s</div>", gettext("Command output:")));
				echo('<pre class="cmdoutput">');
				ob_end_flush();
				exec("/usr/sbin/traceroute " . ($resolve ? "" : "-n ") . "-w 2 -m " . escapeshellarg($ttl) . " " . escapeshellarg($host), $rawdata);
				echo htmlspecialchars(implode("\n", $rawdata));
				unset($rawdata);
				echo('</pre>');
				}
				?>
				<div id="remarks">
					<?php html_remark("note", gettext("Note"), gettext("Traceroute may take a while to complete. You may hit the Stop button on your browser at any time to see the progress of failed traceroutes."));?>
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
