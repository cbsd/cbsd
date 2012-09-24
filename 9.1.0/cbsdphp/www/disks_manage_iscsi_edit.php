#!/usr/local/bin/php
<?php
/*
	disks_manage_iscsi_edit.php
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

$pgtitle = array(gettext("Disks"), gettext("Management"), gettext("iSCSI Initiator"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['iscsiinit']['vdisk']) || !is_array($config['iscsiinit']['vdisk']))
	$config['iscsiinit']['vdisk'] = array();

array_sort_key($config['iscsiinit']['vdisk'], "name");
$a_iscsiinit = &$config['iscsiinit']['vdisk'];

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_iscsiinit, "uuid")))) {
	$pconfig['uuid'] = $a_iscsiinit[$cnid]['uuid'];
	$pconfig['name'] = $a_iscsiinit[$cnid]['name'];
	$pconfig['targetname'] = $a_iscsiinit[$cnid]['targetname'];
	$pconfig['targetaddress'] = $a_iscsiinit[$cnid]['targetaddress'];
	$pconfig['initiatorname'] = $a_iscsiinit[$cnid]['initiatorname'];
} else {
	$pconfig['uuid'] = uuid();
	$pconfig['name'] = "";
	$pconfig['targetname'] = "";
	$pconfig['targetaddress'] = "";
	$pconfig['initiatorname'] = "";
}
if (isset($config['iscsitarget']['nodebase'])
    && !empty($config['iscsitarget']['nodebase'])) {
	$ex_nodebase = $config['iscsitarget']['nodebase'];
	$ex_disk = "disk0";
} else {
	$ex_nodebase = "iqn.2007-09.jp.ne.peach.istgt";
	$ex_disk = "disk0";
}
$ex_iscsitarget = $ex_nodebase.":".$ex_disk;

if ($_POST) {
	unset($input_errors);
	unset($errormsg);
	unset($do_crypt);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: disks_manage_iscsi.php");
		exit;
	}

	// Check for duplicates.
	foreach ($a_iscsiinit as $iscsiinit) {
		if (isset($uuid) && (FALSE !== $cnid) && ($iscsiinit['uuid'] === $uuid)) 
			continue;
		if (($iscsiinit['targetname'] === $_POST['targetname']) && ($iscsiinit['targetaddress'] === $_POST['targetaddress'])) {
			$input_errors[] = gettext("This couple targetname/targetaddress already exists in the disk list.");
			break;
		}
		if ($iscsiinit['name'] == $_POST['name']) {
			$input_errors[] = gettext("This name already exists in the disk list.");
			break;
		}
	}

	// Input validation
	$reqdfields = explode(" ", "name targetname targetaddress initiatorname");
	$reqdfieldsn = array(gettext("Name"), gettext("Target name"), gettext("Target address"), gettext("Initiator name"));
	$reqdfieldst = explode(" ", "alias string string string");

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if (!$input_errors) {
		$iscsiinit = array();
		$iscsiinit['uuid'] = $_POST['uuid'];
		$iscsiinit['name'] = $_POST['name'];
		$iscsiinit['targetname'] = $_POST['targetname'];
		$iscsiinit['targetaddress'] = $_POST['targetaddress'];
		$iscsiinit['initiatorname'] = $_POST['initiatorname'];

		if (isset($uuid) && (FALSE !== $cnid)) {
			$a_iscsiinit[$cnid] = $iscsiinit;
			$mode = UPDATENOTIFY_MODE_MODIFIED;
		} else {
			$a_iscsiinit[] = $iscsiinit;
			$mode = UPDATENOTIFY_MODE_NEW;
		}

		updatenotify_set("iscsiinitiator", $mode, $iscsiinit['uuid']);
		write_config();

		header("Location: disks_manage_iscsi.php");
		exit;
	}
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
<tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
      	<li class="tabinact"><a href="disks_manage.php"><span><?=gettext("Management");?></span></a></li>
      	<li class="tabinact"><a href="disks_manage_smart.php"><span><?=gettext("S.M.A.R.T.");?></span></a></li>
				<li class="tabact"><a href="disks_manage_iscsi.php" title="<?=gettext("Reload page");?>"><span><?=gettext("iSCSI Initiator");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
			<form action="disks_manage_iscsi_edit.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
			  <table width="100%" border="0" cellpadding="6" cellspacing="0">
			  	<?php html_inputbox("name", gettext("Name"), $pconfig['name'], gettext("This is for information only (not using during iSCSI negociation)."), true, 20);?>
					<?php html_inputbox("initiatorname", gettext("Initiator name"), $pconfig['initiatorname'], gettext("This name is for example: iqn.2005-01.il.ac.huji.cs:somebody."), true, 60);?>			
					<?php html_inputbox("targetname", gettext("Target name"), $pconfig['targetname'], sprintf(gettext("This name is for example: %s."), $ex_iscsitarget), true, 60);?>
					<?php html_inputbox("targetaddress", gettext("Target address"), $pconfig['targetaddress'], gettext("This the IP address or DNS name of the iSCSI target."), true, 20);?>
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
