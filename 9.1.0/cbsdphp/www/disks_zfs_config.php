#!/usr/local/bin/php
<?php
/*
	disks_zfs_config.php

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

$pgtitle = array(gettext('Disks'), gettext('ZFS'), gettext('Configuration'), gettext('Detected'));

$zfs = array(
	'vdevices' => array(
		'vdevice' => array()
	),
	'pools' => array(
		'pool' => array()
	),
	'datasets' => array(
		'dataset' => array()
	),
	'volumes' => array(
		'volume' => array()
	),
);

if (isset($_POST['import']))
{
	$cmd = 'zpool import -d /dev -a';

	if (isset($_POST['import_force']))
	{
		$cmd .= ' -f';
	}

	$retval = mwexec($cmd);
}

$rawdata = null;
$spa = @exec("sysctl -q -n vfs.zfs.version.spa");
if ($spa == '' || $spa < 21) {
	mwexec2('zfs list -H -t filesystem -o name,mountpoint,compression,canmount,quota,used,available,xattr,snapdir,readonly,origin', $rawdata);
} else {
	mwexec2('zfs list -H -t filesystem -o name,mountpoint,compression,canmount,quota,used,available,xattr,snapdir,readonly,origin,dedup', $rawdata);
}
foreach($rawdata as $line)
{
	if ($line == 'no datasets available') { continue; }
	list($fname, $mpoint, $compress, $canmount, $quota, $used, $avail, $xattr, $snapdir, $readonly, $origin, $dedup) = explode("\t", $line);
	if (strpos($fname, '/') !== false) // dataset
	{
		if (empty($origin) || $origin != '-') continue;
		list($pool, $name) = explode('/', $fname, 2);
		$zfs['datasets']['dataset'][$name] = array(
			'uuid' => uuid(),
			'name' => $name,
			'pool' => $pool,
			'compression' => $compress,
			'canmount' => ($canmount == 'on') ? null : $canmount,
			'quota' => ($quota == 'none') ? null : $quota,
			'xattr' => ($xattr == 'on'),
			'snapdir' => ($snapdir == 'visible'),
			'readonly' => ($readonly == 'on'),
			'dedup' => $dedup,
		);
	}
	else // zpool
	{
		$zfs['pools']['pool'][$fname] = array(
			'uuid' => uuid(),
			'name' => $fname,
			'vdevice' => array(),
			'root' => null,
			'mountpoint' => ($mpoint == "/mnt/{$fname}") ? null : $mpoint,
		);
		$zfs['extra']['pools']['pool'][$fname] = array(
			'size' => null,
			'used' => $used,
			'avail' => $avail,
			'cap' => null,
			'health' => null,
		);
	}
}

$rawdata = null;
$spa = @exec("sysctl -q -n vfs.zfs.version.spa");
if ($spa == '' || $spa < 21) {
	mwexec2('zfs list -H -t volume -o name,volsize,compression,origin', $rawdata);
} else {
	mwexec2('zfs list -H -t volume -o name,volsize,compression,origin,dedup', $rawdata);
}
foreach($rawdata as $line)
{
	if ($line == 'no datasets available') { continue; }
	list($fname, $volsize, $compress, $origin, $dedup) = explode("\t", $line);
	if (strpos($fname, '/') !== false) // volume
	{
		if (empty($origin) || $origin != '-') continue;
		list($pool, $name) = explode('/', $fname, 2);
		$zfs['volumes']['volume'][$name] = array(
			'uuid' => uuid(),
			'name' => $name,
			'pool' => $pool,
			'volsize' => $volsize,
			'compression' => $compress,
			'dedup' => $dedup,
		);
	}
}

$rawdata = null;
$spa = @exec("sysctl -q -n vfs.zfs.version.spa");
if ($spa == '') {
	mwexec2('zpool list -H -o name,root,size,capacity,health', $rawdata);
} else if ($spa < 21) {
	mwexec2("zpool list -H -o name,altroot,size,capacity,health", $rawdata);
} else {
	mwexec2("zpool list -H -o name,altroot,size,capacity,health,dedup", $rawdata);
}
foreach ($rawdata as $line)
{
	if ($line == 'no pools available') { continue; }
	list($pool, $root, $size, $cap, $health, $dedup) = explode("\t", $line);
	if ($root != '-')
	{
		$zfs['pools']['pool'][$pool]['root'] = $root;
	}
	$zfs['extra']['pools']['pool'][$pool]['size'] = $size;
	$zfs['extra']['pools']['pool'][$pool]['cap'] = $cap;
	$zfs['extra']['pools']['pool'][$pool]['health'] = $health;
	$zfs['extra']['pools']['pool'][$pool]['dedup'] = $dedup;
}

$pool = null;
$vdev = null;
$type = null;
$i = 0;
$vdev_type = array('mirror', 'raidz1', 'raidz2', 'raidz3');

$rawdata = null;
mwexec2('zpool status', $rawdata);
foreach ($rawdata as $line)
{
	if ($line[0] != "\t") continue;

	if (!is_null($vdev) && preg_match('/^\t    (\S+)/', $line, $m)) // dev
	{
		$dev = $m[1];
		if (preg_match("/^(.+)\.nop$/", $dev, $m)) {
			$zfs['vdevices']['vdevice'][$vdev]['device'][] = "/dev/{$m[1]}";
			$zfs['vdevices']['vdevice'][$vdev]['aft4k'] = true;
		} else if (preg_match("/^(.+)\.eli$/", $dev, $m)) {
			//$zfs['vdevices']['vdevice'][$vdev]['device'][] = "/dev/{$m[1]}";
			$zfs['vdevices']['vdevice'][$vdev]['device'][] = "/dev/{$dev}";
		} else {
			$zfs['vdevices']['vdevice'][$vdev]['device'][] = "/dev/{$dev}";
		}
	}
	else if (!is_null($pool) && preg_match('/^\t  (\S+)/', $line, $m)) // vdev or dev (type disk)
	{
		$is_vdev_type = true;
		if ($type == 'spare') // disk in vdev type spares
		{
			$dev = $m[1];
		}
		else if ($type == 'cache')
		{
			$dev = $m[1];
		}
		else if ($type == 'log')
		{
			$dev = $m[1];
			if ($dev == 'mirror') {
				$type = "log-mirror";
			}
		}
		else // vdev or dev (type disk)
		{
			$type = $m[1];
			if (preg_match("/^(.*)\-\d+$/", $type, $m)) {
				$tmp = $m[1];
				$is_vdev_type = in_array($tmp, $vdev_type);
				if ($is_vdev_type)
					$type = $tmp;
			} else {
				$is_vdev_type = in_array($type, $vdev_type);
			}
			if (!$is_vdev_type) // type disk
			{
				$dev = $type;
				$type = 'disk';
				$vdev = sprintf("%s_%s_%d", $pool, $type, $i++);
			}
			else // vdev
			{
				$vdev = sprintf("%s_%s_%d", $pool, $type, $i++);
			}
		}
		if (!array_key_exists($vdev, $zfs['vdevices']['vdevice'])) {
			$zfs['vdevices']['vdevice'][$vdev] = array(
				'uuid' => uuid(),
				'name' => $vdev,
				'type' => $type,
				'device' => array(),
			);
			$zfs['extra']['vdevices']['vdevice'][$vdev]['pool'] = $pool;
			$zfs['pools']['pool'][$pool]['vdevice'][] = $vdev;
		}
		if ($type == 'spare' || $type == 'cache' || $type == 'log' || $type == 'disk')
		{
			if (preg_match("/^(.+)\.nop$/", $dev, $m)) {
				$zfs['vdevices']['vdevice'][$vdev]['device'][] = "/dev/{$m[1]}";
				$zfs['vdevices']['vdevice'][$vdev]['aft4k'] = true;
			} else if (preg_match("/^(.+)\.eli$/", $dev, $m)) {
				//$zfs['vdevices']['vdevice'][$vdev]['device'][] = "/dev/{$m[1]}";
				$zfs['vdevices']['vdevice'][$vdev]['device'][] = "/dev/{$dev}";
			} else {
				$zfs['vdevices']['vdevice'][$vdev]['device'][] = "/dev/{$dev}";
			}
		}
	}
	else if (preg_match('/^\t(\S+)/', $line, $m)) // zpool or spares
	{
		$vdev = null;
		$type = null;
		if ($m[1] == 'spares')
		{
			$type = 'spare';
			$vdev = sprintf("%s_%s_%d", $pool, $type, $i++);
		}
		else if ($m[1] == 'cache')
		{
			$type = 'cache';
			$vdev = sprintf("%s_%s_%d", $pool, $type, $i++);
		}
		else if ($m[1] == 'logs')
		{
			$type = 'log';
			$vdev = sprintf("%s_%s_%d", $pool, $type, $i++);
		}
		else
		{
			$pool = $m[1];
		}
	}
}

if (count($zfs['pools']['pool']) <= 0)
{
	$import_button_value = gettext('Import on-disk ZFS config');
	if (isset($_POST['import']))
	{
		$message_box_type = 'warning';
		$message_box_text = gettext('No pool was found.');
		if (isset($retval) && $retval != 0)
		{
			if (isset($_POST['import_force']))
			{
				$message_box_text = 'error';
			}
			else
			{
				$authToken = Session::getAuthToken();
				$message_box_text .= ' ';
				$message_box_text .= gettext('Try to force import.');
				$message_box_text = <<<HTML
<br />
<form action="{$_SERVER['PHP_SELF']}" method="post">
	{$message_box_text}<br />
	<input type="submit" name="import" value="{$import_button_value}" />
	<input type="hidden" name="import_force" value="true" />
	<input name="authtoken" type="hidden" value="{$authToken}" autocomplete="off">
</form>
HTML;
			}
		}
	} else {
		$authToken = Session::getAuthToken();
		$message_box_type = 'info';
		$text = gettext('No pool was found.').' '.gettext('Try to import from on-disk ZFS config.');
		$message_box_text = <<<HTML
<form action="{$_SERVER['PHP_SELF']}" method="post">
	{$text}<br />
	<input type="submit" name="import" value="{$import_button_value}" />
	<input name="authtoken" type="hidden" value="{$authToken}" autocomplete="off">
</form>
HTML;
	}
}

$health = true;
$health &= (bool)!array_search_ex('DEGRADED', $zfs['extra']['pools']['pool'], 'health');
$health &= (bool)!array_search_ex('FAULTED', $zfs['extra']['pools']['pool'], 'health');

if (!$health)
{
	$message_box_type = 'warning';
	$message_box_text = gettext('Your ZFS system is not healthy.');
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
				<li class="tabinact"><a href="disks_zfs_snapshot.php"><span><?=gettext("Snapshots");?></span></a></li>
				<li class="tabact"><a href="disks_zfs_config.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Configuration");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav2">
				<li class="tabinact"><a href="disks_zfs_config_current.php"><span><?=gettext("Current");?></span></a></li>
				<li class="tabact" title="<?=gettext("Reload page");?>"><a href="disks_zfs_config.php"><span><?=gettext("Detected");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_config_sync.php"><span><?=gettext("Synchronize");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<?php if (isset($message_box_text)) print_core_box($message_box_type, $message_box_text);?>
			<table width="100%" border="0" cellpadding="0" cellspacing="0">
				<?php html_titleline(gettext('Pools').' ('.count($zfs['pools']['pool']).')', 8);?>
				<tr>
					<td width="16%" class="listhdrlr"><?=gettext("Name");?></td>
					<td width="12%" class="listhdrr"><?=gettext("Size");?></td>
					<td width="12%" class="listhdrr"><?=gettext("Used");?></td>
					<td width="12%" class="listhdrr"><?=gettext("Free");?></td>
					<td width="12%" class="listhdrr"><?=gettext("Dedup");?></td>
					<td width="12%" class="listhdrr"><?=gettext("Health");?></td>
					<td width="12%" class="listhdrr"><?=gettext("Mount point");?></td>
					<td width="12%" class="listhdrr"><?=gettext("AltRoot");?></td>
				</tr>
				<?php foreach ($zfs['pools']['pool'] as $key => $pool):?>
				<tr>
					<td class="listlr"><?= $pool['name']; ?></td>
					<td class="listr"><?= $zfs['extra']['pools']['pool'][$key]['size']; ?></td>
					<td class="listr"><?= $zfs['extra']['pools']['pool'][$key]['used']; ?> (<?= $zfs['extra']['pools']['pool'][$key]['cap']; ?>)</td>
					<td class="listr"><?= $zfs['extra']['pools']['pool'][$key]['avail']; ?></td>
					<td class="listr"><?= $zfs['extra']['pools']['pool'][$key]['dedup']; ?></td>
					<td class="listr"><?= $zfs['extra']['pools']['pool'][$key]['health']; ?></td>
					<td class="listr"><?= $pool['mountpoint']; ?></td>
					<td class="listr"><?= empty($pool['root']) ? '-' : $pool['root']; ?></td>
				</tr>
				<?php endforeach; ?>
			</table>
			<br />
			<table width="100%" border="0" cellpadding="0" cellspacing="0">
				<?php html_titleline(gettext('Virtual devices').' ('.count($zfs['vdevices']['vdevice']).')', 4);?>
				<tr>
					<td width="16%" class="listhdrlr"><?=gettext("Name");?></td>
					<td width="21%" class="listhdrr"><?=gettext("Type");?></td>
					<td width="21%" class="listhdrr"><?=gettext("Pool");?></td>
					<td width="42%" class="listhdrr"><?=gettext("Devices");?></td>
				</tr>
				<?php foreach ($zfs['vdevices']['vdevice'] as $key => $vdevice):?>
				<tr>
					<td class="listlr"><?= $vdevice['name']; ?></td>
					<td class="listr"><?= $vdevice['type']; ?></td>
					<td class="listr"><?= $zfs['extra']['vdevices']['vdevice'][$key]['pool']; ?></td>
					<td class="listr"><?= implode(', ', $vdevice['device']); ?></td>
				</tr>
				<?php endforeach; ?>
			</table>
			<br />
			<table width="100%" border="0" cellpadding="0" cellspacing="0">
				<?php html_titleline(gettext('Datasets').' ('.count($zfs['datasets']['dataset']).')', 9);?>
				<tr>
					<td width="16%" class="listhdrlr"><?=gettext("Name");?></td>
					<td width="14%" class="listhdrr"><?=gettext("Pool");?></td>
					<td width="10%" class="listhdrr"><?=gettext("Compression");?></td>
					<td width="10%" class="listhdrr"><?=gettext("Dedup");?></td>
					<td width="10%" class="listhdrr"><?=gettext("Canmount");?></td>
					<td width="10%" class="listhdrr"><?=gettext("Quota");?></td>
					<td width="10%" class="listhdrr"><?=gettext("Extended attributes");?></td>
					<td width="10%" class="listhdrr"><?=gettext("Readonly");?></td>
					<td width="10%" class="listhdrr"><?=gettext("Snapshot Visibility");?></td>
				</tr>
				<?php foreach ($zfs['datasets']['dataset'] as $dataset):?>
				<tr>
					<td class="listlr"><?= $dataset['name']; ?></td>
					<td class="listr"><?= $dataset['pool']; ?></td>
					<td class="listr"><?= $dataset['compression']; ?></td>
					<td class="listr"><?= $dataset['dedup']; ?></td>
					<td class="listr"><?= empty($dataset['canmount']) ? 'on' : $dataset['canmount']; ?></td>
					<td class="listr"><?= empty($dataset['quota']) ? 'none' : $dataset['quota']; ?></td>
					<td class="listr"><?= empty($dataset['xattr']) ? 'off' : 'on'; ?></td>
					<td class="listr"><?= empty($dataset['readonly']) ? 'off' : 'on'; ?></td>
					<td class="listr"><?= empty($dataset['snapdir']) ? 'hidden' : 'visible'; ?></td>
				</tr>
				<?php endforeach; ?>
			</table>
			<br />
			<table width="100%" border="0" cellpadding="0" cellspacing="0">
				<?php html_titleline(gettext('Volumes').' ('.count($zfs['volumes']['volume']).')', 5);?>
				<tr>
					<td width="16%" class="listhdrlr"><?=gettext("Name");?></td>
					<td width="21%" class="listhdrr"><?=gettext("Pool");?></td>
					<td width="21%" class="listhdrr"><?=gettext("Size");?></td>
					<td width="21%" class="listhdrr"><?=gettext("Compression");?></td>
					<td width="21%" class="listhdrr"><?=gettext("Dedup");?></td>
				</tr>
				<?php foreach ($zfs['volumes']['volume'] as $volume):?>
				<tr>
					<td class="listlr"><?= $volume['name']; ?></td>
					<td class="listr"><?= $volume['pool']; ?></td>
					<td class="listr"><?= $volume['volsize']; ?></td>
					<td class="listr"><?= $volume['compression']; ?></td>
					<td class="listr"><?= $volume['dedup']; ?></td>
				</tr>
				<?php endforeach;?>
			</table>
			<div id="remarks">
				<?php html_remark("note", gettext("Note"), gettext("This page reflects the current system configuration. It may be different to the configuration which has been created with the WebGUI if changes has been done via command line."));?>
			</div>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
