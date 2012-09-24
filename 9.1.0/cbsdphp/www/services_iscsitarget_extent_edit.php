#!/usr/local/bin/php
<?php
/*
	services_iscsitarget_extent_edit.php

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	All rights reserved.
	
	Portions of m0n0wall (http://m0n0.ch/wall)
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

/*
TODO: 	1) Script to creat file based extend in existing(mounted) File System e.g.(/mnt/$mountpoint/.../$filename) 
		with automaticaly formatting in necessary structure ( e.g. http://www.freebsd.org/doc/en_US.ISO8859-1/books/handbook/disks-virtual.html). 
		2) Insert changes to GUI for script.
		3) row 196.
*/
require("auth.inc");
require("guiconfig.inc");

$uuid = $_GET['uuid'];
if (isset($_POST['uuid']))
	$uuid = $_POST['uuid'];

$pgtitle = array(gettext("Services"), gettext("iSCSI Target"), gettext("Extent"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['iscsitarget']['extent']) || !is_array($config['iscsitarget']['extent']))
	$config['iscsitarget']['extent'] = array();

array_sort_key($config['iscsitarget']['extent'], "name");
$a_iscsitarget_extent = &$config['iscsitarget']['extent'];

function get_all_device($a_extent,$uuid) {
	$a = array();
	$a[''] = gettext("Must choose one");
	foreach (get_conf_all_disks_list_filtered() as $diskv) {
		$file = $diskv['devicespecialfile'];
		$size = $diskv['size'];
		$name = $diskv['name'];
		$desc = $diskv['desc'];
		if (strcmp($size, "NA") == 0) continue;
		if (disks_exists($file) == 1) continue;
		$index = array_search_ex($file, $a_extent, "path");
		if (FALSE !== $index) {
			if (!isset($uuid)) continue;
			if ($a_extent[$index]['uuid'] != $uuid) continue;
		}
		if (disks_ismounted_ex($file, "devicespecialfile")) continue;
		$a[$file] = htmlspecialchars("$name: $size ($desc)");
	}
	return $a;
}

// TODO: handle SCSI pass-through device
function get_all_scsi_device($a_extent,$uuid) {
	$a = array();
	$a[''] = gettext("Must choose one");
	foreach (get_conf_all_disks_list_filtered() as $diskv) {
		$file = $diskv['devicespecialfile'];
		$size = $diskv['size'];
		$name = $diskv['name'];
		$desc = $diskv['desc'];
		if (strcmp($size, "NA") == 0) continue;
		if (disks_exists($file) == 1) continue;
		$index = array_search_ex($file, $a_extent, "path");
		if (FALSE !== $index) {
			if (!isset($uuid)) continue;
			if ($a_extent[$index]['uuid'] != $uuid) continue;
		}
		if (!preg_match("/^(da|cd|sa|ch)[0-9]/", $name)) continue;
		$a[$file] = htmlspecialchars("$name: $size ($desc)");
	}
	return $a;
}

function get_all_zvol($a_extent,$uuid) {
	$a = array();
	$a[''] = gettext("Must choose one");
	mwexec2("zfs list -H -t volume -o name,volsize,sharenfs,org.freebsd:swap", $rawdata);
	foreach ($rawdata as $line) {
		$zvol = explode("\t", $line);
		$name = $zvol[0];
		$file = "/dev/zvol/$name";
		$size = $zvol[1];
		$sharenfs = $zvol[2];
		$swap = $zvol[3];
		if ($sharenfs !== "-") continue;
		if ($swap !== "-") continue;
		$index = array_search_ex($file, $a_extent, "path");
		if (FALSE !== $index) {
			if (!isset($uuid)) continue;
			if ($a_extent[$index]['uuid'] != $uuid) continue;
		}
		$a[$file] = htmlspecialchars("$name: $size");
	}
	return $a;
}

$a_device = get_all_device($a_iscsitarget_extent,$uuid);
$a_scsi_device = get_all_scsi_device($a_iscsitarget_extent,$uuid);
$a_zvol = get_all_zvol($a_iscsitarget_extent,$uuid);

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_iscsitarget_extent, "uuid")))) {
	$pconfig['uuid'] = $a_iscsitarget_extent[$cnid]['uuid'];
	$pconfig['name'] = $a_iscsitarget_extent[$cnid]['name'];
	$pconfig['path'] = $a_iscsitarget_extent[$cnid]['path'];
	$pconfig['size'] = $a_iscsitarget_extent[$cnid]['size'];
	$pconfig['sizeunit'] = $a_iscsitarget_extent[$cnid]['sizeunit'];
	$pconfig['type'] = $a_iscsitarget_extent[$cnid]['type'];
	$pconfig['comment'] = $a_iscsitarget_extent[$cnid]['comment'];

	if (!isset($pconfig['sizeunit']))
		$pconfig['sizeunit'] = "MB";
} else {
	// Find next unused ID.
	$extentid = 0;
	$a_id = array();
	foreach($a_iscsitarget_extent as $extent)
		$a_id[] = (int)str_replace("extent", "", $extent['name']); // Extract ID.
	while (true === in_array($extentid, $a_id))
		$extentid += 1;

	$pconfig['uuid'] = uuid();
	$pconfig['name'] = "extent{$extentid}";
	$pconfig['path'] = "";
	$pconfig['size'] = "";
	$pconfig['sizeunit'] = "MB";
	$pconfig['type'] = "file";
	$pconfig['comment'] = "";
}

if ($_POST) {
	unset($input_errors);
	unset($errormsg);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: services_iscsitarget_target.php");
		exit;
	}

	// Input validation.
	if ($_POST['type'] == 'device') {
		$pconfig['sizeunit'] = "auto";
		$_POST['sizeunit'] = "auto";
		$pconfig['size'] = "";
		$_POST['size'] = "";
		$reqdfields = explode(" ", "name device");
		$reqdfieldsn = array(gettext("Extent name"), gettext("Device"));
		$reqdfieldst = explode(" ", "string string");
	} else if ($_POST['type'] == 'zvol') {
		$pconfig['sizeunit'] = "auto";
		$_POST['sizeunit'] = "auto";
		$pconfig['size'] = "";
		$_POST['size'] = "";
		$reqdfields = explode(" ", "name zvol");
		$reqdfieldsn = array(gettext("Extent name"), gettext("ZFS volume"));
		$reqdfieldst = explode(" ", "string string");
	} else {
		if ($pconfig['sizeunit'] == 'auto'){
			$pconfig['size'] = "";
			$_POST['size'] = "";
			$reqdfields = explode(" ", "name path sizeunit");
			$reqdfieldsn = array(gettext("Extent name"), gettext("Path"), gettext("Auto size"));
			$reqdfieldst = explode(" ", "string string string");
		}else{
			$reqdfields = explode(" ", "name path size sizeunit");
			$reqdfieldsn = array(gettext("Extent name"), gettext("Path"), gettext("File size"), gettext("File sizeunit"));
			$reqdfieldst = explode(" ", "string string numericint string");
		}
	}

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	// Check for duplicates.
	$index = array_search_ex($_POST['name'], $a_iscsitarget_extent, "name");
	if (FALSE !== $index) {
		if (!((FALSE !== $cnid) && ($a_iscsitarget_extent[$cnid]['uuid'] === $a_iscsitarget_extent[$index]['uuid']))) {
			$input_errors[] = gettext("The extent name already exists.");
		}
	}

	// Check if path exists and match directory.
	if ($_POST['type'] == 'file') {
		$dirname = dirname($_POST['path']);
		$basename = basename($_POST['path']);
		if ($dirname !== "/") {
			$path = "$dirname/$basename";
		} else {
			$path = "/$basename";
		}
		if (!file_exists($dirname)) {
			$input_errors[] = sprintf(gettext("The path '%s' does not exist."), $dirname);
		}
		if (!is_dir($dirname)) {
			$input_errors[] = sprintf(gettext("The path '%s' is not a directory."), $dirname);
		}
		if (is_dir($path)) {
			$input_errors[] = sprintf(gettext("The path '%s' is a directory."), $path);
		}
	} else if ($_POST['type'] == 'zvol') {
		$path = $_POST['zvol'];
	} else {
		$path = $_POST['device'];
	}
	$pconfig['path'] = $path;

	if (!$input_errors) {
		$iscsitarget_extent = array();
		$iscsitarget_extent['uuid'] = $_POST['uuid'];
		$iscsitarget_extent['name'] = $_POST['name'];
		$iscsitarget_extent['path'] = $path;
		$iscsitarget_extent['size'] = $_POST['size'];
		$iscsitarget_extent['sizeunit'] = $_POST['sizeunit'];
		$iscsitarget_extent['type'] = $_POST['type'];
		$iscsitarget_extent['comment'] = $_POST['comment'];

		if (isset($uuid) && (FALSE !== $cnid)) {
			$a_iscsitarget_extent[$cnid] = $iscsitarget_extent;
			$mode = UPDATENOTIFY_MODE_MODIFIED;
		} else {
			$a_iscsitarget_extent[] = $iscsitarget_extent;
			$mode = UPDATENOTIFY_MODE_NEW;
		}

		updatenotify_set("iscsitarget_extent", $mode, $iscsitarget_extent['uuid']);
		write_config();

		header("Location: services_iscsitarget_target.php");
		exit;
	}
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function type_change() {
	switch (document.iform.type.value) {
	case "file":
		showElementById("path_tr", 'show');
		showElementById("size_tr", 'show');
		showElementById("device_tr", 'hide');
		showElementById("zvol_tr", 'hide');
		break;
	case "device":
		showElementById("path_tr", 'hide');
		showElementById("size_tr", 'hide');
		showElementById("device_tr", 'show');
		showElementById("zvol_tr", 'hide');
		break;
	case "zvol":
		showElementById("path_tr", 'hide');
		showElementById("size_tr", 'hide');
		showElementById("device_tr", 'hide');
		showElementById("zvol_tr", 'show');
		break;
	}
}

function sizeunit_change() {
	switch (document.iform.sizeunit.value) {
	case "auto":
		document.iform.size.disabled = true;
		break;
	default:
		document.iform.size.disabled = false;
		break;
	}
}
//-->
</script>
<form action="services_iscsitarget_extent_edit.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
	  <tr>
	    <td class="tabnavtbl">
	      <ul id="tabnav">
					<li class="tabinact"><a href="services_iscsitarget.php"><span><?=gettext("Settings");?></span></a></li>
					<li class="tabact"><a href="services_iscsitarget_target.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Targets");?></span></a></li>
					<li class="tabinact"><a href="services_iscsitarget_pg.php"><span><?=gettext("Portals");?></span></a></li>
					<li class="tabinact"><a href="services_iscsitarget_ig.php"><span><?=gettext("Initiators");?></span></a></li>
					<li class="tabinact"><a href="services_iscsitarget_ag.php"><span><?=gettext("Auths");?></span></a></li>
					<li class="tabinact"><a href="services_iscsitarget_media.php"><span><?=gettext("Media");?></span></a></li>
	      </ul>
	    </td>
	  </tr>
	  <tr>
	    <td class="tabcont">
	      <?php if ($input_errors) print_input_errors($input_errors);?>
	      <table width="100%" border="0" cellpadding="6" cellspacing="0">
	      <?php html_inputbox("name", gettext("Extent Name"), $pconfig['name'], gettext("String identifier of the extent."), true, 30, (isset($uuid) && (FALSE !== $cnid)));?>
	      <?php html_combobox("type", gettext("Type"), $pconfig['type'], array("file" => gettext("File"), "device" => gettext("Device"), "zvol" => gettext("ZFS volume")), gettext("Type used as extent."), true, false, "type_change()");?>
	      <?php html_filechooser("path", gettext("Path"), $pconfig['path'], sprintf(gettext("File path (e.g. /mnt/sharename/extent/%s) used as extent."), $pconfig['name']), $g['media_path'], true);?>
	      <?php html_combobox("device", gettext("Device"), $pconfig['path'], $a_device, "", true);?>
	      <?php html_combobox("zvol", gettext("ZFS volume"), $pconfig['path'], $a_zvol, "", true);?>
	      <tr id="size_tr">
	        <td width="22%" valign="top" class="vncellreq"><?=gettext("File size");?></td>
	        <td width="78%" class="vtable">
	          <input name="size" type="text" class="formfld" id="size" size="10" value="<?=htmlspecialchars($pconfig['size']);?>" />
	          <select name="sizeunit" onclick="sizeunit_change()"> 
	            <option value="MB" <?php if ($pconfig['sizeunit'] === "MB") echo "selected=\"selected\"";?>><?=htmlspecialchars(gettext("MiB"));?></option>
	            <option value="GB" <?php if ($pconfig['sizeunit'] === "GB") echo "selected=\"selected\"";?>><?=htmlspecialchars(gettext("GiB"));?></option>
	            <option value="TB" <?php if ($pconfig['sizeunit'] === "TB") echo "selected=\"selected\"";?>><?=htmlspecialchars(gettext("TiB"));?></option>
	            <option value="auto" <?php if ($pconfig['sizeunit'] === "auto") echo "selected=\"selected\"";?>><?=htmlspecialchars(gettext("Auto"));?></option>
	          </select><br />
	          <span class="vexpl"><?=gettext("Size offered to the initiator. (up to 8EiB=8388608TiB. actual size is depend on your disks.)");?></span>
	        </td>
	      </tr>
	      <?php html_inputbox("comment", gettext("Comment"), $pconfig['comment'], gettext("You may enter a description here for your reference."), false, 40);?>
	      </table>
	      <div id="submit">
		      <input name="Submit" type="submit" class="formbtn" value="<?=(isset($uuid) && (FALSE !== $cnid)) ? gettext("Save") : gettext("Add")?>" />
		      <input name="Cancel" type="submit" class="formbtn" value="<?=gettext("Cancel");?>" />
		      <input name="uuid" type="hidden" value="<?=$pconfig['uuid'];?>" />
	      </div>
	    </td>
	  </tr>
	</table>
	<?php include("formend.inc");?>
</form>
<script type="text/javascript">
<!--
	type_change();
	sizeunit_change();
//-->
</script>
<?php include("fend.inc");?>
