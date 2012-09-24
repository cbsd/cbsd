#!/usr/local/bin/php
<?php
/*
	disks_zfs_config_sync.php
	
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

$pgtitle = array(gettext("Disks"), gettext("ZFS"), gettext("Configuration"), gettext("Synchronize"));

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

function get_geli_info($device) {
	$result = array();
	exec("/sbin/geli dump {$device}", $rawdata);
	array_shift($rawdata);
	foreach($rawdata as $line) {
		$a = preg_split("/:\s+/", $line);
		$key = trim($a[0]);
		$val = trim($a[1]);
		$result[$key] = $val;
	}
	return $result;
}

if (isset($_POST['import_config']))
{
	$import = false;
	$cfg['zfs'] = array(
		'vdevices' => array(),
		'pools' => array(),
		'datasets' => array(),
		'volumes' => array(),
		'autosnapshots' => array(),
	);
	if (!isset($_POST['vol'])) { $_POST['vol'] = array(); }
	if (!isset($_POST['dset'])) { $_POST['dset'] = array(); }
	if (!isset($_POST['vdev'])) { $_POST['vdev'] = array(); }
	if (!isset($_POST['pool'])) { $_POST['pool'] = array(); }
	foreach ($_POST['vol'] as $vol)
	{
		$import |= true;
		$cfg['zfs']['volumes']['volume'][] = $zfs['volumes']['volume'][$vol];
		if (!in_array($zfs['volumes']['volume'][$vol]['pool'], $_POST['pool']))
		{
			$_POST['pool'][] = $zfs['volumes']['volume'][$vol]['pool'];
		}
	}
	foreach ($_POST['dset'] as $dset)
	{
		$import |= true;
		$cfg['zfs']['datasets']['dataset'][] = $zfs['datasets']['dataset'][$dset];
		if (!in_array($zfs['datasets']['dataset'][$dset]['pool'], $_POST['pool']))
		{
			$_POST['pool'][] = $zfs['datasets']['dataset'][$dset]['pool'];
		}
	}
	foreach ($_POST['pool'] as $pool)
	{
		$import |= true;
		$cfg['zfs']['pools']['pool'][] = $zfs['pools']['pool'][$pool];
		foreach ($zfs['pools']['pool'][$pool]['vdevice'] as $vdev)
		{
			if (!in_array($vdev, $_POST['vdev']))
			{
				$_POST['vdev'][] = $vdev;
			}
		}
	}
	foreach ($_POST['vdev'] as $vdev)
	{
		$import |= true;
		$cfg['zfs']['vdevices']['vdevice'][] = $zfs['vdevices']['vdevice'][$vdev];
	}
	
	if ($import)
	{
		$cfg['disks'] = $config['disks'];
		$cfg['geli'] = $config['geli'];
		$disks = get_physical_disks_list();
		foreach ($cfg['zfs']['vdevices']['vdevice'] as $vdev)
		{
			foreach ($vdev['device'] as $device)
			{
				$encrypted = false;
				$device = disks_label_to_device($device);
				if (preg_match("/^(.+)\.eli$/", $device, $m)) {
					$device = $m[1];
					$encrypted = true;
				}
				if (preg_match("/^(.*)p\d+$/", $device, $m)) {
					$device = $m[1];
				}
				$index = array_search_ex($device, $cfg['disks']['disk'], 'devicespecialfile');
				if ($index === false && isset($_POST['import_disks']))
				{
					$disk = array_search_ex($device, $disks, 'devicespecialfile');
					$disk = $disks[$disk];
					$cfg['disks']['disk'][] = array(
						'uuid' => uuid(),
						'name' => $disk['name'],
						'devicespecialfile' => $disk['devicespecialfile'],
						'harddiskstandby' => 0,
						'acoustic' => 0,
						'fstype' => $encrypted ? 'geli' : 'zfs',
						'apm' => 0,
						'transfermode' => 'auto',
						'type' => $disk['type'],
						'desc' => $disk['desc'],
						'size' => $disk['size'],
						'serial' => $disk['serial'],
						'smart' => false,
					);
				}
				else if ($index !== false && isset($_POST['import_disks_overwrite']))
				{
					if ($encrypted) {
						$cfg['disks']['disk'][$index]['fstype'] = 'geli';
					} else {
						$cfg['disks']['disk'][$index]['fstype'] = 'zfs';
					}
				}
				if ($encrypted) {
					$index = array_search_ex($device, $cfg['geli']['vdisk'], 'device');
					$geli_info = get_geli_info($device);
					if ($index === false && !empty($geli_info) && isset($_POST['import_disks'])) {
						$disk = array_search_ex($device, $disks, 'devicespecialfile');
						$disk = $disks[$disk];
						$cfg['geli']['vdisk'][] = array(
							'uuid' => uuid(),
							'name' => $disk['name'],
							'device' => $disk['devicespecialfile'],
							'devicespecialfile' => $disk['devicespecialfile'].".eli",
							'desc' => "Encrypted disk",
							'size' => $disk['size'],
							'aalgo' => "none",
							'ealgo' => $geli_info['ealgo'],
							'fstype' => 'zfs',
						);
					} else if ($index !== false && isset($_POST['import_disks_overwrite'])) {
						$cfg['geli']['vdisk'][$index]['fstype'] = 'zfs';
					}
				}
			}
		}
		
		$pconfig['zfs']['autosnapshots'] = $_GET['zfs']['autosnapshots'];
		if (isset($_POST['leave_autosnapshots'])) {
			$cfg['zfs']['autosnapshots'] = $config['zfs']['autosnapshots'];
		}
		$config['zfs'] = $cfg['zfs'];
		$config['disks'] = $cfg['disks'];
		$config['geli'] = $cfg['geli'];
		updatenotify_set('zfs_import_config', UPDATENOTIFY_MODE_UNKNOWN, true);
		write_config();
		header('Location: disks_zfs_config_current.php');
		exit();
	}
}

$health = true;
$health &= (bool)!array_search_ex('DEGRADED', $zfs['extra']['pools']['pool'], 'health');
$health &= (bool)!array_search_ex('FAULTED', $zfs['extra']['pools']['pool'], 'health');

if (!$health)
{
	$message_box_type = 'warning';
	$message_box_text = gettext('Your ZFS system is not healthy.');
	$message_box_text .= ' ';
	$message_box_text .= gettext('It is not recommanded to import non healty pools nor virtual devices that are part of a non healthy pool.');
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
				<li class="tabinact" title="<?=gettext("Reload page");?>"><a href="disks_zfs_config.php"><span><?=gettext("Detected");?></span></a></li>
				<li class="tabact"><a href="disks_zfs_config_sync.php"><span><?=gettext("Synchronize");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<?php if (isset($message_box_text)) print_core_box($message_box_type, $message_box_text);?>
			<?php if (isset($import) && $import === false): ?>
			<?php print_error_box(gettext('Nothing to synchronize'));?>
			<?php endif; ?>
			<form action="<?= $_SERVER['PHP_SELF']; ?>" method="post">
				<table width="100%" border="0" cellpadding="0" cellspacing="0">
					<?php html_titleline(gettext('Pools').' ('.count($zfs['pools']['pool']).')', 9);?>
					<tr>
						<td width="1%" class="listhdrlr">&nbsp;</td>
						<td width="15%" class="listhdrr"><?=gettext("Name");?></td>
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
						<td class="listlr"><input type="checkbox" checked="checked" name="pool[]" value="<?= $pool['name']; ?>" id="pool_<?= $pool['uuid']; ?>" /></td>
						<td class="listr"><label for="pool_<?= $pool['uuid']; ?>"><?= $pool['name']; ?></label></td>
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
					<?php html_titleline(gettext('Virtual devices').' ('.count($zfs['vdevices']['vdevice']).')', 5);?>
					<tr>
						<td width="1%" class="listhdrlr">&nbsp;</td>
						<td width="15%" class="listhdrr"><?=gettext("Name");?></td>
						<td width="21%" class="listhdrr"><?=gettext("Type");?></td>
						<td width="21%" class="listhdrr"><?=gettext("Pool");?></td>
						<td width="42%" class="listhdrr"><?=gettext("Devices");?></td>
					</tr>
					<?php foreach ($zfs['vdevices']['vdevice'] as $key => $vdevice):?>
					<tr>
						<td class="listlr"><input type="checkbox" checked="checked" name="vdev[]" value="<?= $vdevice['name']; ?>" id="vdev_<?= $vdevice['uuid']; ?>" /></td>
						<td class="listr"><label for="vdev_<?= $vdevice['uuid']; ?>"><?= $vdevice['name']; ?></label></td>
						<td class="listr"><?= $vdevice['type']; ?></td>
						<td class="listr"><?= $zfs['extra']['vdevices']['vdevice'][$key]['pool']; ?></td>
						<td class="listr"><?= implode(', ', $vdevice['device']); ?></td>
					</tr>
					<?php endforeach; ?>
				</table>
				<br />
				<table width="100%" border="0" cellpadding="0" cellspacing="0">
					<?php html_titleline(gettext('Datasets').' ('.count($zfs['datasets']['dataset']).')', 10);?>
					<tr>
						<td width="1%" class="listhdrlr">&nbsp;</td>
						<td width="15%" class="listhdrr"><?=gettext("Name");?></td>
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
						<td class="listlr"><input type="checkbox" checked="checked" name="dset[]" value="<?= $dataset['name']; ?>" id="ds_<?= $dataset['uuid']; ?>" /></td>
						<td class="listr"><label for="ds_<?= $dataset['uuid']; ?>"><?= $dataset['name']; ?></label></td>
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
				<?php html_titleline(gettext('Volumes').' ('.count($zfs['volumes']['volume']).')', 6);?>
				<tr>
						<td width="1%" class="listhdrlr">&nbsp;</td>
					<td width="15%" class="listhdrr"><?=gettext("Name");?></td>
					<td width="21%" class="listhdrr"><?=gettext("Pool");?></td>
					<td width="21%" class="listhdrr"><?=gettext("Size");?></td>
					<td width="21%" class="listhdrr"><?=gettext("Compression");?></td>
					<td width="21%" class="listhdrr"><?=gettext("Dedup");?></td>
				</tr>
				<?php foreach ($zfs['volumes']['volume'] as $volume):?>
				<tr>
						<td class="listlr"><input type="checkbox" checked="checked" name="vol[]" value="<?= $volume['name']; ?>" id="vol_<?= $dataset['uuid']; ?>" /></td>

					<td class="listr"><?= $volume['name']; ?></td>
					<td class="listr"><?= $volume['pool']; ?></td>
					<td class="listr"><?= $volume['volsize']; ?></td>
					<td class="listr"><?= $volume['compression']; ?></td>
					<td class="listr"><?= $volume['dedup']; ?></td>
				</tr>
				<?php endforeach;?>
			</table>
				<br />







				<table width="100%" border="0" cellpadding="5" cellspacing="0">
					<?php html_titleline(gettext('Options'));?>
					<?php html_checkbox("leave_autosnapshots", gettext("Leave auto snapshot configuration"), true, gettext("Leave already configured auto snapshots."), "", false);?>
					<?php html_checkbox("import_disks", gettext("Import disks"), true, gettext("Import disks used in configuration."), "", false);?>
					<?php html_checkbox("import_disks_overwrite", gettext("Overwrite disks configuration"), false, gettext("Overwrite already configured disks (only affects filesystem value)."), "", false);?>
				</table>
				<br />
				<div id="submit">
					<input type="submit" name="import_config" value="<?= gettext('Synchronize'); ?>" />
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
