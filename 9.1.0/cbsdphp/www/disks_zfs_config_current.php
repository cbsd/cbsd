#!/usr/local/bin/php
<?php
/*
	disks_zfs_config_current.php
	
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

$pgtitle = array(gettext("Disks"), gettext("ZFS"), gettext("Configuration"), gettext("Current"));

$zfs = $config['zfs'];

if (!is_array($zfs['pools'])) $zfs['pools']['pool'] = array();
if (!is_array($zfs['vdevices'])) $zfs['vdevices']['vdevice'] = array();
if (!is_array($zfs['datasets'])) $zfs['datasets']['dataset'] = array();
if (!is_array($zfs['volumes'])) $zfs['volumes']['volume'] = array();

foreach ($zfs['pools']['pool'] as $index => $pool)
{
	$zfs['pools']['pool'][$index]['size']   = gettext('Unknown');
	$zfs['pools']['pool'][$index]['used']   = gettext('Unknown');
	$zfs['pools']['pool'][$index]['avail']  = gettext('Unknown');
	$zfs['pools']['pool'][$index]['cap']    = gettext('Unknown');
	$zfs['pools']['pool'][$index]['health'] = gettext('Unknown');

	foreach ($pool['vdevice'] as $vdevice)
	{
		if (false === ($index = array_search_ex($vdevice, $zfs['vdevices']['vdevice'], 'name'))) { continue; }
		$zfs['vdevices']['vdevice'][$index]['pool'] = $pool['name'];
	}
}

$rawdata = null;
mwexec2("zfs list -H -t filesystem -o name,used,available", $rawdata);
foreach($rawdata as $line)
{
	if ($line == 'no datasets available') { continue; }
	list($fname, $used, $avail) = explode("\t", $line);
	if (false === ($index = array_search_ex($fname, $zfs['pools']['pool'], 'name'))) { continue; }
	if (strpos($fname, '/') === false) // zpool
	{
		$zfs['pools']['pool'][$index]['used'] = $used;
		$zfs['pools']['pool'][$index]['avail'] = $avail;
	}
}

$rawdata = null;
$spa = @exec("sysctl -q -n vfs.zfs.version.spa");
if ($spa == '') {
	mwexec2("zpool list -H -o name,root,size,capacity,health", $rawdata);
} else if ($spa < 21) {
	mwexec2("zpool list -H -o name,altroot,size,capacity,health", $rawdata);
} else {
	mwexec2("zpool list -H -o name,altroot,size,capacity,health,dedup", $rawdata);
}
foreach ($rawdata as $line)
{
	if ($line == 'no pools available') { continue; }
	list($pool, $root, $size, $cap, $health, $dedup) = explode("\t", $line);
	if (false === ($index = array_search_ex($pool, $zfs['pools']['pool'], 'name'))) { continue; }
	if ($root != '-')
	{
		$zfs['pools']['pool'][$index]['root'] = $root;
	}
	$zfs['pools']['pool'][$index]['size'] = $size;
	$zfs['pools']['pool'][$index]['cap'] = $cap;
	$zfs['pools']['pool'][$index]['health'] = $health;
	$zfs['pools']['pool'][$index]['dedup'] = $dedup;
}

if (updatenotify_exists('zfs_import_config'))
{
	$notifications = updatenotify_get('zfs_import_config');
	$retval = 0;
	foreach ($notifications as $notification)
	{
		$retval |= !($notification['data'] == true);
	}
	$savemsg = get_std_save_message($retval);
	if ($retval == 0)
	{
		updatenotify_delete("zfs_import_config");
	}
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
			<li class="tabact"><a href="disks_zfs_config_current.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Current");?></span></a></li>
			<li class="tabinact"><a href="disks_zfs_config.php"><span><?=gettext("Detected");?></span></a></li>
			<li class="tabinact"><a href="disks_zfs_config_sync.php"><span><?=gettext("Synchronize");?></span></a></li>
		</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<?php if (isset($savemsg)) print_info_box($savemsg); ?>
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
				<?php foreach ($zfs['pools']['pool'] as $pool):?>
				<tr>
					<td class="listlr"><?= $pool['name']; ?></td>
					<td class="listr"><?= $pool['size']; ?></td>
					<td class="listr"><?= $pool['used']; ?> (<?= $pool['cap']; ?>)</td>
					<td class="listr"><?= $pool['avail']; ?></td>
					<td class="listr"><?= $pool['dedup']; ?></td>
					<td class="listr"><?= $pool['health']; ?></td>
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
				<?php foreach ($zfs['vdevices']['vdevice'] as $vdevice):?>
				<tr>
					<td class="listlr"><?= $vdevice['name']; ?></td>
					<td class="listr"><?= $vdevice['type']; ?></td>
					<td class="listr"><?= $vdevice['pool']; ?></td>
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
					<td class="listr"><?= $dataset['pool'][0]; ?></td>
					<td class="listr"><?= $dataset['compression']; ?></td>
					<td class="listr"><?= $dataset['dedup']; ?></td>
					<td class="listr"><?= isset($dataset['canmount']) ? 'on' : 'off'; ?></td>
					<td class="listr"><?= empty($dataset['quota']) ? 'none' : $dataset['quota']; ?></td>
					<td class="listr"><?= isset($dataset['xattr']) ? 'on' : 'off'; ?></td>
					<td class="listr"><?= isset($dataset['readonly']) ? 'on' : 'off'; ?></td>
					<td class="listr"><?= isset($dataset['snapdir']) ? 'visible' : 'hidden'; ?></td>
				</tr>
				<?php endforeach;?>
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
					<td class="listr"><?= $volume['pool'][0]; ?></td>
					<td class="listr"><?= $volume['volsize']; ?></td>
					<td class="listr"><?= $volume['compression']; ?></td>
					<td class="listr"><?= $volume['dedup']; ?></td>
				</tr>
				<?php endforeach;?>
			</table>
			<div id="remarks">
				<?php html_remark("note", gettext("Note"), gettext("This page reflects the configuration that has been created with the WebGUI."));?>
			</div>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
