#!/usr/local/bin/php
<?php
/*
	disks_mount_edit.php
	
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

$pgtitle = array(gettext("Disks"), gettext("Mount Point"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['mounts']['mount']) || !is_array($config['mounts']['mount']))
	$config['mounts']['mount'] = array();

array_sort_key($config['mounts']['mount'], "devicespecialfile");
$a_mount = &$config['mounts']['mount'];

// Get list of all configured disks (physical and virtual).
$a_disk = get_conf_all_disks_list_filtered();

// Load the /etc/cfdevice file to find out on which disk the OS is installed.
$cfdevice = trim(file_get_contents("{$g['etc_path']}/cfdevice"));
$cfdevice = "/dev/{$cfdevice}";

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_mount, "uuid")))) {
	$pconfig['uuid'] = $a_mount[$cnid]['uuid'];
	$pconfig['type'] = $a_mount[$cnid]['type'];
	$pconfig['mdisk'] = $a_mount[$cnid]['mdisk'];
	$pconfig['partition'] = $a_mount[$cnid]['partition'];
	$pconfig['devicespecialfile'] = $a_mount[$cnid]['devicespecialfile'];
	$pconfig['fstype'] = $a_mount[$cnid]['fstype'];
	$pconfig['sharename'] = $a_mount[$cnid]['sharename'];
	$pconfig['desc'] = $a_mount[$cnid]['desc'];
	$pconfig['readonly'] = isset($a_mount[$cnid]['readonly']);
	$pconfig['fsck'] = isset($a_mount[$cnid]['fsck']);
	$pconfig['owner'] = $a_mount[$cnid]['accessrestrictions']['owner'];
	$pconfig['group'] = $a_mount[$cnid]['accessrestrictions']['group'][0];
	$pconfig['mode'] = $a_mount[$cnid]['accessrestrictions']['mode'];
	$pconfig['filename'] = $a_mount[$cnid]['filename'];
} else {
	$pconfig['uuid'] = uuid();
	$pconfig['type'] = "disk";
	$pconfig['partition'] = "p1";
	$pconfig['readonly'] = false;
	$pconfig['fsck'] = true;
	$pconfig['owner'] = "root";
	$pconfig['group'] = "wheel";
	$pconfig['mode'] = "0777";
}

// Split partition string
$pconfig['partitiontype'] = substr($pconfig['partition'], 0, 1);
$pconfig['partitionnum'] = substr($pconfig['partition'], 1);
$pconfig['partitionnum'] = preg_replace('/(\d+).*/', '\1', $pconfig['partitionnum']);

initmodectrl($pconfig, $pconfig['mode']);

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: disks_mount.php");
		exit;
	}

	// Rebuild partition string
	$_POST['partition'] = "";
	if ("disk" === $_POST['type']) {
		switch ($_POST['partitiontype']) {
			case 'p':
			case 's':
				$_POST['partition'] = $_POST['partitiontype'].trim($_POST['partitionnum']);
				break;
		}
	}

	// Input validation
	switch ($_POST['type']) {
		case "disk":
			$reqdfields = explode(" ", "mdisk partitiontype fstype sharename");
			$reqdfieldsn = array(gettext("Disk"), gettext("Partition type"), gettext("File system"), gettext("Mount point name"));
			$reqdfieldst = explode(" ", "string string string string");
			switch ($_POST['partitiontype']) {
				case 'p':
				case 's':
					$reqdfields = array_merge($reqdfields, explode(" ", "partitionnum"));
					$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Partition number")));
					$reqdfieldst = array_merge($reqdfieldst, explode(" ", "numeric"));
					break;
			}
			break;

		case "iso":
			$reqdfields = explode(" ", "filename sharename");
			$reqdfieldsn = array(gettext("Filename"), gettext("Mount point name"));
			$reqdfieldst = explode(" ", "string string");
			break;
	}

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if (($_POST['sharename'] && !is_validsharename($_POST['sharename']))) {
		$input_errors[] = sprintf(gettext("The attribute '%s' may only consist of the characters a-z, A-Z, 0-9, _ , -."), gettext("Name"));
	}

	if (($_POST['desc'] && !is_validdesc($_POST['desc']))) {
		$input_errors[] = sprintf(gettext("The attribute '%s' contains invalid characters."), gettext("Description"));
	}

	// Do some 'disk' specific checks.
	if ("disk" === $_POST['type']) {
		if (($_POST['partition'] == "p1") && (($_POST['fstype'] == "msdosfs") || ($_POST['fstype'] == "cd9660") || ($_POST['fstype'] == "ntfs") || ($_POST['fstype'] == "ext2fs")))  {
			$input_errors[] = gettext("EFI/GPT partition can be use with UFS only.");
		}

		$device = "{$_POST['mdisk']}{$_POST['partition']}";
		if (($_POST['fstype'] == "ufs") && preg_match("/s\d+$/", $_POST['partition'])) {
			// MBR/UFS
			if (file_exists("{$device}a")) {
				$_POST['partition'] = "{$_POST['partition']}a";
				$device = "{$device}a";
			}
		}
		if ($device === $cfdevice) {
			$input_errors[] = gettext("Can't mount the system partition 1, the DATA partition is the 2.");
		}
		//Check if partition exist
		if (!file_exists($device)) {
			$input_errors[] = gettext("Wrong partition type or partition number.");
		}

		// convert to UFSID
		if ($_POST['fstype'] == "ufs") {
			$ufsid = disks_get_ufsid($device);
			if (empty($ufsid)) {
				$input_errors[] = gettext("Can't get UFS ID.");
			} else {
				$device = "/dev/ufsid/$ufsid";
			}
		}
	}

	// Check if it is a valid ISO image.
	if (("iso" === $_POST['type']) && (FALSE === util_is_iso_image($_POST['filename']))) {
		$input_errors[] = gettext("Selected file isn't an valid ISO file.");
	}

	// Check for duplicates.
	if ("disk" === $_POST['type']) {
		foreach ($a_mount as $mount) {
			if (isset($uuid) && (FALSE !== $cnid) && ($mount['uuid'] === $uuid)) 
				continue;
			if (($mount['mdisk'] === $_POST['mdisk']) && ($mount['partition'] === $_POST['partition'])) {
				$input_errors[] = gettext("The disk/partition is already configured.");
				break;
			}
		}
	}

	// Check whether the mount point name is already in use.
	$index = array_search_ex($_POST['sharename'], $a_mount, "sharename");
	if (FALSE !== $index) {
		// Ensure we do not check the current processed mount point itself.
		if (!((FALSE !== $cnid) && ($a_mount[$cnid]['uuid'] === $a_mount[$index]['uuid']))) {
			$input_errors[] = gettext("Duplicate mount point name.");
		}
	}

	if (!$input_errors) {
		$mount = array();
		$mount['uuid'] = $_POST['uuid'];
		$mount['type'] = $_POST['type'];

		switch($_POST['type']) {
			case "disk":
				$mount['mdisk'] = $_POST['mdisk'];
				$mount['partition'] = $_POST['partition'];
				$mount['fstype'] = $_POST['fstype'];
				if ($mount['fstype'] == "ufs") {
					$mount['devicespecialfile'] = $device;
				} else {
					$mount['devicespecialfile'] = trim("{$mount['mdisk']}{$mount['partition']}");
				}
				$mount['readonly'] = $_POST['readonly'] ? true : false;
				$mount['fsck'] = $_POST['fsck'] ? true : false;
				break;

			case "iso":
				$mount['filename'] = $_POST['filename'];
				$mount['fstype'] = util_is_iso_image($_POST['filename']);
				break;
		}

		$mount['sharename'] = $_POST['sharename'];
		$mount['desc'] = $_POST['desc'];
		$mount['accessrestrictions']['owner'] = $_POST['owner'];
		$mount['accessrestrictions']['group'] = $_POST['group'];
		$mount['accessrestrictions']['mode'] = getmodectrl($pconfig['mode_owner'], $pconfig['mode_group'], $pconfig['mode_others']);

		if (isset($uuid) && (FALSE !== $cnid)) {
			$mode = UPDATENOTIFY_MODE_MODIFIED;
			$a_mount[$cnid] = $mount;
		} else {
			$mode = UPDATENOTIFY_MODE_NEW;
			$a_mount[] = $mount;
		}

		updatenotify_set("mountpoint", $mode, $mount['uuid']);
		write_config();

		header("Location: disks_mount.php");
		exit;
	}
}

function initmodectrl(&$pconfig, $mode) {
	$pconfig['mode_owner'] = array();
	$pconfig['mode_group'] = array();
	$pconfig['mode_others'] = array();

	// Convert octal to decimal
	$mode = octdec($mode);

	// Owner
	if ($mode & 0x0100) $pconfig['mode_owner'][] = "r"; //Read
	if ($mode & 0x0080) $pconfig['mode_owner'][] = "w"; //Write
	if ($mode & 0x0040) $pconfig['mode_owner'][] = "x"; //Execute

	// Group
	if ($mode & 0x0020) $pconfig['mode_group'][] = "r"; //Read
	if ($mode & 0x0010) $pconfig['mode_group'][] = "w"; //Write
	if ($mode & 0x0008) $pconfig['mode_group'][] = "x"; //Execute

	// Others
	if ($mode & 0x0004) $pconfig['mode_others'][] = "r"; //Read
	if ($mode & 0x0002) $pconfig['mode_others'][] = "w"; //Write
	if ($mode & 0x0001) $pconfig['mode_others'][] = "x"; //Execute
}

function getmodectrl($owner, $group, $others) {
		$mode = "";
		$legal = array("r", "w", "x");

		foreach ($legal as $value) {
			$mode .= (is_array($owner) && in_array($value, $owner)) ? $value : "-";
		}
		foreach ($legal as $value) {
			$mode .= (is_array($group) && in_array($value, $group)) ? $value : "-";
		}
		foreach ($legal as $value) {
			$mode .= (is_array($others) && in_array($value, $others)) ? $value : "-";
		}

    $realmode = "";
    $legal = array("", "w", "r", "x", "-");
    $attarray = preg_split("//",$mode);

    for ($i=0; $i<count($attarray); $i++) {
        if ($key = array_search($attarray[$i], $legal)) {
            $realmode .= $legal[$key];
        }
    }

    $mode = str_pad($realmode, 9, '-');
    $trans = array('-'=>'0', 'r'=>'4', 'w'=>'2', 'x'=>'1');
    $mode = strtr($mode, $trans);
    $newmode = "0";
    $newmode .= $mode[0]+$mode[1]+$mode[2];
    $newmode .= $mode[3]+$mode[4]+$mode[5];
    $newmode .= $mode[6]+$mode[7]+$mode[8];

    return $newmode;
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function type_change() {
  switch (document.iform.type.selectedIndex) {
    case 0: /* Disk */
      showElementById('mdisk_tr','show');
      showElementById('partitiontype_tr','show');
      showElementById('partitionnum_tr','show');
      showElementById('fstype_tr','show');
      showElementById('filename_tr','hide');
      showElementById('readonly_tr','show');
      showElementById('fsck_tr','show');
      partitiontype_change();
      break;

    case 1: /* ISO */
      showElementById('mdisk_tr','hide');
      showElementById('partitiontype_tr','hide');
      showElementById('partitionnum_tr','hide');
      showElementById('fstype_tr','hide');
      showElementById('filename_tr','show');
      showElementById('readonly_tr','hide');
      showElementById('fsck_tr','hide');
      break;
  }
}

function partitiontype_change() {
	switch (document.iform.partitiontype.selectedIndex) {
		case 0: /* GPT */
		case 1: /* MBR */
<?php if (!isset($uuid)):?>
			document.iform.fsck.checked = true;
<?php endif;?>
			showElementById('partitionnum_tr','show');
			break;

		case 2: /* CD/DVD */
<?php if (!isset($uuid)):?>
			document.iform.fsck.checked = false;
<?php endif;?>
			showElementById('partitionnum_tr','hide');
			break;
	}
}

function enable_change(enable_change) {
	document.iform.type.disabled = !enable_change;
	document.iform.mdisk.disabled = !enable_change;
	document.iform.filename.disabled = !enable_change;
}
// -->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
        <li class="tabact"><a href="disks_mount.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Management");?></span></a></li>
        <li class="tabinact"><a href="disks_mount_tools.php"><span><?=gettext("Tools");?></span></a></li>
        <li class="tabinact"><a href="disks_mount_fsck.php"><span><?=gettext("Fsck");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
			<form action="disks_mount_edit.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
			  <table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline(gettext("Settings"));?>
					<?php html_combobox("type", gettext("Type"), $pconfig['type'], array("disk" => gettext("Disk"), "iso" => "ISO"), "", true, false, "type_change()");?>
					<tr id="mdisk_tr">
			      <td width="22%" valign="top" class="vncellreq"><?=gettext("Disk");?></td>
			      <td class="vtable">
							<select name="mdisk" class="formfld" id="mdisk">
								<option value=""><?=gettext("Must choose one");?></option>
								<?php foreach ($a_disk as $diskv):?>
								<option value="<?=$diskv['devicespecialfile'];?>" <?php if ($pconfig['mdisk'] === $diskv['devicespecialfile']) echo "selected=\"selected\"";?>>
								<?php $diskinfo = disks_get_diskinfo($diskv['devicespecialfile']); echo htmlspecialchars("{$diskv['name']}: {$diskinfo['mediasize_mbytes']}MB ({$diskv['desc']})");?>
								</option>
								<?php endforeach;?>
							</select>
			      </td>
			    </tr>
			    <tr id="partitiontype_tr">
			      <td width="22%" valign="top" class="vncellreq"><?=gettext("Partition type");?></td>
			      <td class="vtable">
							<select name="partitiontype" class="formfld" id="partitiontype" onclick="partitiontype_change()">
								<option value="p" <?php if ($pconfig['partitiontype'] === "p") echo "selected=\"selected\"";?>><?=gettext("GPT partition");?></option>
								<option value="s" <?php if ($pconfig['partitiontype'] === "s") echo "selected=\"selected\"";?>><?=gettext("MBR partition");?></option>
								<option value=" " <?php if (empty($pconfig['partitiontype'])) echo "selected=\"selected\"";?>><?=gettext("CD/DVD");?></option>
							</select><br />
							<span class="vexpl"><?=gettext("<b>EFI GPT partition</b> if you want to mount a GPT formatted drive (<b>default partition</b>).<br/><b>MBR partition</b> if you want to mount a UFS formatted drive or do imported disks from other OS.<br/><b>CD/DVD volume</b> if you want to mount a CD/DVD volume.");?></span>
			      </td>
			    </tr>
					<?php html_inputbox("partitionnum", gettext("Partition number"), $pconfig['partitionnum'], "", true, 3);?>
					<?php html_combobox("fstype", gettext("File system"), $pconfig['fstype'], array("ufs" => "UFS", "msdosfs" => "FAT", "cd9660" => "CD/DVD", "ntfs" => "NTFS", "ext2fs" => "EXT2"), "", true);?>
					<?php html_filechooser("filename", "Filename", $pconfig['filename'], gettext("ISO file to be mounted."), $g['media_path'], true);?>
					<?php html_inputbox("sharename", gettext("Mount point name"), $pconfig['sharename'], "", true, 20);?>
					<?php html_inputbox("desc", gettext("Description"), $pconfig['desc'], gettext("You may enter a description here for your reference."), false, 40);?>
					<?php html_checkbox("readonly", gettext("Read only"), $pconfig['readonly'] ? true : false, gettext("Mount the file system read-only (even the super-user may not write it)."), "", false);?>
					<?php html_checkbox("fsck", gettext("File system check"), $pconfig['fsck'] ? true : false, gettext("Enable foreground/background file system consistency check during boot process."), "", false);?>
					<?php html_separator();?>
					<?php html_titleline(gettext("Access Restrictions"));?>
					<?php $a_owner = array(); foreach (system_get_user_list() as $userk => $userv) { $a_owner[$userk] = htmlspecialchars($userk); }?>
					<?php html_combobox("owner", gettext("Owner"), $pconfig['owner'], $a_owner, "", false);?>
					<?php $a_group = array(); foreach (system_get_group_list() as $groupk => $groupv) { $a_group[$groupk] = htmlspecialchars($groupk); }?>
					<?php html_combobox("group", gettext("Group"), $pconfig['group'], $a_group, "", false);?>
					<tr>
						<td width="22%" valign="top" class="vncell"><?=gettext("Mode");?></td>
			      <td width="78%" class="vtable">
			      	<table width="100%" border="0" cellpadding="0" cellspacing="0">
				        <tr>
				        	<td width="20%" class="listhdrlr">&nbsp;</td>
									<td width="20%" class="listhdrc"><?=gettext("Read");?></td>
									<td width="50%" class="listhdrc"><?=gettext("Write");?></td>
									<td width="20%" class="listhdrc"><?=gettext("Execute");?></td>
									<td width="10%" class="list"></td>
				        </tr>
				        <tr>
									<td class="listlr"><?=gettext("Owner");?>&nbsp;</td>
									<td class="listrc" align="center"><input type="checkbox" name="mode_owner[]" id="owner_read" value="r" <?php if (in_array("r", $pconfig['mode_owner'])) echo "checked=\"checked\"";?> />&nbsp;</td>
									<td class="listrc" align="center"><input type="checkbox" name="mode_owner[]" id="owner_write" value="w" <?php if (in_array("w", $pconfig['mode_owner'])) echo "checked=\"checked\"";?> />&nbsp;</td>
									<td class="listrc" align="center"><input type="checkbox" name="mode_owner[]" id="owner_execute" value="x" <?php if (in_array("x", $pconfig['mode_owner'])) echo "checked=\"checked\"";?> />&nbsp;</td>
				        </tr>
				        <tr>
				          <td class="listlr"><?=gettext("Group");?>&nbsp;</td>
									<td class="listrc" align="center"><input type="checkbox" name="mode_group[]" id="group_read" value="r" <?php if (in_array("r", $pconfig['mode_group'])) echo "checked=\"checked\"";?> />&nbsp;</td>
									<td class="listrc" align="center"><input type="checkbox" name="mode_group[]" id="group_write" value="w" <?php if (in_array("w", $pconfig['mode_group'])) echo "checked=\"checked\"";?> />&nbsp;</td>
									<td class="listrc" align="center"><input type="checkbox" name="mode_group[]" id="group_execute" value="x" <?php if (in_array("x", $pconfig['mode_group'])) echo "checked=\"checked\"";?> />&nbsp;</td>
				        </tr>
				        <tr>
				          <td class="listlr"><?=gettext("Others");?>&nbsp;</td>
									<td class="listrc" align="center"><input type="checkbox" name="mode_others[]" id="others_read" value="r" <?php if (in_array("r", $pconfig['mode_others'])) echo "checked=\"checked\"";?> />&nbsp;</td>
									<td class="listrc" align="center"><input type="checkbox" name="mode_others[]" id="others_write" value="w" <?php if (in_array("w", $pconfig['mode_others'])) echo "checked=\"checked\"";?> />&nbsp;</td>
									<td class="listrc" align="center"><input type="checkbox" name="mode_others[]" id="others_execute" value="x" <?php if (in_array("x", $pconfig['mode_others'])) echo "checked=\"checked\"";?> />&nbsp;</td>
				        </tr>
							</table>
			      </td>
			    </tr>
			  </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=(isset($uuid) && (FALSE !== $cnid)) ? gettext("Save") : gettext("Add")?>" onclick="enable_change(true)" />
					<input name="Cancel" type="submit" class="formbtn" value="<?=gettext("Cancel");?>" />
					<input name="uuid" type="hidden" value="<?=$pconfig['uuid'];?>" />
				</div>
				<div id="remarks">
					<?php html_remark("warning", gettext("Warning"), sprintf(gettext("You can't mount the partition '%s' where the config file is stored.<br />"),htmlspecialchars($cfdevice)) . sprintf(gettext("UFS and variants are the NATIVE file format for FreeBSD (the underlying OS of %s). Attempting to use other file formats such as FAT, FAT32, EXT2, EXT3, or NTFS can result in unpredictable results, file corruption, and loss of data!"), get_product_name()));?>
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<script type="text/javascript">
<!--
type_change();
<?php if (isset($uuid) && (FALSE !== $cnid)):?>
<!-- Disable controls that should not be modified anymore in edit mode. -->
enable_change(false);
<?php endif;?>
//-->
</script>
<?php include("fend.inc");?>
