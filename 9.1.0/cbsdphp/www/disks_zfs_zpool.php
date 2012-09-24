#!/usr/local/bin/php
<?php
/*
	disks_zfs_zpool.php

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

$pgtitle = array(gettext("Disks"), gettext("ZFS"), gettext("Pools"), gettext("Management"));

if (!isset($config['zfs']['pools']['pool']) || !is_array($config['zfs']['pools']['pool']))
	$config['zfs']['pools']['pool'] = array();

array_sort_key($config['zfs']['pools']['pool'], "name");
$a_pool = &$config['zfs']['pools']['pool'];

if ($_POST) {
	$pconfig = $_POST;

	if ($_POST['apply']) {
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			// Process notifications
			$retval |= updatenotify_process("zfszpool", "zfszpool_process_updatenotification");
		}
		$savemsg = get_std_save_message($retval);
		if ($retval == 0) {
			updatenotify_delete("zfszpool");
		}
		//header("Location: disks_zfs_zpool.php");
		//exit;
	}
}

if ($_GET['act'] === "del") {
	updatenotify_set("zfszpool", UPDATENOTIFY_MODE_DIRTY, $_GET['uuid']);
	header("Location: disks_zfs_zpool.php");
	exit;
}

function zfszpool_process_updatenotification($mode, $data) {
	global $config;

	$retval = 0;

	switch ($mode) {
		case UPDATENOTIFY_MODE_NEW:
			$retval = zfs_zpool_configure($data);
			break;

		case UPDATENOTIFY_MODE_MODIFIED:
			break;

		case UPDATENOTIFY_MODE_DIRTY:
			$cnid = array_search_ex($data, $config['zfs']['pools']['pool'], "uuid");
			if (FALSE !== $cnid) {
				zfs_zpool_destroy($data);
				unset($config['zfs']['pools']['pool'][$cnid]);
				write_config();
			}
			break;
	}

	return $retval;
}

$a_poolstatus = zfs_get_pool_list();
?>
<?php include("fbegin.inc");?>
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
				<li class="tabinact"><a href="disks_zfs_zpool_vdevice.php"><span><?=gettext("Virtual device");?></span></a></li>
				<li class="tabact"><a href="disks_zfs_zpool.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Management");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_zpool_tools.php"><span><?=gettext("Tools");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_zpool_info.php"><span><?=gettext("Information");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_zpool_io.php"><span><?=gettext("I/O statistics");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="disks_zfs_zpool.php" method="post">
				<?php if ($savemsg) print_info_box($savemsg);?>
				<?php if (updatenotify_exists("zfszpool")) print_config_change_box();?>
				<table width="100%" border="0" cellpadding="0" cellspacing="0">
					<tr>
						<td width="15%" class="listhdrlr"><?=gettext("Name");?></td>
						<td width="10%" class="listhdrr"><?=gettext("Size");?></td>
						<td width="10%" class="listhdrr"><?=gettext("Used");?></td>
						<td width="10%" class="listhdrr"><?=gettext("Free");?></td>
						<td width="10%" class="listhdrr"><?=gettext("Capacity");?></td>
						<td width="10%" class="listhdrr"><?=gettext("Dedup");?></td>
						<td width="10%" class="listhdrr"><?=gettext("Health");?></td>
						<td width="15%" class="listhdrr"><?=gettext("AltRoot");?></td>
						<td width="10%" class="list"></td>
					</tr>
					<?php foreach ($a_pool as $poolk => $poolv):?>
					<?php
					$notificationmode = updatenotify_get_mode("zfszpool", $poolv['uuid']);
					$altroot = $cap = $avail = $used = $size = $health = gettext("Unknown");
					if (is_array($a_poolstatus) && array_key_exists($poolv['name'], $a_poolstatus)) {
						$size = $a_poolstatus[$poolv['name']]['size'];
						$used = $a_poolstatus[$poolv['name']]['used'];
						$avail = $a_poolstatus[$poolv['name']]['avail'];
						$cap = $a_poolstatus[$poolv['name']]['cap'];
						$dedup = $a_poolstatus[$poolv['name']]['dedup'];
						$health = $a_poolstatus[$poolv['name']]['health'];
						$altroot = $a_poolstatus[$poolv['name']]['altroot'];
					}
					?>
					<tr>
						<td class="listlr"><?=htmlspecialchars($poolv['name']);?>&nbsp;</td>
						<td class="listr"><?=$size;?>&nbsp;</td>
						<td class="listr"><?=$used;?>&nbsp;</td>
						<td class="listr"><?=$avail;?>&nbsp;</td>
						<td class="listr"><?=$cap;?>&nbsp;</td>
						<td class="listr"><?=$dedup;?>&nbsp;</td>
						<td class="listbg"><a href="disks_zfs_zpool_info.php?pool=<?=$poolv['name']?>"><?=$health;?></a>&nbsp;</td>
						<td class="listr"><?=$altroot;?>&nbsp;</td>	
						<?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>	
						<td valign="middle" nowrap="nowrap" class="list">
							<a href="disks_zfs_zpool_edit.php?uuid=<?=$poolv['uuid'];?>"><img src="e.gif" title="<?=gettext("Edit pool");?>" border="0" alt="<?=gettext("Edit pool");?>" /></a>&nbsp;
							<a href="disks_zfs_zpool.php?act=del&amp;uuid=<?=$poolv['uuid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this pool?");?>')"><img src="x.gif" title="<?=gettext("Delete pool");?>" border="0" alt="<?=gettext("Delete pool");?>" /></a>
						</td>
						<?php else:?>
						<td valign="middle" nowrap="nowrap" class="list">
							<img src="del.gif" border="0" alt="" />
						</td>
						<?php endif;?>
					</tr>
					<?php endforeach;?>
					<tr>
						<td class="list" colspan="8"></td>
						<td class="list">
							<a href="disks_zfs_zpool_edit.php"><img src="plus.gif" title="<?=gettext("Add pool");?>" border="0" alt="<?=gettext("Add pool");?>" /></a>
						</td>
					</tr>
				</table>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
