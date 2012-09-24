#!/usr/local/bin/php
<?php
/*
	disks_zfs_volume_edit.php

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
require("zfs.inc");

$uuid = $_GET['uuid'];
if (isset($_POST['uuid']))
	$uuid = $_POST['uuid'];

$pgtitle = array(gettext("Disks"), gettext("ZFS"), gettext("Volumes"), gettext("Volume"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['zfs']['pools']['pool']) || !is_array($config['zfs']['pools']['pool']))
	$config['zfs']['pools']['pool'] = array();

if (!isset($config['zfs']['volumes']['volume']) || !is_array($config['zfs']['volumes']['volume']))
	$config['zfs']['volumes']['volume'] = array();

array_sort_key($config['zfs']['pools']['pool'], "name");
array_sort_key($config['zfs']['volumes']['volume'], "name");

$a_pool = &$config['zfs']['pools']['pool'];
$a_volume = &$config['zfs']['volumes']['volume'];

if (!isset($uuid) && (!sizeof($a_pool))) {
	$errormsg = sprintf(gettext("No configured pools. Please add new <a href='%s'>pools</a> first."), "disks_zfs_zpool.php");
}

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_volume, "uuid")))) {
	$pconfig['uuid'] = $a_volume[$cnid]['uuid'];
	$pconfig['name'] = $a_volume[$cnid]['name'];
	$pconfig['pool'] = $a_volume[$cnid]['pool'][0];
	$pconfig['volsize'] = $a_volume[$cnid]['volsize'];
	$pconfig['compression'] = $a_volume[$cnid]['compression'];
	$pconfig['dedup'] = $a_volume[$cnid]['dedup'];
	$pconfig['desc'] = $a_volume[$cnid]['desc'];
} else {
	$pconfig['uuid'] = uuid();
	$pconfig['name'] = "";
	$pconfig['pool'] = "";
	$pconfig['compression'] = "off";
	$pconfig['volsize'] = "";
	$pconfig['dedup'] = "off";
	$pconfig['desc'] = "";
}
if ($pconfig['dedup'] == "") {
	$pconfig['dedup'] = "off";
}

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: disks_zfs_volume.php");
		exit;
	}

	// Input validation
	$reqdfields = explode(" ", "name volsize");
	$reqdfieldsn = array(gettext("Name"), gettext("Size"));
	$reqdfieldst = explode(" ", "string string");

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if (!$input_errors) {
		$volume = array();
		$volume['uuid'] = $_POST['uuid'];
		$volume['name'] = $_POST['name'];
		$volume['pool'] = $_POST['pool'];
		$volume['volsize'] = $_POST['volsize'];
		$volume['compression'] = $_POST['compression'];
		$volume['dedup'] = $_POST['dedup'];
		$volume['desc'] = $_POST['desc'];

		if (isset($uuid) && (FALSE !== $cnid)) {
			$mode = UPDATENOTIFY_MODE_MODIFIED;
			$a_volume[$cnid] = $volume;
		} else {
			$mode = UPDATENOTIFY_MODE_NEW;
			$a_volume[] = $volume;
		}

		updatenotify_set("zfsvolume", $mode, $volume['uuid']);
		write_config();

		header("Location: disks_zfs_volume.php");
		exit;
	}
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	document.iform.name.disabled = !enable_change;
	document.iform.pool.disabled = !enable_change;
}
// -->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav">
				<li class="tabinact"><a href="disks_zfs_zpool.php"><span><?=gettext("Pools");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_dataset.php"><span><?=gettext("Datasets");?></span></a></li>
				<li class="tabact"><a href="disks_zfs_volume.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Volumes");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_snapshot.php"><span><?=gettext("Snapshots");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_config.php"><span><?=gettext("Configuration");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav2">
				<li class="tabact"><a href="disks_zfs_volume.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Volume");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_volume_info.php"><span><?=gettext("Information");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="disks_zfs_volume_edit.php" method="post" name="iform" id="iform">
				<?php if ($errormsg) print_error_box($errormsg);?>
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if (file_exists($d_sysrebootreqd_path)) print_info_box(get_std_save_message(0));?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_inputbox("name", gettext("Name"), $pconfig['name'], "", true, 20);?>
					<?php $a_poollist = array(); foreach ($a_pool as $poolv) { $poolstatus = zfs_get_pool_list(); $poolstatus = $poolstatus[$poolv['name']]; $text = "{$poolv['name']}: {$poolstatus['size']}"; if (!empty($poolv['desc'])) { $text .= " ({$poolv['desc']})"; } $a_poollist[$poolv['name']] = htmlspecialchars($text); }?>
					<?php html_combobox("pool", gettext("Pool"), $pconfig['pool'], $a_poollist, "", true);?>
					<?php html_inputbox("volsize", gettext("Size"), $pconfig['volsize'], gettext("ZFS volume size. To specify the size use the following human-readable suffixes (for example, 'k', 'KB', 'M', 'Gb', etc.)."), true, 10);?>
					<?php $a_compressionmode = array("on" => gettext("On"), "off" => gettext("Off"), "lzjb" => "lzjb", "gzip" => "gzip", "zle" => "zle"); for ($n = 1; $n <= 9; $n++) { $mode = "gzip-{$n}"; $a_compressionmode[$mode] = $mode; }?>
					<?php html_combobox("compression", gettext("Compression"), $pconfig['compression'], $a_compressionmode, gettext("Controls the compression algorithm used for this volume. The 'lzjb' compression algorithm is optimized for performance while providing decent data compression. Setting compression to 'On' uses the 'lzjb' compression algorithm. You can specify the 'gzip' level by using the value 'gzip-N', where N is an integer from 1 (fastest) to 9 (best compression ratio). Currently, 'gzip' is equivalent to 'gzip-6'."), true);?>
					<?php $a_dedup = array("on" => gettext("On"), "off" => gettext("Off"), "verify" => "verify", "sha256" => "sha256", "sha256,verify" => "sha256,verify"); ?>
					<?php html_combobox("dedup", gettext("Dedup"), $pconfig['dedup'], $a_dedup, gettext("Controls the dedup method. <br><b><font color='red'>NOTE/WARNING</font>: See <a href='http://wiki.nas4free.org/doku.php?id=documentation:setup_and_user_guide:disks-zfs-volumes-volume' target='_blank'>ZFS volumes & deduplication</a> wiki article BEFORE using this feature.</b></br>"), true);?>
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
<?php if (isset($uuid) && (FALSE !== $cnid)):?>
<!-- Disable controls that should not be modified anymore in edit mode. -->
enable_change(false);
<?php endif;?>
//-->
</script>
<?php include("fend.inc");?>
