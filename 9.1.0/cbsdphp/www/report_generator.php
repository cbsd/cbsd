#!/usr/local/bin/php
<?php
/*
	report_generator.php

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.	

	Portions of FreeNAS (http://www.freenas.org)
	Copyright (C) 2005-2011 Olivier Cochard <olivier@freenas.org>
	All rights reserved.

	Portions of code from:
	Exec+ v1.02-000 - Copyright 2001-2003, All rights reserved
	Created by technologEase (http://www.technologEase.com).
	Modified for m0n0wall by Manuel Kasper <mk@neon1.net>)
	Adapted to FreeNAS GUI by Volker Theile <votdev@gmx.de>)
*/
// Configure page permission
$pgperm['allowuser'] = TRUE;

require("auth.inc");
require("guiconfig.inc");

$pgtitle = array(gettext("Help"), gettext("Report Generator"));

$pglocalheader = <<< EOD
<style type="text/css">
<!--
pre {
	border: 2px solid #999999;
	background: #F0F0F0;
	padding: 1em;
	font-family: 'Courier New', Courier, monospace;
	white-space: pre;
	line-height: 10pt;
	font-size: 10pt;
	width: 523pt;
}
-->
</style>
EOD;
?>
<?php include("fbegin.inc");?>
<?php
// Function: is Blank. Returns true or false depending on blankness of argument.
	function isBlank( $arg ) { return ereg( "^\s*$", $arg ); }

// Put string, Ruby-style.
	function puts( $arg ) { echo "$arg\n"; }
?>
<script type="text/javascript">
<!--
// Function: Reset onClick (event handler)  . Resets form on reset button click event.
	function Reset_onClick( form )
	{
		form.txtSubject.value = '';
		form.txtError.value = '';
		form.txtDescription.value = '';
		form.txtDescription.focus();
		return true;
	}
//-->
</script>
<?php
	// phpBB variables
	$nl = "\n"; //Set new line
	$hr = ""; //Set horizontal line
	$bs = ""; //Set bold start
	$be = "";	//Set bold end
	$cs = "Error or code:";	//Set code end
	$ce = "";	//Set code end

	// Get system and hardware informations
	$cpuinfo = system_get_cpu_info();
	$meminfo = system_get_ram_info();
	$hwinfo = trim(exec("/sbin/sysctl -a | /usr/bin/awk -F:\  '/controller|interface/ &&! /AT|VGA|Key|inet|floppy/{!u[$2]++}END{for(i in u) a=a OFS i;print a}'"));
	mwexec2("sysctl -n dev.acpi.0.%desc", $mbinfo);

	$sys_summary = sprintf("%s %s (revision %s) %s; %s %s %sMiB RAM",
		get_product_name(),
		get_product_version(),
		get_product_revision(),
		get_platform_type(),
		$mbinfo[0],
		$cpuinfo['model'],
		round($meminfo['real'] / 1024 / 1024));
?>
<form action="<?=$_SERVER['SCRIPT_NAME'];?>" method="post" enctype="multipart/form-data" name="iform">
  <table>
		<tr>
			<td class="label" align="right"><?=gettext("Info");?></td>
			<td class="text" align="left"><?=$sys_summary;?></td>
		</tr>
		<tr>
			<td class="label" align="right"><?=gettext("Subject");?></td>
			<td class="text"><input id="txtSubject" name="txtSubject" type="text" size="130" value="<?php echo $_POST['txtSubject']; ?>" /></td>
		</tr>
		<tr>
			<td class="label" align="right"><?=gettext("Description");?></td>
			<td class="text"><textarea id="txtDescription" name="txtDescription" rows="7" cols="80" wrap="on"><?=htmlspecialchars($_POST['txtDescription']);?></textarea></td>
		</tr>
		<tr>
			<td align="right"><?=gettext("Error");?></td>
			<td class="text"><textarea id="txtError" name="txtError" rows="3" cols="80" wrap="on"><?=htmlspecialchars($_POST['txtError']);?></textarea></td>
		</tr>
		<tr>
			<td align="right"><?=gettext("Hardware");?></td>
			<td class="type" valign="top"><input name="chk_Hardware" type="checkbox" id="chk_Hardware" checked="checked" /><?=gettext("Include basic hardware information.");?></td>
		</tr>
		<tr>
			<td align="right"><?=gettext("phpBB");?></td>
			<td class="type" valign="top"><input name="chk_phpBB" type="checkbox" id="chk_phpBB" checked="checked" /><?=gettext("Format the report for phpBB forum.");?></td>
		</tr>
		<tr>
			<td valign="top">&nbsp;&nbsp;&nbsp;</td>
			<td valign="top" align="center" class="label">
				<input type="submit" class="button" value="<?=gettext("Generate");?>" />
				<input type="button" class="button" value="<?=gettext("Clear");?>" onclick="return Reset_onClick( this.form )" />
			</td>
		</tr>
  </table>
	<?php
	if (!isBlank($_POST['txtSubject']) && !isBlank($_POST['txtDescription'])) {
		puts("<pre>");
		if (isset($_POST['chk_phpBB'])) { //Format report for phpBB
			$hr	= "[hr]1[/hr]";		//Set horizontal line
			$bs	= "[b]"; 			//Set bold start
			$be	= "[/b]";			//Set bold end
			$cs	= "[code]";			//Set code end
			$ce	= "[/code]";		//Set code end
		}
		print str_replace("; ", "\n", $sys_summary).$nl.$nl;
		if (isset($_POST['chk_Hardware'])) {
		print wordwrap($hwinfo, 70, $nl, true);
		print $nl;			

		}
		print wordwrap($hr.$nl.$nl.$bs."Subject:".$be.$nl.$_POST['txtSubject'].$hr, 80, $nl, true);
		print wordwrap($nl.$nl.$bs."Description:".$be.$nl.$_POST['txtDescription'], 80, $nl, true);
		if (!isBlank($_POST['txtError'])) {
			print wordwrap($nl.$nl.$hr.$cs.$nl.$_POST['txtError'].$nl.$ce, 80, $nl, true);
		}
		puts("</pre>");
	}
	?>
	<?php include("formend.inc");?>
</form>
<script type="text/javascript">
<!--
document.forms[0].txtDescription.focus();
//-->
</script>
<?php include("fend.inc");?>
