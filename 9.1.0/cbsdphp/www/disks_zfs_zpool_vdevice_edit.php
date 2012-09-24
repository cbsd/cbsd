#!/usr/local/bin/php
<?php
/*
	disks_zfs_zpool_vdevice_edit.php

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
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

$pgtitle = array(gettext("Disks"), gettext("ZFS"), gettext("Pools"), gettext("Virtual device"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['zfs']['vdevices']['vdevice']) || !is_array($config['zfs']['vdevices']['vdevice']))
	$config['zfs']['vdevices']['vdevice'] = array();

array_sort_key($config['zfs']['vdevices']['vdevice'], "name");

$a_vdevice = &$config['zfs']['vdevices']['vdevice'];
$a_disk = get_conf_disks_filtered_ex("fstype", "zfs");

function strip_dev($device) {
	if (preg_match("/^\/dev\/(.+)$/", $device, $m)) {
		$device = $m[1];
	}
	return $device;
}
function strip_partition($device) {
	if (preg_match("/^(.*)p\d+$/", $device, $m)) {
		$device = $m[1];
	}
	return $device;
}

if (!isset($uuid) && (!sizeof($a_disk)) && (!sizeof($a_encrypteddisk))) {
	$errormsg = sprintf(gettext("No disks available. Please add new <a href='%s'>disk</a> first."), "disks_manage.php");
}

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_vdevice, "uuid")))) {
	$pconfig['uuid'] = $a_vdevice[$cnid]['uuid'];
	$pconfig['name'] = $a_vdevice[$cnid]['name'];
	$pconfig['type'] = $a_vdevice[$cnid]['type'];
	$pconfig['device'] = $a_vdevice[$cnid]['device'];
	$pconfig['aft4k'] = isset($a_vdevice[$cnid]['aft4k']);
	$pconfig['desc'] = $a_vdevice[$cnid]['desc'];
} else {
	$pconfig['uuid'] = uuid();
	$pconfig['name'] = "";
	$pconfig['type'] = "stripe";
	$pconfig['aft4k'] = false;
	$pconfig['desc'] = "";
}

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: disks_zfs_zpool_vdevice.php");
		exit;
	}

	// Input validation
	$reqdfields = explode(" ", "name type");
	$reqdfieldsn = array(gettext("Name"), gettext("Type"));
	$reqdfieldst = explode(" ", "string string");

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	// Check for duplicate name
	if (!(isset($uuid) && $_POST['name'] === $a_vdevice[$cnid]['name'])) {
		if (false !== array_search_ex($_POST['name'], $a_vdevice, "name")) {
			$input_errors[] = gettext("This virtual device name already exists.");
		}
	}

	switch ($_POST['type']) {
		case "log-mirror":
		case "mirror": {
				if (count($_POST['device']) <  2) {
					$input_errors[] = gettext("There must be at least 2 disks in a mirror.");
				}
			}
			break;

		case "raidz":
		case "raidz1": {
				if (count($_POST['device']) <  2) {
					$input_errors[] = gettext("There must be at least 2 disks in a raidz.");
				}
			}
			break;

		case "raidz2":{
				if (count($_POST['device']) <  3) {
					$input_errors[] = gettext("There must be at least 3 disks in a raidz2.");
				}
			}
			break;

		case "raidz3":{
				if (count($_POST['device']) <  4) {
					$input_errors[] = gettext("There must be at least 4 disks in a raidz3.");
				}
			}
			break;

		default: {
				if (count($_POST['device']) <  1) {
					$input_errors[] = gettext("There must be at least 1 disks selected.");
				}
			}
			break;
	}

	if (!$input_errors) {
		$vdevice = array();
		$vdevice['uuid'] = $_POST['uuid'];
		$vdevice['name'] = $_POST['name'];
		$vdevice['type'] = $_POST['type'];
		$vdevice['device'] = $_POST['device'];
		$vdevice['aft4k'] = isset($_POST['aft4k']);
		$vdevice['desc'] = $_POST['desc'];

		if (isset($uuid) && (FALSE !== $cnid)) {
			$mode = UPDATENOTIFY_MODE_MODIFIED;
			$a_vdevice[$cnid] = $vdevice;
		} else {
			$mode = UPDATENOTIFY_MODE_NEW;
			$a_vdevice[] = $vdevice;
		}

		updatenotify_set("zfsvdev", $mode, $vdevice['uuid']);
		write_config();

		header("Location: disks_zfs_zpool_vdevice.php");
		exit;
	}
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	document.iform.name.disabled = !enable_change;
	document.iform.type.disabled = !enable_change;
	document.iform.device.disabled = !enable_change;
	document.iform.aft4k.disabled = !enable_change;
}
// -->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav">
				<li class="tabact"><a href="disks_zfs_zpool.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Pools");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_dataset.php"><span><?=gettext("Datasets");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_volume.php"><span><?=gettext("Volumes");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_snapshot.php"><span><?=gettext("Snapshots");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_config.php"><span><?=gettext("Configuration");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav2">
				<li class="tabact"><a href="disks_zfs_zpool_vdevice.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Virtual device");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_zpool.php"><span><?=gettext("Management");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_zpool_tools.php"><span><?=gettext("Tools");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_zpool_info.php"><span><?=gettext("Information");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_zpool_io.php"><span><?=gettext("I/O statistics");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="disks_zfs_zpool_vdevice_edit.php" method="post" name="iform" id="iform">
				<?php if ($errormsg) print_error_box($errormsg);?>
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if (file_exists($d_sysrebootreqd_path)) print_info_box(get_std_save_message(0));?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_inputbox("name", gettext("Name"), $pconfig['name'], "", true, 20, isset($uuid) && false !== $cnid);?>
					<?php html_combobox("type", gettext("Type"), $pconfig['type'], array("stripe" => gettext("Stripe"), "mirror" => gettext("Mirror"), "raidz1" => gettext("Single-parity RAID-Z"), "raidz2" => gettext("Double-parity RAID-Z"), "raidz3" => gettext("Triple-parity RAID-Z"), "spare" => gettext("Hot Spare"), "cache" => gettext("Cache"), "log" => gettext("Log"), "log-mirror" => gettext("Log (mirror)")), "", true, isset($uuid) && false !== $cnid);?>
					<?php $a_device = array(); foreach ($a_disk as $diskv) { if (isset($uuid) && false !== $cnid && !(is_array($pconfig['device']) && in_array($diskv['devicespecialfile'], $pconfig['device']))) { continue; } if ((!isset($uuid) || isset($uuid) && false === $cnid) && false !== array_search_ex($diskv['devicespecialfile'], $a_vdevice, "device")) { continue; } $a_device[$diskv['devicespecialfile']] = htmlspecialchars("{$diskv['name']} ({$diskv['size']}, {$diskv['desc']})"); }?>
					<?php
					    if (isset($uuid) && false !== $cnid) {
						foreach($a_vdevice[$cnid]['device'] as $dev) {
						    $tmp = disks_label_to_device($dev);
						    if (strcmp($tmp, $dev) != 0) {
							$a_device[strip_dev($dev)] = htmlspecialchars(sprintf("%s (%s)", strip_dev($dev), strip_dev($tmp)));
						    } else {
							$tmp = strip_partition($dev);
							if (strcmp($tmp, $dev) != 0) {
							    $a_device[strip_dev($dev)] = htmlspecialchars(sprintf("%s (%s)", strip_dev($dev), strip_dev($tmp)));
							}
						    }
						}
					    }
					?>
					<?php html_listbox("device", gettext("Devices"), $pconfig['device'], $a_device, "", true, isset($uuid) && false !== $cnid);?>
					<?php html_checkbox("aft4k", gettext("Advanced Format"), $pconfig['aft4k'] ? true : false, gettext("Enable Advanced Format (4KB sector)"), "", false, "");?>
					<?php html_inputbox("desc", gettext("Description"), $pconfig['desc'], gettext("You may enter a description here for your reference."), false, 40);?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=((isset($uuid) && (FALSE !== $cnid))) ? gettext("Save") : gettext("Add");?>" onclick="enable_change(true)" />
					<input name="Cancel" type="submit" class="formbtn" value="<?=gettext("Cancel");?>" />
					<input name="uuid" type="hidden" value="<?=$pconfig['uuid'];?>" />
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<script type="text/javascript">
<!--
<?php if (isset($uuid) && (false !== $cnid)):?>
<!-- Disable controls that should not be modified anymore in edit mode. -->
enable_change(false);
<?php endif;?>
//-->
</script>
<?php include("fend.inc");?>
