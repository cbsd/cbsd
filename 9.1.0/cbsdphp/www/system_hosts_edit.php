#!/usr/local/bin/php
<?php
/*
	system_hosts_edit.php

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

$pgtitle = array(gettext("Network"), gettext("Hosts"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['system']['hosts']) || !is_array($config['system']['hosts']))
	$config['system']['hosts'] = array();

array_sort_key($config['system']['hosts'], "name");
$a_hosts = &$config['system']['hosts'];

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_hosts, "uuid")))) {
	$pconfig['uuid'] = $a_hosts[$cnid]['uuid'];
	$pconfig['name'] = $a_hosts[$cnid]['name'];
	$pconfig['address'] = $a_hosts[$cnid]['address'];
	$pconfig['descr'] = $a_hosts[$cnid]['descr'];
} else {
	$pconfig['uuid'] = uuid();
	$pconfig['name'] = "";
	$pconfig['address'] = "";
	$pconfig['descr'] = "";
}

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: system_hosts.php");
		exit;
	}

	// Input validation.
	$reqdfields = explode(" ", "name address");
	$reqdfieldsn = array(gettext("Hostname"),gettext("IP address"));

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

	if (($_POST['name'] && !is_validdesc($_POST['name']))) {
		$input_errors[] = gettext("The host name contain invalid characters.");
	}
	if (($_POST['address'] && !is_ipaddr($_POST['address']))) {
		$input_errors[] = gettext("A valid IP address must be specified.");
	}

	// Check for duplicates.
	$index = array_search_ex($_POST['name'], $a_hosts, "name");
	if (FALSE !== $index) {
		if (!((FALSE !== $cnid) && ($a_hosts[$cnid]['uuid'] === $a_hosts[$index]['uuid']))) {
			$input_errors[] = gettext("An host with this name already exists.");
		}
	}

	if (!$input_errors) {
		$host = array();
		$host['uuid'] = $_POST['uuid'];
		$host['name'] = $_POST['name'];
		$host['address'] = $_POST['address'];
		$host['descr'] = $_POST['descr'];

		if (isset($uuid) && (FALSE !== $cnid)) {
			$a_hosts[$cnid] = $host;
			$mode = UPDATENOTIFY_MODE_MODIFIED;
		} else {
			$a_hosts[] = $host;
			$mode = UPDATENOTIFY_MODE_NEW;
		}

		updatenotify_set("hosts", $mode, $host['uuid']);
		write_config();

		header("Location: system_hosts.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabcont">
      <form action="system_hosts_edit.php" method="post" name="iform" id="iform">
      	<?php if ($input_errors) print_input_errors($input_errors); ?>
        <table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_inputbox("name", gettext("Hostname"), $pconfig['name'], gettext("The host name may only consist of the characters a-z, A-Z and 0-9, - , _ and ."), true, 40);?>
					<?php html_inputbox("address", gettext("IP address"), $pconfig['address'], gettext("The IP address that this hostname represents."), true, 20);?>
					<?php html_inputbox("descr", gettext("Description"), $pconfig['descr'], gettext("You may enter a description here for your reference."), false, 20);?>
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
