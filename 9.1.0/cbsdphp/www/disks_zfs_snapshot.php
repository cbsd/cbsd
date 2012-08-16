#!/usr/local/bin/php
<?php
/*
	disks_zfs_snapshot.php
	
	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.
	
	Modified for XHTML by Daisuke Aoyama <aoyama@peach.ne.jp>
	Copyright (C) 2010 Daisuke Aoyama <aoyama@peach.ne.jp>.	
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	Copyright (C) 2008-2009 Volker Theile <votdev@gmx.de>.
	All rights reserved.
	
	Based on m0n0wall (http://m0n0.ch/wall)
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
require("zfs.inc");

$pgtitle = array(gettext("Disks"), gettext("ZFS"), gettext("Snapshots"), gettext("Snapshot"));

if (!isset($config['zfs']['snapshots']['snapshot']) || !is_array($config['zfs']['snapshots']['snapshot']))
	$config['zfs']['snapshots']['snapshot'] = array();

array_sort_key($config['zfs']['snapshots']['snapshot'], "name");
$a_snapshot = &$config['zfs']['snapshots']['snapshot'];

function get_zfs_snapshots() {
	$result = array();
	mwexec2("zfs list -H -o name,used,creation -t snapshot 2>&1", $rawdata);
	foreach ($rawdata as $line) {
		$a = preg_split("/\t/", $line);
		$r = array();
		$name = $a[0];
		$r['snapshot'] = $name;
		if (preg_match('/^([^\/\@]+)(\/([^\@]+))?\@(.*)$/', $name, $m)) {
			$r['pool'] = $m[1];
			$r['name'] = $m[4];
			$r['path'] = $m[1].$m[2];
		} else {
			$r['pool'] = 'unknown'; // XXX
			$r['name'] = 'unknown'; // XXX
			$r['path'] = $name;
		}
		$r['used'] = $a[1];
		$r['creation'] = $a[2];
		$result[] = $r;
	}
	return $result;
}
$a_snapshot_all = get_zfs_snapshots();

$filter_time = "1week";
if (isset($_SESSION['filter_time'])) {
	$filter_time = $_SESSION['filter_time'];
}
$a_filter_time = array(
	    "1week" => sprintf(gettext("%d week"), 1),
	    "2weeks" => sprintf(gettext("%d weeks"), 2),
	    "30days" => sprintf(gettext("%d days"), 30),
	    "90days" => sprintf(gettext("%d days"), 90),
	    "180days" => sprintf(gettext("%d days"), 180),
	    "0" => gettext("All"));

function get_zfs_snapshots_filter($snapshots, $filter) {
	$now = time() / 86400;
	$now *= 86400;
	if ($filter['time'] != 0) {
		$f_time = strtotime("-".$filter['time'], $now);
	} else {
		$f_time = 0;
	}

	$result = array();
	foreach ($snapshots as $v) {
		$t = strtotime($v['creation']);
		if ($f_time != 0 && $t < $f_time) continue;
		$result[] = $v;
	}
	return $result;
}
$a_snapshot = get_zfs_snapshots_filter($a_snapshot_all, array('time' => $filter_time));

if ($_POST) {
	$pconfig = $_POST;

	if ($_POST['filter']) {
		$_SESSION['filter_time'] = $_POST['filter_time'];
		header("Location: disks_zfs_snapshot.php");
		exit;
	}
	if ($_POST['apply']) {
		$ret = array("output" => array(), "retval" => 0);

		if (!file_exists($d_sysrebootreqd_path)) {
			// Process notifications
			$ret = zfs_updatenotify_process("zfssnapshot", "zfssnapshot_process_updatenotification");

		}
		$savemsg = get_std_save_message($ret['retval']);
		if ($ret['retval'] == 0) {
			updatenotify_delete("zfssnapshot");
			header("Location: disks_zfs_snapshot.php");
			exit;
		}
		updatenotify_delete("zfssnapshot");
		$errormsg = implode("\n", $ret['output']);
	}
}

if ($_GET['act'] === "del") {
	$snapshot = array();
	$snapshot['snapshot'] = $_GET['snapshot'];
	$snapshot['recursive'] = false;
	updatenotify_set("zfssnapshot", UPDATENOTIFY_MODE_DIRTY, serialize($snapshot));
	header("Location: disks_zfs_snapshot.php");
	exit;
}

function zfssnapshot_process_updatenotification($mode, $data) {
	global $config;

	$ret = array("output" => array(), "retval" => 0);

	switch ($mode) {
		case UPDATENOTIFY_MODE_NEW:
			$data = unserialize($data);
			$ret = zfs_snapshot_configure($data);
			break;

		case UPDATENOTIFY_MODE_MODIFIED:
			$data = unserialize($data);
			$ret = zfs_snapshot_properties($data);
			break;

		case UPDATENOTIFY_MODE_DIRTY:
			$data = unserialize($data);
			$ret = zfs_snapshot_destroy($data);
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
				<li class="tabinact"><a href="disks_zfs_zpool.php"><span><?php echo gettext("Pools");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_dataset.php"><span><?php echo gettext("Datasets");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_volume.php"><span><?php echo gettext("Volumes");?></span></a></li>
				<li class="tabact"><a href="disks_zfs_snapshot.php" title="<?php echo gettext("Reload page");?>"><span><?php echo gettext("Snapshots");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_config.php"><span><?php echo gettext("Configuration");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav2">
				<li class="tabact"><a href="disks_zfs_snapshot.php" title="<?php echo gettext("Reload page");?>"><span><?php echo gettext("Snapshot");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_snapshot_clone.php"><span><?php echo gettext("Clone");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_snapshot_auto.php"><span><?php echo gettext("Auto Snapshot");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_snapshot_info.php"><span><?php echo gettext("Information");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="disks_zfs_snapshot.php" method="post">
				<?php if ($errormsg) print_error_box($errormsg);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<?php if (updatenotify_exists("zfssnapshot")) print_config_change_box();?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<tr id="filter_tr">
						<td width="22%" valign="top" class="vncell"><?php echo gettext("Filter"); ?></td>
						<td width="78%" class="vtable">
							<select name='filter_time' class='formfld' id='filter_time' >
							<?
								foreach ($a_filter_time as $k => $v) {
									$sel = $filter_time == $k ? "selected='selected'" : "";
									echo "<option value='${k}' $sel>${v}</option>\n";
								}
							?>
							</select>
							<input name="filter" type="submit" class="formbtn" id="filter" value="<?php echo gettext("Apply");?>" />
						</td>
					</tr>
					<?php html_separator();?>
				</table>
				<table width="100%" border="0" cellpadding="0" cellspacing="0">
					<tr>
						<td width="40%" class="listhdrlr"><?php echo gettext("Path");?><? echo sprintf(" (%d/%d)", count($a_snapshot), count($a_snapshot_all)); ?></td>
						<td width="20%" class="listhdrr"><?php echo gettext("Name");?></td>
						<td width="10%" class="listhdrr"><?php echo gettext("Used");?></td>
						<td width="20%" class="listhdrr"><?php echo gettext("Creation");?></td>
						<td width="10%" class="list"></td>
					</tr>
					<?php foreach ($a_snapshot as $snapshotv):?>
					<?php $notificationmode = updatenotify_get_mode("zfssnapshot", serialize(array('snapshot' => $snapshotv['snapshot'], 'recursive'=> false)));?>
					<tr>
						<td class="listlr"><?php htmlspecialchars($snapshotv['path']);?>&nbsp;</td>
						<td class="listr"><?php htmlspecialchars($snapshotv['name']);?>&nbsp;</td>
						<?php if (UPDATENOTIFY_MODE_MODIFIED == $notificationmode):?>
						<td class="listr"><?php htmlspecialchars($snapshotv['used']);?>&nbsp;</td>
						<?php else:?>
						<td class="listr"><?php htmlspecialchars($snapshotv['used']);?>&nbsp;</td>
						<?php endif;?>
						<td class="listr"><?php htmlspecialchars($snapshotv['creation']);?>&nbsp;</td>
						<?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
						<td valign="middle" nowrap="nowrap" class="list">
							<a href="disks_zfs_snapshot_edit.php?snapshot=<?php urlencode($snapshotv['snapshot']);?>"><img src="e.gif" title="<?php echo gettext("Edit snapshot");?>" border="0" alt="<?php echo gettext("Edit snapshot");?>" /></a>&nbsp;
							<a href="disks_zfs_snapshot.php?act=del&amp;snapshot=<?php urlencode($snapshotv['snapshot']);?>" onclick="return confirm('<?php echo gettext("Do you really want to delete this snapshot?");?>')"><img src="x.gif" title="<?php echo gettext("Delete snapshot");?>" border="0" alt="<?php echo gettext("Delete snapshot");?>" /></a>
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
							<a href="disks_zfs_snapshot_add.php"><img src="plus.gif" title="<?php echo gettext("Add snapshot");?>" border="0" alt="<?php echo gettext("Add snapshot");?>" /></a>
						</td>
					</tr>
				</table>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
