#!/usr/local/bin/php
<?php
/*
	disks_zfs_zpool_info.php

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
require("sajax/sajax.php");

$pgtitle = array(gettext("Disks"), gettext("ZFS"), gettext("Pools"), gettext("Information"));

if (!isset($config['zfs']['pools']['pool']) || !is_array($config['zfs']['pools']['pool']))
	$config['zfs']['pools']['pool'] = array();

if (!isset($config['zfs']['vdevices']['vdevice']) || !is_array($config['zfs']['vdevices']['vdevice']))
	$config['zfs']['vdevices']['vdevice'] = array();

function zfs_zpool_get_status() {
	global $config;

	array_sort_key($config['zfs']['pools']['pool'], "name");
	array_sort_key($config['zfs']['vdevices']['vdevice'], "name");

	$a_pool = $config['zfs']['pools']['pool'];
	$a_vdevice = $config['zfs']['vdevices']['vdevice'];

	// Get zpool status informations
	$cmd = "zpool status -v";
	if (isset($_GET['pool'])) {
		$cmd .= " {$_GET['pool']}";
	}
	mwexec2($cmd, $rawdata);

	// Modify and render status informations
	$result = "";
	foreach ($rawdata as $line) {
		if (preg_match("/(\s+)(?:pool\:)(\s+)(.*)/", $line, $match)) {
			$pool = trim($match[3]);
			$index = array_search_ex($pool, $a_pool, "name");
			if (0 && $index !== false) {
				$href = "<a href='disks_zfs_zpool_edit.php?uuid={$a_pool[$index]['uuid']}'>{$pool}</a>";
				$result .= "{$match[1]}pool:{$match[2]}{$href}";
			} else {
				$result .= htmlspecialchars($line);
			}
		} else if (preg_match("/(\s+)(?:scrub\:)(\s+)(.*)/", $line, $match)) {
			if (0 && isset($pool)) {
				$href = "<a href='disks_zfs_zpool_tools.php?action=scrub&option=s&pool={$pool}' title=\"".sprintf(gettext("Start scrub on '%s'."), $pool)."\">scrub</a>:";
			} else {
				$href = "scrub";
			}
			$result .= "{$match[1]}{$href}{$match[2]}{$match[3]}";
		} else {
			if (0 && isset($pool)) {
				$a_disk = get_conf_disks_filtered_ex("fstype", "zfs");
				$found = false;
				if (count($a_disk) > 0 && false !== ($index = array_search_ex($pool, $a_pool, "name"))) {
					$pool_conf = $a_pool[$index];
					if (is_array($pool_conf['vdevice'])) {
						foreach ($pool_conf['vdevice'] as $vdevicev) {
							if (false !== ($index = array_search_ex($vdevicev, $a_vdevice, "name"))) {
								$vdevice = $a_vdevice[$index];
								if (is_array($vdevice['device'])) {
									foreach ($vdevice['device'] as $devicev) {
										$index = array_search_ex($devicev, $a_disk, "devicespecialfile");
										if ($index === false) continue 2;
										$disk = $a_disk[$index];
										$string = "/(\s+)(?:".$disk['name'].")(\s+)(\w+)(.*)/";
										if (preg_match($string, $line, $match)) {
											$href = "<a href='disks_zfs_zpool_tools.php'>{$disk['name']}</a>";
											if (0 && $match[3] == "ONLINE") {
												$href1 = "<a href='disks_zfs_zpool_tools.php?action=offline&option=d&pool={$pool}&device={$disk[name]}'>{$match[3]}</a>";
											} else if(0 && $match[3] == "OFFLINE") {
												$href1 = "<a href='disks_zfs_zpool_tools.php?action=online&option=d&pool={$pool}&device={$disk[name]}'>{$match[3]}</a>";
											} else {
												$href1 = "";
											}
											$result .= "{$match[1]}{$href}{$match[2]}{$href1}{$match[4]}";
											$found = true;
											continue 2;
										}
									}
								}
							}
						}
					}
				}
				if (!$found) {
					$result .= htmlspecialchars($line);
				}
			} else {
				$result .= htmlspecialchars($line);
			}
		}
		$result .= "<br />";
	}
	return $result;
}

sajax_init();
sajax_export("zfs_zpool_get_status");
sajax_handle_client_request();
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">//<![CDATA[
<?php sajax_show_javascript();?>
//]]>
</script>
<script type="text/javascript" src="javascript/disks_zfs_zpool_info.js"></script>
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
				<li class="tabinact"><a href="disks_zfs_zpool.php"><span><?=gettext("Management");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_zpool_tools.php"><span><?=gettext("Tools");?></span></a></li>
				<li class="tabact"><a href="disks_zfs_zpool_info.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Information");?></span></a></li>
				<li class="tabinact"><a href="disks_zfs_zpool_io.php"><span><?=gettext("I/O statistics");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<table width="100%" border="0" cellspacing="0" cellpadding="0">
				<?php html_titleline(gettext("Pool information and status"));?>
				<tr>
					<td class="listt">
						<pre><span id="zfs_zpool_status"><?=zfs_zpool_get_status();?></span></pre>
					</td>
				</tr>
			</table>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
