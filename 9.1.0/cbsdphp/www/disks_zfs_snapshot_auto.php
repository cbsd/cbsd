#!/usr/local/bin/php
<?php
/*
	disks_zfs_snapshot_auto.php

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

$pgtitle = array(gettext("Disks"), gettext("ZFS"), gettext("Snapshots"), gettext("Auto Snapshot"));

if (!isset($config['zfs']['autosnapshots']['autosnapshot']) || !is_array($config['zfs']['autosnapshots']['autosnapshot']))
	$config['zfs']['autosnapshots']['autosnapshot'] = array();

array_sort_key($config['zfs']['autosnapshots']['autosnapshot'], "path");
$a_autosnapshot = &$config['zfs']['autosnapshots']['autosnapshot'];

if (!isset($config['zfs']['pools']['pool']) || !is_array($config['zfs']['pools']['pool']))
	$config['zfs']['pools']['pool'] = array();

array_sort_key($config['zfs']['pools']['pool'], "name");
$a_pool = &$config['zfs']['pools']['pool'];

if (!isset($uuid) && (!sizeof($a_pool))) {
	$errormsg = sprintf(gettext("No configured pools. Please add new <a href='%s'>pools</a> first."), "disks_zfs_zpool.php");
}

$a_timehour = array();
foreach (range(0, 23) as $hour) {
	$min = 0;
	$a_timehour[sprintf("%02.2d%02.2d", $hour, $min)] = sprintf("%02.2d:%02.2d", $hour, $min);
}
$a_lifetime = array("0" => gettext("infinity"),
	    "1w" => sprintf(gettext("%d week"), 1),
	    "2w" => sprintf(gettext("%d weeks"), 2),
	    "30d" => sprintf(gettext("%d days"), 30),
	    "90d" => sprintf(gettext("%d days"), 90),
	    "180d" => sprintf(gettext("%d days"), 180),
	    "1y" => sprintf(gettext("%d year"), 1),
	    "2y" => sprintf(gettext("%d years"), 2));

if ($_POST) {
	$pconfig = $_POST;

	if ($_POST['apply']) {
		$ret = array("output" => array(), "retval" => 0);

		if (!file_exists($d_sysrebootreqd_path)) {
			// Process notifications
			$ret = zfs_updatenotify_process("zfsautosnapshot", "zfsautosnapshot_process_updatenotification");
			config_lock();
			$ret['retval'] |= rc_update_service("autosnapshot");
			config_unlock();
		}
		$savemsg = get_std_save_message($ret['retval']);
		if ($ret['retval'] == 0) {
			updatenotify_delete("zfsautosnapshot");
			header("Location: disks_zfs_snapshot_auto.php");
			exit;
		}
		updatenotify_delete("zfsautosnapshot");
		$errormsg = implode("\n", $ret['output']);
	}
}

if ($_GET['act'] === "del") {
	$autosnapshot = array();
	$autosnapshot['uuid'] = $_GET['uuid'];
	updatenotify_set("zfsautosnapshot", UPDATENOTIFY_MODE_DIRTY, serialize($autosnapshot));
	header("Location: disks_zfs_snapshot_auto.php");
	exit;
}

function zfsautosnapshot_process_updatenotification($mode, $data) {
	global $config;

	$ret = array("output" => array(), "retval" => 0);

	switch ($mode) {
		case UPDATENOTIFY_MODE_NEW:
			$data = unserialize($data);
			//$ret = zfs_snapshot_configure($data);
			break;

		case UPDATENOTIFY_MODE_MODIFIED:
			$data = unserialize($data);
			//$ret = zfs_snapshot_properties($data);
			break;

		case UPDATENOTIFY_MODE_DIRTY:
			$data = unserialize($data);
			$cnid = array_search_ex($data['uuid'], $config['zfs']['autosnapshots']['autosnapshot'], "uuid");
			if (FALSE !== $cnid) {
				unset($config['zfs']['autosnapshots']['autosnapshot'][$cnid]);
				write_config();
			}
			break;
	}

	return $ret;
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav">
				<li class="tabinact"><a href="disks_zfs_zpool.php"><span><?=gettext("Pools");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_dataset.php"><span><?=gettext("Datasets");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_volume.php"><span><?=gettext("Volumes");?></span></a></li>
				<li class="tabact"><a href="disks_zfs_snapshot.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Snapshots");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_config.php"><span><?=gettext("Configuration");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav2">
				<li class="tabinact"><a href="disks_zfs_snapshot.php"><span><?=gettext("Snapshot");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_snapshot_clone.php"><span><?=gettext("Clone");?></span></a></li>
				<li class="tabact"><a href="disks_zfs_snapshot_auto.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Auto Snapshot");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_snapshot_info.php"><span><?=gettext("Information");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="disks_zfs_snapshot_auto.php" method="post">
				<?php if ($errormsg) print_error_box($errormsg);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<?php if (updatenotify_exists("zfsautosnapshot")) print_config_change_box();?>
				<table width="100%" border="0" cellpadding="0" cellspacing="0">
					<tr>
						<td width="30%" class="listhdrlr"><?=gettext("Path");?></td>
						<td width="20%" class="listhdrr"><?=gettext("Name");?></td>
						<td width="10%" class="listhdrr"><?=gettext("Recursive");?></td>
						<td width="10%" class="listhdrr"><?=gettext("Type");?></td>
						<td width="10%" class="listhdrr"><?=gettext("Schedule time");?></td>
						<td width="10%" class="listhdrr"><?=gettext("Life time");?></td>
						<td width="10%" class="list"></td>
					</tr>
					<?php foreach ($a_autosnapshot as $autosnapshotv):?>
					<?php $notificationmode = updatenotify_get_mode("zfsautosnapshot", serialize(array('uuid' => $autosnapshotv['uuid'])));?>
					<tr>
						<td class="listlr"><?=htmlspecialchars($autosnapshotv['path']);?>&nbsp;</td>
						<td class="listr"><?=htmlspecialchars($autosnapshotv['name']);?>&nbsp;</td>
						<td class="listr"><?=htmlspecialchars(isset($autosnapshotv['recursive']) ? "yes" : "no");?>&nbsp;</td>
						<td class="listr"><?=htmlspecialchars($autosnapshotv['type']);?>&nbsp;</td>
						<td class="listr"><?=htmlspecialchars($a_timehour[$autosnapshotv['timehour']]);?>&nbsp;</td>
						<td class="listr"><?=htmlspecialchars($a_lifetime[$autosnapshotv['lifetime']]);?>&nbsp;</td>
						<?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
						<td valign="middle" nowrap="nowrap" class="list">
							<a href="disks_zfs_snapshot_auto_edit.php?uuid=<?=$autosnapshotv['uuid'];?>"><img src="e.gif" title="<?=gettext("Edit auto snapshot");?>" border="0" alt="<?=gettext("Edit auto snapshot");?>" /></a>&nbsp;
							<a href="disks_zfs_snapshot_auto.php?act=del&amp;uuid=<?=$autosnapshotv['uuid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this auto snapshot?");?>')"><img src="x.gif" title="<?=gettext("Delete auto snapshot");?>" border="0" alt="<?=gettext("Delete auto snapshot");?>" /></a>
						</td>
						<?php else:?>
						<td valign="middle" nowrap="nowrap" class="list">
							<img src="del.gif" border="0" alt="" />
						</td>
						<?php endif;?>
					</tr>
					<?php endforeach;?>
					<tr>
						<td class="list" colspan="6"></td>
						<td class="list">
							<a href="disks_zfs_snapshot_auto_edit.php"><img src="plus.gif" title="<?=gettext("Add auto snapshot");?>" border="0" alt="<?=gettext("Add auto snapshot");?>" /></a>
						</td>
					</tr>
				</table>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
