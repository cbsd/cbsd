#!/usr/local/bin/php
<?php
/*
	system_rc_edit.php

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

$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

$type = $_GET['type'];
if (isset($_POST['type']))
	$type = $_POST['type'];

$pgtitle = array(gettext("System"), gettext("Advanced"), gettext("Command scripts"), isset($id) ? gettext("Edit") : gettext("Add"));

if (!isset($config['rc']['preinit']['cmd']) || !is_array($config['rc']['preinit']['cmd']))
	$config['rc']['preinit']['cmd'] = array();

if (!isset($config['rc']['postinit']['cmd']) || !is_array($config['rc']['postinit']['cmd']))
	$config['rc']['postinit']['cmd'] = array();

if (!isset($config['rc']['shutdown']['cmd']) || !is_array($config['rc']['shutdown']['cmd']))
	$config['rc']['shutdown']['cmd'] = array();

if (isset($id) && isset($type)) {
	$pconfig['type'] = $type;
	switch($pconfig['type']) {
		case "PREINIT":
			$pconfig['command'] = $config['rc']['preinit']['cmd'][$id];
			break;
		case "POSTINIT":
			$pconfig['command'] = $config['rc']['postinit']['cmd'][$id];
			break;
		case "SHUTDOWN":
			$pconfig['command'] = $config['rc']['shutdown']['cmd'][$id];
			break;
	}
} else {
	$pconfig['type'] = NULL;
	$pconfig['command'] = "";
}

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: system_rc.php");
		exit;
	}

	// Input validation.
	$reqdfields = explode(" ", "command type");
	$reqdfieldsn = array(gettext("Command"), gettext("Type"));
	$reqdfieldst = explode(" ", "string string");

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if (!$input_errors) {
		switch($_POST['type']) {
			case "PREINIT":
				$a_cmd = &$config['rc']['preinit']['cmd'];
				break;
			case "POSTINIT":
				$a_cmd = &$config['rc']['postinit']['cmd'];
				break;
			case "SHUTDOWN":
				$a_cmd = &$config['rc']['shutdown']['cmd'];
				break;
		}

		if (isset($id) && $a_cmd[$id])
			$a_cmd[$id] = $_POST['command'];
		else
			$a_cmd[] = $_POST['command'];

		write_config();

		header("Location: system_rc.php");
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
        <li class="tabact"><a href="system_rc.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Command scripts");?></span></a></li>
        <li class="tabinact"><a href="system_cron.php"><span><?=gettext("Cron");?></span></a></li>
        <li class="tabinact"><a href="system_rcconf.php"><span><?=gettext("rc.conf");?></span></a></li>
        <li class="tabinact"><a href="system_sysctl.php"><span><?=gettext("sysctl.conf");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
			<form action="system_rc_edit.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors); ?>
			  <table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_inputbox("command", gettext("Command"), $pconfig['command'], gettext("The command to be executed."), true, 60);?>
					<?php html_combobox("type", gettext("Type"), $pconfig['type'], array("PREINIT" => "PreInit", "POSTINIT" => "PostInit", "SHUTDOWN" => "Shutdown"), gettext("Execute command pre or post system initialization (booting) or before system shutdown."), true, isset($pconfig['type']));?>
			  </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=(isset($id) && isset($type)) ? gettext("Save") : gettext("Add")?>" />
					<input name="Cancel" type="submit" class="formbtn" value="<?=gettext("Cancel");?>" />
					<?php if (isset($id) && isset($type)):?>
					<input name="id" type="hidden" value="<?=$id;?>" />
					<input name="type" type="hidden" value="<?=$type;?>" />
					<?php endif;?>
				</div>
				<?php include("formend.inc");?>
			</form>
    </td>
  </tr>
</table>
<?php include("fend.inc");?>
