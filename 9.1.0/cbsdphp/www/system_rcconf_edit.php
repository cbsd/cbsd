#!/usr/local/bin/php
<?php
/*
	system_rcconf_edit.php

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

$uuid = $_GET['uuid'];
if (isset($_POST['uuid']))
	$uuid = $_POST['uuid'];

$pgtitle = array(gettext("System"), gettext("Advanced"), gettext("rc.conf"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['system']['rcconf']['param']) || !is_array($config['system']['rcconf']['param']))
	$config['system']['rcconf']['param'] = array();

array_sort_key($config['system']['rcconf']['param'], "name");
$a_rcvar = &$config['system']['rcconf']['param'];

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_rcvar, "uuid")))) {
	$pconfig['enable'] = isset($a_rcvar[$cnid]['enable']);
	$pconfig['uuid'] = $a_rcvar[$cnid]['uuid'];
	$pconfig['name'] = $a_rcvar[$cnid]['name'];
	$pconfig['value'] = $a_rcvar[$cnid]['value'];
	$pconfig['comment'] = $a_rcvar[$cnid]['comment'];
} else {
	$pconfig['enable'] = true;
	$pconfig['uuid'] = uuid();
	$pconfig['name'] = "";
	$pconfig['value'] = "";
	$pconfig['comment'] = "";
}

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: system_rcconf.php");
		exit;
	}

	// Input validation.
	$reqdfields = explode(" ", "name value");
	$reqdfieldsn = array(gettext("Option"), gettext("Value"));
	$reqdfieldst = explode(" ", "string string");

  do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
  do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if (!$input_errors) {
		$param = array();
		$param['enable'] = $_POST['enable'] ? true : false;
		$param['uuid'] = $_POST['uuid'];
		$param['name'] = $pconfig['name'];
		$param['value'] = $pconfig['value'];
		$param['comment'] = $pconfig['comment'];

		if (isset($uuid) && (FALSE !== $cnid)) {
			$a_rcvar[$cnid] = $param;
			$mode = UPDATENOTIFY_MODE_MODIFIED;
		} else {
			$a_rcvar[] = $param;
			$mode = UPDATENOTIFY_MODE_NEW;
		}

		updatenotify_set("rcconf", $mode, $param['uuid']);
		write_config();

		header("Location: system_rcconf.php");
		exit;
	}
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
      	<li class="tabinact"><a href="system_advanced.php"><span><?=gettext("Advanced");?></span></a></li>
      	<li class="tabinact"><a href="system_email.php"><span><?=gettext("Email");?></span></a></li>
      	<li class="tabinact"><a href="system_proxy.php"><span><?=gettext("Proxy");?></span></a></li>
      	<li class="tabinact"><a href="system_swap.php"><span><?=gettext("Swap");?></span></a></li>
        <li class="tabinact"><a href="system_rc.php"><span><?=gettext("Command scripts");?></span></a></li>
        <li class="tabinact"><a href="system_cron.php"><span><?=gettext("Cron");?></span></a></li>
        <li class="tabact"><a href="system_rcconf.php" title="<?=gettext("Reload page");?>"><span><?=gettext("rc.conf");?></span></a></li>
        <li class="tabinact"><a href="system_sysctl.php"><span><?=gettext("sysctl.conf");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
			<form action="system_rcconf_edit.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors); ?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", "", $pconfig['enable'] ? true : false, gettext("Enable"));?>
					<?php html_inputbox("name", gettext("Name"), $pconfig['name'], gettext("Name of the variable."), true, 40);?>
					<?php html_inputbox("value", gettext("Value"), $pconfig['value'], gettext("The value of the variable."), true);?>
					<?php html_inputbox("comment", gettext("Comment"), $pconfig['comment'], gettext("You may enter a description here for your reference."), false, 40);?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=(isset($uuid) && (FALSE !== $cnid)) ? gettext("Save") : gettext("Add")?>" />
					<input name="Cancel" type="submit" class="formbtn" value="<?=gettext("Cancel");?>" />
					<input name="uuid" type="hidden" value="<?=$pconfig['uuid'];?>" />
				</div>
				<?php include("formend.inc");?>
			</form>
    </td>
  </tr>
</table>
<?php include("fend.inc");?>
