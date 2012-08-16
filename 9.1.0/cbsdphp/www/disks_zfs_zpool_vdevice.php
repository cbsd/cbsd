#!/usr/local/bin/php
<?php
/*
	disks_zfs_zpool_vdevice.php

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.
	
	Modified for XHTML by Daisuke Aoyama <aoyama@peach.ne.jp>
	Copyright (C) 2010 Daisuke Aoyama <aoyama@peach.ne.jp>.	
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	Copyright (C) 2008-2009 Volker Theile <votdev@gmx.de>.
	Copyright (c) 2008 Nelson Silva
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

$pgtitle = array(gettext("Disks"), gettext("ZFS"), gettext("Pools"), gettext("Virtual device"));

if (!isset($config['zfs']['vdevices']['vdevice']) || !is_array($config['zfs']['vdevices']['vdevice']))
	$config['zfs']['vdevices']['vdevice'] = array();

array_sort_key($config['zfs']['vdevices']['vdevice'], "name");
$a_vdevice = &$config['zfs']['vdevices']['vdevice'];

if ($_POST) {
	$pconfig = $_POST;

	if ($_POST['apply']) {
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			// Process notifications
			updatenotify_process("zfsvdev", "zfsvdev_process_updatenotification");
		}
		$savemsg = get_std_save_message($retval);
		if ($retval == 0) {
			updatenotify_delete("zfsvdev");
		}
		header("Location: disks_zfs_zpool_vdevice.php");
		exit;
	}
}

if ($_GET['act'] === "del") {
	$index = array_search_ex($_GET['uuid'], $config['zfs']['vdevices']['vdevice'], "uuid");
	if (false !== $index) {
		updatenotify_set("zfsvdev", UPDATENOTIFY_MODE_DIRTY, $_GET['uuid']);
		header("Location: disks_zfs_zpool_vdevice.php");
		exit;
	}
}

function zfsvdev_process_updatenotification($mode, $data) {
	global $config;

	$retval = 0;

	switch ($mode) {
		case UPDATENOTIFY_MODE_NEW:
			break;

		case UPDATENOTIFY_MODE_MODIFIED:
			break;

		case UPDATENOTIFY_MODE_DIRTY:
			$cnid = array_search_ex($data, $config['zfs']['vdevices']['vdevice'], "uuid");
			if (FALSE !== $cnid) {
				unset($config['zfs']['vdevices']['vdevice'][$cnid]);
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
				<li class="tabact"><a href="disks_zfs_zpool.php" title="<?php echo gettext("Reload page");?>"><span><?php echo gettext("Pools");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_dataset.php"><span><?php echo gettext("Datasets");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_volume.php"><span><?php echo gettext("Volumes");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_snapshot.php"><span><?php echo gettext("Snapshots");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_config.php"><span><?php echo gettext("Configuration");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav2">
				<li class="tabact"><a href="disks_zfs_zpool_vdevice.php" title="<?php echo gettext("Reload page");?>"><span><?php echo gettext("Virtual device");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_zpool.php"><span><?php echo gettext("Management");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_zpool_tools.php"><span><?php echo gettext("Tools");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_zpool_info.php"><span><?php echo gettext("Information");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_zpool_io.php"><span><?php echo gettext("I/O statistics");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="disks_zfs_zpool_vdevice.php" method="post">
				<?php if ($savemsg) print_info_box($savemsg); ?>
				<?php if (updatenotify_exists("zfsvdev")) print_config_change_box();?>
				<table width="100%" border="0" cellpadding="0" cellspacing="0">
					<tr>
						<td width="20%" class="listhdrlr"><?php echo gettext("Name");?></td>
						<td width="15%" class="listhdrr"><?php echo gettext("Type");?></td>
						<td width="55%" class="listhdrr"><?php echo gettext("Description");?></td>
						<td width="10%" class="list"></td>
					</tr>
					<?php foreach ($a_vdevice as $vdevicev):?>
					<?php $notificationmode = updatenotify_get_mode("zfsvdev", $vdevicev['uuid']);?>
					<tr>
						<td class="listlr"><?php htmlspecialchars($vdevicev['name']);?></td>
						<td class="listr"><?php htmlspecialchars($vdevicev['type']);?></td>
						<td class="listbg"><?php htmlspecialchars($vdevicev['desc']);?>&nbsp;</td>
						<?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
						<td valign="middle" nowrap="nowrap" class="list">
							<a href="disks_zfs_zpool_vdevice_edit.php?uuid=<?php $vdevicev['uuid'];?>"><img src="e.gif" title="<?php echo gettext("Edit device");?>" border="0" alt="<?php echo gettext("Edit device");?>" /></a>&nbsp;
							<a href="disks_zfs_zpool_vdevice.php?act=del&amp;uuid=<?php $vdevicev['uuid'];?>" onclick="return confirm('<?php echo gettext("Do you really want to delete this device?");?>')"><img src="x.gif" title="<?php echo gettext("Delete device");?>" border="0" alt="<?php echo gettext("Delete device");?>" /></a>
						</td>
						<?php else:?>
						<td valign="middle" nowrap="nowrap" class="list">
							<img src="del.gif" border="0" alt="" />
						</td>
						<?php endif;?>
					</tr>
					<?php endforeach;?>
					<tr>
						<td class="list" colspan="3"></td>
						<td class="list">
							<a href="disks_zfs_zpool_vdevice_edit.php"><img src="plus.gif" title="<?php echo gettext("Add device");?>" border="0" alt="<?php echo gettext("Add device");?>" /></a>
						</td>
					</tr>
				</table>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
