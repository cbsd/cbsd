#!/usr/local/bin/php
<?php
/*
	disks_zfs_volume.php

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

$pgtitle = array(gettext("Disks"), gettext("ZFS"), gettext("Volumes"), gettext("Volume"));

if (!isset($config['zfs']['volumes']['volume']) || !is_array($config['zfs']['volumes']['volume']))
	$config['zfs']['volumes']['volume'] = array();

array_sort_key($config['zfs']['volumes']['volume'], "name");
$a_volume = &$config['zfs']['volumes']['volume'];

if ($_POST) {
	$pconfig = $_POST;

	if ($_POST['apply']) {
		$retval = 0;

		if (!file_exists($d_sysrebootreqd_path)) {
			// Process notifications
			updatenotify_process("zfsvolume", "zfsvolume_process_updatenotification");
		}
		$savemsg = get_std_save_message($retval);
		if ($retval == 0) {
			updatenotify_delete("zfsvolume");
		}
		header("Location: disks_zfs_volume.php");
		exit;
	}
}

if ($_GET['act'] === "del") {
	updatenotify_set("zfsvolume", UPDATENOTIFY_MODE_DIRTY, $_GET['uuid']);
	header("Location: disks_zfs_volume.php");
	exit;
}

function get_volsize($pool, $name) {
	mwexec2("zfs get -H -o value volsize $pool/$name 2>&1", $rawdata);
	return $rawdata[0];
}

function zfsvolume_process_updatenotification($mode, $data) {
	global $config;

	$retval = 0;

	switch ($mode) {
		case UPDATENOTIFY_MODE_NEW:
			$retval = zfs_volume_configure($data);
			break;

		case UPDATENOTIFY_MODE_MODIFIED:
			$retval = zfs_volume_properties($data);
			break;

		case UPDATENOTIFY_MODE_DIRTY:
			zfs_volume_destroy($data);
			$cnid = array_search_ex($data, $config['zfs']['volumes']['volume'], "uuid");
			if (FALSE !== $cnid) {
				unset($config['zfs']['volumes']['volume'][$cnid]);
				write_config();
			}
			break;
	}

	return $retval;
}
?>
<?php include("fbegin.inc");?>
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
			<form action="disks_zfs_volume.php" method="post">
				<?php if ($savemsg) print_info_box($savemsg);?>
				<?php if (updatenotify_exists("zfsvolume")) print_config_change_box();?>
				<table width="100%" border="0" cellpadding="0" cellspacing="0">
					<tr>
						<td width="20%" class="listhdrlr"><?=gettext("Pool");?></td>
						<td width="25%" class="listhdrr"><?=gettext("Name");?></td>
						<td width="10%" class="listhdrr"><?=gettext("Size");?></td>
						<td width="35%" class="listhdrr"><?=gettext("Description");?></td>
						<td width="10%" class="list"></td>
					</tr>
					<?php foreach ($a_volume as $volumev):?>
					<?php $notificationmode = updatenotify_get_mode("zfsvolume", $volumev['uuid']);?>
					<tr>
						<td class="listlr"><?=htmlspecialchars($volumev['pool'][0]);?>&nbsp;</td>
						<td class="listr"><?=htmlspecialchars($volumev['name']);?>&nbsp;</td>
						<?php if (UPDATENOTIFY_MODE_MODIFIED == $notificationmode || UPDATENOTIFY_MODE_NEW == $notificationmode):?>
						<td class="listr"><?=htmlspecialchars($volumev['volsize']);?>&nbsp;</td>
						<?php else:?>
						<td class="listr"><?=htmlspecialchars(get_volsize($volumev['pool'][0], $volumev['name']));?>&nbsp;</td>
						<?php endif;?>
						<td class="listbg"><?=htmlspecialchars($volumetv['desc']);?>&nbsp;</td>
						<?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
						<td valign="middle" nowrap="nowrap" class="list">
							<a href="disks_zfs_volume_edit.php?uuid=<?=$volumev['uuid'];?>"><img src="e.gif" title="<?=gettext("Edit volume");?>" border="0" alt="<?=gettext("Edit volume");?>" /></a>&nbsp;
							<a href="disks_zfs_volume.php?act=del&amp;uuid=<?=$volumev['uuid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this volume?");?>')"><img src="x.gif" title="<?=gettext("Delete volume");?>" border="0" alt="<?=gettext("Delete volume");?>" /></a>
						</td>
						<?php else:?>
						<td valign="middle" nowrap="nowrap" class="list">
							<img src="del.gif" border="0" alt="" />
						</td>
						<?php endif;?>
					</tr>
					<?php endforeach;?>
					<tr>
						<td class="list" colspan="4"></td>
						<td class="list">
							<a href="disks_zfs_volume_edit.php"><img src="plus.gif" title="<?=gettext("Add volume");?>" border="0" alt="<?=gettext("Add volume");?>" /></a>
						</td>
					</tr>
				</table>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
