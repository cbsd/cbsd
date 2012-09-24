#!/usr/local/bin/php
<?php
/*
	diag_log.php
	
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
require("diag_log.inc");

$log = $_GET['log'];
if (isset($_POST['log']))
	$log = $_POST['log'];
if (empty($log))
	$log = 0;

$pgtitle = array(gettext("Diagnostics"), gettext("Log"));

if ($_POST['clear']) {
	log_clear($loginfo[$log]);
	header("Location: diag_log.php?log={$log}");
	exit;
}

if ($_POST['download']) {
	log_download($loginfo[$log]);
	exit;
}

if ($_POST['refresh']) {
	header("Location: diag_log.php?log={$log}");
	exit;
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function log_change() {
	// Reload page
	window.document.location.href = 'diag_log.php?log=' + document.iform.log.value;
}
//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav">
				<li class="tabact"><a href="diag_log.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Log");?></span></a></li>
				<li class="tabinact"><a href="diag_log_settings.php"><span><?=gettext("Settings");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
    <td class="tabcont">
    	<form action="diag_log.php" method="post" name="iform" id="iform">
				<select id="log" class="formfld" onchange="log_change()" name="log">
					<?php foreach($loginfo as $loginfok => $loginfov):?>
					<?php if (FALSE === $loginfov['visible']) continue;?>
					<option value="<?=$loginfok;?>" <?php if ($loginfok == $log) echo "selected=\"selected\"";?>><?=htmlspecialchars($loginfov['desc']);?></option>
					<?php endforeach;?>
				</select>
				<input name="clear" type="submit" class="formbtn" value="<?=gettext("Clear");?>" />
				<input name="download" type="submit" class="formbtn" value="<?=gettext("Download");?>" />
				<input name="refresh" type="submit" class="formbtn" value="<?=gettext("Refresh");?>" />
				<br /><br />
				<table width="100%" border="0" cellpadding="0" cellspacing="0">
				  <?php log_display($loginfo[$log]);?>
				</table>
				<?php include("formend.inc");?>
			</form>
		</td>
  </tr>
</table>
<?php include("fend.inc");?>
