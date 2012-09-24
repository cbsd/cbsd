#!/usr/local/bin/php
<?php
/*
	disks_zfs_zpool_tools.php

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

$pgtitle = array(gettext("Disks"), gettext("ZFS"), gettext("Pools"), gettext("Tools"));

if (!isset($config['zfs']['pools']['pool']) || !is_array($config['zfs']['pools']['pool']))
	$config['zfs']['pools']['pool'] = array();

if (!isset($config['zfs']['vdevices']['vdevice']) || !is_array($config['zfs']['vdevices']['vdevice']))
	$config['zfs']['vdevices']['vdevice'] = array();

array_sort_key($config['zfs']['pools']['pool'], "name");
array_sort_key($config['zfs']['vdevices']['vdevice'], "name");

$a_pool = $config['zfs']['pools']['pool'];
$a_vdevice = $config['zfs']['vdevices']['vdevice'];
$a_vdevice_cache = array();
$a_vdevice_spare = array();
$a_vdevice_vdev = array();
foreach ($a_vdevice as $vdevicev) {
	if ($vdevicev['type'] == 'cache') {
		$tmp = $vdevicev;
		$a_devs = array();
		foreach ($vdevicev['device'] as $device) {
			$name = preg_replace("/^\/dev\//", "", $device);
			$a_devs[] = $name;
		}
		$tmp['devs'] = implode(" ", $a_devs);
		$a_vdevice_cache[] = $tmp;
	} else if ($vdevicev['type'] == 'spare') {
		$tmp = $vdevicev;
		$a_devs = array();
		foreach ($vdevicev['device'] as $device) {
			$name = preg_replace("/^\/dev\//", "", $device);
			$a_devs[] = $name;
		}
		$tmp['devs'] = implode(" ", $a_devs);
		$a_vdevice_spare[] = $tmp;
	} else if ($vdevicev['type'] == 'stripe' ||$vdevicev['type'] == 'mirror'
		   || $vdevicev['type'] == 'raidz1' || $vdevicev['type'] == 'raidz2'
		   || $vdevicev['type'] == 'raidz3') {
		$tmp = $vdevicev;
		$a_devs = array();
		foreach ($vdevicev['device'] as $device) {
			$name = preg_replace("/^\/dev\//", "", $device);
			$a_devs[] = $name;
		}
		$tmp['devs'] = implode(" ", $a_devs);
		$a_vdevice_vdev[] = $tmp;
	}
}

$a_disk = get_conf_disks_filtered_ex("fstype", "zfs");
$a_disk_free = array();
foreach ($a_disk as $diskv) {
	if (false !== array_search_ex($diskv['devicespecialfile'], $a_vdevice, "device"))
		continue;
	$a_disk_free = array_merge($a_disk_free, array("{$diskv['devicespecialfile']}" => "{$diskv['name']} ({$diskv['desc']})"));
	//$a_disk_free = array_merge($a_disk_free, array("{$diskv['name']}" => "{$diskv['name']} ({$diskv['desc']})"));
}

function get_spare_list($pool) {
	$result = array();
	mwexec2("zpool status {$pool}", $rawdata);
	$req_level = -1;
	$key = "";
	$devs = array();
	foreach ($rawdata as $line) {
		if ($line[0] != "\t") continue;
		if (preg_match('/^\t(\s+)(spare-\S+)/', $line, $m)) {
			$req_level = strlen($m[1]) + 2;
			$key = $m[2];
			continue;
		}
		if (preg_match('/^\t(\s+)(\S+)/', $line, $m)) {
			$level = strlen($m[1]);
			if ($level == $req_level) {
				$devs[] = "/dev/{$m[2]}";
				continue;
			}
		}
		if ($key != "")
			$result[$key] = $devs;
		$key = "";
		$devs = array();
		$req_level = -1;
	}
	return $result;
}

function get_device_type($device, &$a_vdevice) {
	$index = array_search_ex($device, $a_vdevice, "device");
	if ($index !== false) {
		$type = $a_vdevice[$index]['type'];
	} else {
		$type = "none";
	}
	return $type;
}

$pconfig['action'] = $_GET['action'];
if (isset($_POST['action']))
	$pconfig['action'] = $_POST['action'];

$pconfig['option'] = $_GET['option'];
if (isset($_POST['option']))
	$pconfig['option'] = $_POST['option'];

$pconfig['pool'] = $_GET['pool'];
if (isset($_POST['pool']))
	$pconfig['pool'] = $_POST['pool'];

$pconfig['device'] = $_GET['device'];
if (isset($_POST['device']))
	$pconfig['device'] = $_POST['device'];

$pconfig['device_new'] = $_GET['device_new'];
if (isset($_POST['device_new']))
	$pconfig['device_new'] = $_POST['device_new'];

$pconfig['device_new2'] = $_GET['device_new2'];
if (isset($_POST['device_new2']))
	$pconfig['device_new2'] = $_POST['device_new2'];

$pconfig['device_cache'] = $_GET['device_cache'];
if (isset($_POST['device_cache']))
	$pconfig['device_cache'] = $_POST['device_cache'];

$pconfig['device_spare'] = $_GET['device_spare'];
if (isset($_POST['device_spare']))
	$pconfig['device_spare'] = $_POST['device_spare'];

$pconfig['device_vdev'] = $_GET['device_vdev'];
if (isset($_POST['device_vdev']))
	$pconfig['device_vdev'] = $_POST['device_vdev'];

if ($_POST || $_GET) {
	unset($input_errors);
	unset($do_action);

	if (!$input_errors) {
		$do_action = true;
	}
}

if (!isset($do_action)) {
	$do_action = false;
	$pconfig['action'] = "history";
	$pconfig['option'] = "";
	$pconfig['pool'] = "";
	$pconfig['device'] = "";
	$pconfig['device_new'] = "";
	$pconfig['device_cache'] = "";
	$pconfig['device_spare'] = "";
	$pconfig['device_vdev'] = "";
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">//<![CDATA[
function command_change() {
	showElementById('devices_tr','hide');
	showElementById('device_new_tr','hide');
	showElementById('device_new2_tr','hide');
	showElementById('device_cache_tr','hide');
	showElementById('device_spare_tr','hide');
	showElementById('device_vdev_tr','hide');
	document.iform.option.length = 0;
	document.iform.device_new.length = 0;
	document.iform.device_cache.length = 0;
	document.iform.device_spare.length = 0;
	document.iform.device_vdev.length = 0;
	var action = document.iform.action.value;
	switch (action) {
		case "upgrade":
			document.iform.option[0] = new Option('<?=gettext("Display")?>','v', <?=$pconfig['option'] === 'v' ? "true" : "false"?>);
			document.iform.option[1] = new Option('<?=gettext("All")?>','a', <?=$pconfig['option'] === 'a' ? "true" : "false"?>);
			<?php if (is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[2] = new Option('<?=gettext("Pool")?>','p', <?=$pconfig['option'] === 'p' ? "true" : "false"?>);
			<?php endif;?>
			break;
		case "history":
			document.iform.option[0] = new Option('<?=gettext("All")?>','a', <?=$pconfig['option'] === 'a' ? "true" : "false"?>);
			<?php if (is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[1] = new Option('<?=gettext("Pool")?>','p', <?=$pconfig['option'] === 'p' ? "true" : "false"?>);
			<?php endif;?>
			break;
		case "scrub":
			document.iform.option[0] = new Option('<?=gettext("Start")?>','s', <?=$pconfig['option'] === 's' ? "true" : "false"?>);
			document.iform.option[1] = new Option('<?=gettext("Stop")?>','st', <?=$pconfig['option'] === 'st' ? "true" : "false"?>);
			break;
		case "clear":
			showElementById('devices_tr','show');
			<?php if (is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[0] = new Option('<?=gettext("Pool")?>','p', <?=$pconfig['option'] === 'p' ? "true" : "false"?>);
			document.iform.option[1] = new Option('<?=gettext("Device")?>','d', <?=$pconfig['option'] === 'd' ? "true" : "false"?>);
			<?php endif;?>
			break;
		case "offline":
			showElementById('devices_tr','show');
			<?php if (is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[0] = new Option('<?=gettext("Device")?>','d', <?=$pconfig['option'] === 'd' ? "true" : "false"?>);
			document.iform.option[1] = new Option('<?=gettext("Temporary Device")?>','t', <?=$pconfig['option'] === 't' ? "true" : "false"?>);
			<?php endif;?>
			break;
		case "online":
			showElementById('devices_tr','show');
			<?php if(is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[0] = new Option('<?=gettext("Device")?>','d', <?=$pconfig['option'] === 'd' ? "true" : "false"?>);
			<?php endif;?>
			break;
		case "attach":
			showElementById('devices_tr','show');
			showElementById('device_new2_tr','show');
			<?php if(is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[0] = new Option('<?=gettext("Device")?>','d', <?=$pconfig['option'] === 'd' ? "true" : "false"?>);
			<?php endif;?>
			break;
		case "detach":
			showElementById('devices_tr','show');
			<?php if(is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[0] = new Option('<?=gettext("Device")?>','d', <?=$pconfig['option'] === 'd' ? "true" : "false"?>);
			<?php endif;?>
			break;
		case "remove":
			showElementById('devices_tr','show');
			<?php if (is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[0] = new Option('<?=gettext("Device")?>','d', <?=$pconfig['option'] === 'd' ? "true" : "false"?>);
			<?php endif;?>
			break;
		case "replace":
			showElementById('devices_tr','show');
			showElementById('device_new_tr','show');
			<?php if (is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[0] = new Option('<?=gettext("Device")?>','d', <?=$pconfig['option'] === 'd' ? "true" : "false"?>);
			<?php endif;?>
			break;
		case "cache add":
			showElementById('devices_tr','hide');
			showElementById('device_cache_tr','show');
			<?php if (is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[0] = new Option('<?=gettext("Device")?>','d', <?=$pconfig['option'] === 'd' ? "true" : "false"?>);
			<?php endif;?>
			break;
		case "cache remove":
			showElementById('devices_tr','hide');
			showElementById('device_cache_tr','show');
			<?php if (is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[0] = new Option('<?=gettext("Device")?>','d', <?=$pconfig['option'] === 'd' ? "true" : "false"?>);
			<?php endif;?>
			break;
		case "spare add":
			showElementById('devices_tr','hide');
			showElementById('device_spare_tr','show');
			<?php if (is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[0] = new Option('<?=gettext("Device")?>','d', <?=$pconfig['option'] === 'd' ? "true" : "false"?>);
			<?php endif;?>
			break;
		case "spare remove":
			showElementById('devices_tr','hide');
			showElementById('device_spare_tr','show');
			<?php if (is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[0] = new Option('<?=gettext("Device")?>','d', <?=$pconfig['option'] === 'd' ? "true" : "false"?>);
			<?php endif;?>
			break;
		case "vdev add":
			showElementById('devices_tr','hide');
			showElementById('device_vdev_tr','show');
			<?php if (is_array($a_pool) && !empty($a_pool)):?>
			document.iform.option[0] = new Option('<?=gettext("Device")?>','d', <?=$pconfig['option'] === 'd' ? "true" : "false"?>);
			<?php endif;?>
			break;
		default:
			break;
	}
	option_change();
}

function option_change() {
	var div = document.getElementById("devices");
	div.innerHTML = "<?=gettext("No device selected.");?>";

	document.iform.pool.disabled = 1;
	document.iform.pool.length = 0;
	var option = document.iform.option.value;
	if (option == "s" || option == "st" || option == "p" || option == "d" || option == "t") {
		<?php if (is_array($a_pool) && !empty($a_pool)):?>
		document.iform.pool.disabled = 0;
		<?php $i = 0; foreach($a_pool  as $pool):?>
		document.iform.pool[<?=$i?>] = new Option('<?=$pool['name']?>','<?=$pool['name']?>', <?=$pconfig['pool'] === $pool['name'] ? "true" : "false"?>);
		<?php if ($pconfig['pool'] === $pool['name']) {?>
			document.iform.pool.selectedIndex = <?=$i?>;
		<?php }?>
		<?php $i++; endforeach;?>
		<?php endif;?>
	}
	if (option == "d" || option == "t") {
		pool_change();
	}
}

function pool_change() {
	document.iform.device_new.length = 0;
	document.iform.device_cache.length = 0;
	document.iform.device_spare.length = 0;
	document.iform.device_vdev.length = 0;
	var div = document.getElementById("devices");
	div.innerHTML ="";
	var pool = document.iform.pool.value;
	var action = document.iform.action.value;
	switch (pool) {
		<?php foreach ($a_pool as $pool):?>
		case "<?=$pool['name'];?>": {
			<?php
			$result = array();
			$first_type = "";
			foreach ($pool['vdevice'] as $vdevicev) {
				$index = array_search_ex($vdevicev, $a_vdevice, "name");
				$vdevice = $a_vdevice[$index];
				$type = $vdevice['type'];
				if ($first_type == ""
				    && ($type == "stripe" || $type == "mirror"
					|| $type == "raidz1" || $type == "raidz2"
					|| $type == "raidz3")) {
					$first_type = $type;
				}
				foreach ($vdevice['device'] as $devicev) {
					$a_disk = get_conf_disks_filtered_ex("fstype", "zfs");
					$a_encrypteddisk = get_conf_encryped_disks_list();

					if (($index = array_search_ex($devicev, $a_disk, "devicespecialfile")) !== false) {
						$tmp = $a_disk[$index];
						$tmp['type'] = $type;
						$tmp['name2'] = $tmp['name'];
						if (($index = array_search_ex($devicev, $a_encrypteddisk, "devicespecialfile")) !== false) {
							$tmp['name2'] = $tmp['name'].".eli";
						}
						$result[] = $tmp;
					}
				}
			}
			$i = 0; $j = 0;
			array_sort_key($result, "name");
			foreach ($result as $disk) {
				$checked = "";
				if (is_array($pconfig['device'])) {
					foreach ($pconfig['device'] as $devicev) {
						if ($devicev === $disk['name']) {
							$checked = " checked='checked'";
							break;
						}
					}
				} else {
					if ($pconfig['device'] === $disk['name']) {
						$checked = " checked='checked'";
					}
				}
				if ($disk['type'] != "cache") {
				?>

				if (action != "cache add" && action != "cache remove") {
					div.innerHTML += "<input name='device[]' id='<?=$i?>' type='checkbox' value='<?=$disk['name2'];?>'<?=$checked?> />";
					div.innerHTML += "<?=$disk['name'];?> (<?=$disk['size']?>, <?=htmlspecialchars($disk['desc'])?>)";
					div.innerHTML += "<br />";
					document.iform.device_new[<?=$i;?>] = new Option('<?="{$disk['name']} ({$disk['size']}, {$disk['desc']})";?>','<?=$disk['name2'];?>','false');
				}

				<?php
					$i++;
				} else if ($disk['type'] == "cache") {
				?>

				if (action == "cache add" || action == "cache remove") {
					div.innerHTML += "<input name='device[]' id='<?=$i?>' type='checkbox' value='<?=$disk['name2'];?>'<?=$checked?> />";
					div.innerHTML += "<?=$disk['name'];?> (<?=$disk['type']?>, <?=$disk['size']?>, <?=htmlspecialchars($disk['desc'])?>)";
					div.innerHTML += "<br />";
					document.iform.device_new[<?=$j;?>] = new Option('<?="{$disk['name']} ({$disk['type']}, {$disk['size']}, {$disk['desc']})";?>','<?=$disk['name2'];?>','false');
				}

				<?php
					$j++;
				}
			}

			$result_add = array();
			$result_del = array();
			array_sort_key($a_vdevice_cache, "name");
			foreach ($a_vdevice_cache as $vdevicev) {
				$index = array_search_ex($vdevicev['name'], $a_pool, "vdevice");
				if ($index !== false) {
					if ($a_pool[$index]['name'] == $pool['name']) {
						$result_del[] = $vdevicev;
					}
				} else {
					$result_add[] = $vdevicev;
				}
			}
			?>

			if (action == "cache add") {
			<?php $i = 0; foreach ($result_add as $vdevicev) {?>
				document.iform.device_cache[<?=$i;?>] = new Option('<?="{$vdevicev['name']} ({$vdevicev['devs']})";?>','<?="{$vdevicev['name']}";?>','false');
			<?php $i++; } ?>

			} else if (action == "cache remove") {
			<?php $i = 0; foreach ($result_del as $vdevicev) {?>
				document.iform.device_cache[<?=$i;?>] = new Option('<?="{$vdevicev['name']} ({$vdevicev['devs']})";?>','<?="{$vdevicev['name']}";?>','false');
			<?php $i++; } ?>

			}

			<?php
			$result_add = array();
			$result_del = array();
			array_sort_key($a_vdevice_spare, "name");
			foreach ($a_vdevice_spare as $vdevicev) {
				$index = array_search_ex($vdevicev['name'], $a_pool, "vdevice");
				if ($index !== false) {
					if ($a_pool[$index]['name'] == $pool['name']) {
						$result_del[] = $vdevicev;
					}
				} else {
					$result_add[] = $vdevicev;
				}
			}
			?>

			if (action == "spare add") {
			<?php $i = 0; foreach ($result_add as $vdevicev) {?>
				document.iform.device_spare[<?=$i;?>] = new Option('<?="{$vdevicev['name']} ({$vdevicev['devs']})";?>','<?="{$vdevicev['name']}";?>','false');
			<?php $i++; } ?>

			} else if (action == "spare remove") {
			<?php $i = 0; foreach ($result_del as $vdevicev) {?>
				document.iform.device_spare[<?=$i;?>] = new Option('<?="{$vdevicev['name']} ({$vdevicev['devs']})";?>','<?="{$vdevicev['name']}";?>','false');
			<?php $i++; } ?>

			}

			<?php
			$result_add = array();
			$result_del = array();
			array_sort_key($a_vdevice_vdev, "name");
			foreach ($a_vdevice_vdev as $vdevicev) {
				if ($first_type == "" || $first_type != $vdevicev['type']) {
					continue;
				}
				$index = array_search_ex($vdevicev['name'], $a_pool, "vdevice");
				if ($index !== false) {
					if ($a_pool[$index]['name'] == $pool['name']) {
						$result_del[] = $vdevicev;
					}
				} else {
					$result_add[] = $vdevicev;
				}
			}
			?>

			if (action == "vdev add") {
			<?php $i = 0; foreach ($result_add as $vdevicev) {?>
				document.iform.device_vdev[<?=$i;?>] = new Option('<?="{$vdevicev['name']} ({$vdevicev['devs']})";?>','<?="{$vdevicev['name']}";?>','false');
			<?php $i++; } ?>

			} else if (action == "vdev remove") {
			<?php $i = 0; foreach ($result_del as $vdevicev) {?>
				document.iform.device_vdev[<?=$i;?>] = new Option('<?="{$vdevicev['name']} ({$vdevicev['devs']})";?>','<?="{$vdevicev['name']}";?>','false');
			<?php $i++; } ?>

			}
		}
		break;
		<?php endforeach;?>
	}
}
//]]>
</script>
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
	<li class="tabact"><a href="disks_zfs_zpool_tools.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Tools");?></span></a></li>
	<li class="tabinact"><a href="disks_zfs_zpool_info.php"><span><?=gettext("Information");?></span></a></li>
	<li class="tabinact"><a href="disks_zfs_zpool_io.php"><span><?=gettext("I/O statistics");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
      <form action="disks_zfs_zpool_tools.php" method="post" name="iform" id="iform">
	<?php if ($input_errors) print_input_errors($input_errors);?>
	<?php if (file_exists($d_sysrebootreqd_path)) print_info_box(get_std_save_message(0));?>
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
	<tr>
	  <td width="22%" valign="top" class="vncellreq"><?=gettext("Command");?></td>
	  <td width="78%" class="vtable">
	  <select name="action" class="formfld" id="action" onchange="command_change()">
	  <?
		$cmd = "upgrade history";
		if (is_array($a_pool) && !empty($a_pool)) {
		    $cmd .= " remove clear scrub offline online replace attach detach";
		}
		$a_cmd = explode(" ", $cmd);
		if (is_array($a_pool) && !empty($a_pool)) {
		    $a_cmd[] = "cache add";
		    $a_cmd[] = "cache remove";
		    $a_cmd[] = "spare add";
		    $a_cmd[] = "spare remove";
		    $a_cmd[] = "vdev add";
		}
		asort($a_cmd);
		foreach ($a_cmd as $cmdv) {
		    echo "<option value=\"${cmdv}\"";
		if ($cmdv === $pconfig['action'])
		    echo " selected=\"selected\"";
		    echo ">${cmdv}</option>";
		}
	  ?>
	  </select>
	  </td>
	</tr>
	<?php html_combobox("option", gettext("Option"), NULL, NULL, "", true, false, "option_change()");?>
	<?php html_combobox("pool", gettext("Pool"), NULL, NULL, "", true, true, "pool_change()");?>
	<tr id='devices_tr'>
	<td valign="top" class="vncellreq"><?=gettext("Devices");?></td>
	<td class="vtable">
	<div id="devices">
	<?=gettext("No device selected.");?>
	</div>
	</td>
	</tr>
	<?php html_combobox("device_new", gettext("New Device"), NULL, NULL, "", true);?>
	<?php html_combobox("device_new2", gettext("New Device"), NULL, $a_disk_free, "", true);?>
	<?php html_combobox("device_cache", gettext("Cache Device"), NULL, NULL, "", true);?>
	<?php html_combobox("device_spare", gettext("Hot Spare"), NULL, NULL, "", true);?>
	<?php html_combobox("device_vdev", gettext("Virtual device"), NULL, NULL, gettext("Once you add the virtual device, it becomes impossible to delete again. It recommends adding the same number of drives as the existing virtual device."), true);?>
	</table>
	<div id="submit">
	  <input name="Submit" type="submit" class="formbtn" value="<?=gettext("Send Command!");?>" />
	</div>
	<?php if ($do_action) {
		echo(sprintf("<div id='cmdoutput'>%s</div>", gettext("Command output:")));
		echo('<pre class="cmdoutput">');
		ob_end_flush();

		$action = $pconfig['action'];
		$option = $pconfig['option'];
		$pool = $pconfig['pool'];
		$device = $pconfig['device'];
		$new_device = $pconfig['device_new'];
		$new2_device = $pconfig['device_new2'];
		$cache_device = $pconfig['device_cache'];
		$spare_device = $pconfig['device_spare'];
		$vdev_device = $pconfig['device_vdev'];

		if (is_array($device)) {
			$a = array();
			foreach ($device as $dev) {
				$index = array_search_ex("/dev/{$dev}", $a_vdevice, "device");
				if ($index !== false) {
					$aft4k = $a_vdevice[$index]['aft4k'];
					if (isset($aft4k)) {
						$a[] = "{$dev}.nop";
					} else {
						$a[] = "{$dev}";
					}
				}
			}
			$device = $a;
		} else {
			$index = array_search_ex("/dev/{$device}", $a_vdevice, "device");
			if ($index !== false) {
				$aft4k = $a_vdevice[$index]['aft4k'];
				if (isset($aft4k)) {
					$device = "{$device}.nop";
				} else {
					$device = "{$device}";
				}
			}
		}

		$index = array_search_ex("/dev/{$new_device}", $a_vdevice, "device");
		if ($index !== false) {
			$aft4k = $a_vdevice[$index]['aft4k'];
			if (isset($aft4k)) {
				$new_device = "{$new_device}.nop";
			} else {
				$new_device = "{$new_device}";
			}
		}

		switch ($action) {
		case "upgrade": {
		    switch ($option) {
		    case "v": {
			zfs_zpool_cmd($action, "-v", false, false, true, $output);
			foreach ($output as $line) {
			    if (preg_match("/(\s+)(\d+)(\s+)(.*)/",$line, $match)) {
				$href = "<a href=\"http://www.opensolaris.org/os/community/zfs/version/{$match[2]}\" target=\"_blank\">{$match[2]}</a>";
				echo "{$match[1]}{$href}{$match[3]}{$match[4]}";
			    } else {
				echo htmlspecialchars($line);
			    }
			    echo "<br />";
			}
		    }
		    break;

		    case "a":
			zfs_zpool_cmd($action, "-a", true);
			break;

		    case "p":
			zfs_zpool_cmd($action, $pool, true);
			break;
		    }
		}
		break;

		case "history": {
		    switch ($option) {
		    case "a":
			zfs_zpool_cmd($action, "", true);
			break;

		    case "p":
			zfs_zpool_cmd($action, $pool, true);
			break;
		    }
		}
		break;

		case "scrub": {
		    switch ($option) {
		    case "s":
			zfs_zpool_cmd($action, $pool, true);
			break;

		    case "st":
			zfs_zpool_cmd($action,"-s {$pool}", true);
			break;
		    }
		}
		break;

		case "clear": {
		    switch ($option) {
		    case "p":
			zfs_zpool_cmd($action, $pool, true);
			break;

		    case "d":
			if (is_array($device)) {
			    foreach ($device as $dev) {
				zfs_zpool_cmd($action, "{$pool} {$dev}", true);
			    }
			} else {
			    zfs_zpool_cmd($action, "{$pool} {$device}", true);
			}
			break;
		    }
		}
		break;

		case "offline": {
		    switch ($option) {
		    case "t":
			$result = zfs_zpool_cmd($action, "-t {$pool} {$device}", true);
			if ($result == 0) {
			    echo gettext("Done.")."\n";
			}
			break;

		    case "d":
			if (is_array($device)) {
			    $result = 0;
			    foreach ($device as $dev) {
				$result |= zfs_zpool_cmd($action, "{$pool} {$dev}", true);
			    }
			} else {
			    $result = zfs_zpool_cmd($action, "{$pool} {$device}", true);
			}
			if ($result == 0) {
			    echo gettext("Done.")."\n";
			}
			break;
		    }
		}
		break;

		case "online": {
		    switch ($option) {
		    case "d":
			if (is_array($device)) {
			    $result = 0;
			    foreach ($device as $dev) {
				$result |= zfs_zpool_cmd($action, "{$pool} {$dev}", true);
			    }
			} else {
			    $result = zfs_zpool_cmd($action, "{$pool} {$device}", true);
			}
			if ($result == 0) {
			    echo gettext("Done.")."\n";
			}
			break;
		    }
		}
		break;

		case "attach": {
		    echo "$action...\n";
		    if ((is_array($device) && count($device) == 0) || $device == "")
			break;
		    switch ($option) {
		    case "d":
			if (is_array($device)) {
			    if (count($device) != 1) {
				echo "Only one device is allowed.\n";
				break;
			    }
			    $dev = $device[0];
			} else {
			    $dev = $device;
			}

			// Create gnop
			unset($aft4k);
			$result = 0;
			$target_device = "/dev/{$dev}";
			$attach_device = "{$new2_device}";
			if (preg_match("/^(.+)\.nop$/", $dev, $m)) {
			    $target_device = "/dev/{$m[1]}";
			    $aft4k = true;
			    if (isset($aft4k)) {
				$gnop_cmd = "gnop create -S 4096 {$new2_device}";
				write_log("$gnop_cmd");
				$result = mwexec($gnop_cmd, true);
				if ($result != 0)
				    break;
				$new2_device = "{$new2_device}.nop";
			    }
			}
			if ($result != 0)
			    break;

			$result = zfs_zpool_cmd($action, "{$pool} {$dev} {$new2_device}", true);
			if ($result == 0) {
			    $index = array_search_ex($target_device, $config['zfs']['vdevices']['vdevice'], "device");
			    if ($index !== false) {
				if ($config['zfs']['vdevices']['vdevice'][$index]['type'] == "stripe") {
				    $config['zfs']['vdevices']['vdevice'][$index]['type'] = "mirror";
				}
				$config['zfs']['vdevices']['vdevice'][$index]['device'][] = $attach_device;
			    }

			    write_config();
			    echo gettext("Done.")."\n";
			}
			break;
		    }
		}
		break;

		case "detach": {
		    echo "$action...\n";
		    if ((is_array($device) && count($device) == 0) || $device == "")
			break;
		    // get curret spare list
		    $a_spares = get_spare_list($pool);

		    switch ($option) {
		    case "d":
			if (is_array($device)) {
			    $result = 0;
			    foreach ($device as $dev) {
				$result |= zfs_zpool_cmd($action, "{$pool} {$dev}", true);
			    }
			} else {
			    $result = zfs_zpool_cmd($action, "{$pool} {$device}", true);
			}
			if ($result == 0) {
			   $target_devices = array();
			   if (is_array($device)) {
				foreach ($device as $dev) {
				    if (preg_match("/^(.+)\.nop$/", $dev, $m)) {
				        $target_devices[] = "/dev/{$m[1]}";
				    } else {
					$target_devices[] = "/dev/{$dev}";
				    }
				}
			    } else {
				if (preg_match("/^(.+)\.nop$/", $dev, $m)) {
				    $target_devices[] = "/dev/{$m[1]}";
				} else {
				    $target_devices[] = "/dev/{$dev}";
				}
			    }
			    foreach ($target_devices as $target_device) {
				$index = array_search_ex($target_device, $config['zfs']['vdevices']['vdevice'], "device");
				if ($index !== false) {
				    $type = get_device_type($target_device, $a_vdevice);

				    // remove spares if any
				    if ($type != "spare" && !empty($a_spares)) {
					foreach ($a_spares as $spares) {
					    if (in_array($target_device, $spares) == false)
						continue;
					    // ok target in the array, remove spare
					    foreach ($spares as $spare_device) {
						if (strcmp($spare_device, $target_device) == 0)
						    continue;
						$s_index = array_search_ex($spare_device, $config['zfs']['vdevices']['vdevice'], "device");
						if ($s_index !== false) {
						    $new_devices = array();
						    foreach ($config['zfs']['vdevices']['vdevice'][$s_index]['device'] as $device) {
							if (strcmp($device, $spare_device) != 0) {
							    $new_devices[] = $device;
						        }
						    }
						    $config['zfs']['vdevices']['vdevice'][$s_index]['device'] = $new_devices;
						}
					    }
var_dump($config['zfs']['vdevices']);
					    // insert spare to target vdevice
					    $config['zfs']['vdevices']['vdevice'][$index]['device'][] = $spare_device;
					}
				    }
var_dump($config['zfs']['vdevices']);

				    // cleanup spare vdevice
				    $new_vdevices = array();
				    $del_vdevices = array();
				    foreach ($config['zfs']['vdevices']['vdevice'] as $vdevice) {
					if (count($vdevice['device']) == 0) {
					    $del_vdevices[] = $vdevice;
					} else {
					    $new_vdevices[] = $vdevice;
					}
				    }
				    $config['zfs']['vdevices']['vdevice'] = $new_vdevices;

				    // reflect pool
				    $p_index = array_search_ex($pool, $config['zfs']['pools']['pool'], "name");
				    if ($p_index !== false) {
					$new_vdevices = array();
					foreach ($config['zfs']['pools']['pool'][$p_index]['vdevice'] as $vdevice) {
					    $v_index = array_search_ex($vdevice, $del_vdevices, "name");
					    if ($v_index === false) {
						$new_vdevices[] = $vdevice;
					    }
					}
					$config['zfs']['pools']['pool'][$p_index]['vdevice'] = $new_vdevices;
				    }

				    // now get replaced vdevs, remove target
				    $new_devices = array();
				    $result = 0;
				    foreach ($config['zfs']['vdevices']['vdevice'][$index]['device'] as $device) {
					if (strcmp($device, $target_device) == 0 && $config['zfs']['vdevices']['vdevice'][$index]['type'] != "spare") {
					    if (isset($config['zfs']['vdevices']['vdevice'][$index]['aft4k'])) {
						// Destroy gnop
						$gnop_cmd = "gnop destroy {$target_device}.nop";
						write_log("$gnop_cmd");
						$result = mwexec($gnop_cmd, true);
						if ($result != 0)
							break;
					    }
					} else {
					    $new_devices[] = $device;
					}
				    }
				    if ($result != 0)
					break;
				    if (count($new_devices) == 1 && $config['zfs']['vdevices']['vdevice'][$index]['type'] == "mirror") {
					$config['zfs']['vdevices']['vdevice'][$index]['type'] = "stripe";
				    }
				    $config['zfs']['vdevices']['vdevice'][$index]['device'] = $new_devices;
				}
			    }
			    if ($result != 0)
				break;

			    write_config();
			    echo gettext("Done.")."\n";
			}
			break;
		    }
		}
		break;

		case "remove": {
		    switch ($option) {
		    case "d":
			if (is_array($device)) {
			    $result = 0;
			    foreach ($device as $dev) {
				$result |= zfs_zpool_cmd($action, "{$pool} {$dev}", true);
			    }
			} else if (!empty($device)) {
			    $result = zfs_zpool_cmd($action, "{$pool} {$device}", true);
			}
			if ($result == 0) {
			    echo gettext("Done.")."\n";
			}
			break;
		    }
		}
		break;

		case "replace": {
		    switch ($option) {
		    case "d":
			if (is_array($device)) {
			    $result = 0;
			    foreach ($device as $dev) {
				$result |= zfs_zpool_cmd($action, "{$pool} {$dev} {$new_device}", true);
			    }
			} else {
			    $result = zfs_zpool_cmd($action, "{$pool} {$device} {$new_device}", true);
			}
			if ($result == 0) {
			    echo gettext("Done.")."\n";
			}
			break;
		    }
		}
		break;

		case "cache add": {
		    switch ($option) {
		    case "d":
			if ($cache_device == '')
				break;
			$index = array_search_ex($cache_device, $a_vdevice_cache, "name");
			if ($index === false)
				break;
			$vdevice = $a_vdevice_cache[$index];
			$device = $vdevice['device'];
			$result = 0;
			if (isset($vdevice['aft4k'])) {
				$a = array();
				foreach ($device as $dev) {
					$gnop_cmd = "gnop create -S 4096 {$dev}";
					write_log("$gnop_cmd");
					$result = mwexec($gnop_cmd, true);
					if ($result != 0)
						break;
					$a[] = "${dev}.nop";
				}
				$device = $a;
			}
			if ($result != 0)
				break;
			$devs = implode(" ", $device);
			$result = zfs_zpool_cmd("add", "{$pool} cache {$devs}", true);
			// Update config
			if ($result == 0) {
				$index = array_search_ex($pool, $config['zfs']['pools']['pool'], "name");
				if ($index !== false) {
					$config['zfs']['pools']['pool'][$index]['vdevice'][] = $cache_device;
					write_config();
					echo gettext("Done.")."\n";
				}
			}
			break;
		    }
		}
		break;

		case "cache remove": {
		    switch ($option) {
		    case "d":
			if ($cache_device == '')
				break;
			$index = array_search_ex($cache_device, $a_vdevice_cache, "name");
			if ($index === false)
				break;
			$vdevice = $a_vdevice_cache[$index];
			$device = $vdevice['device'];
			$result = 0;
			if (isset($vdevice['aft4k'])) {
				$a = array();
				foreach ($device as $dev) {
					$a[] = "${dev}.nop";
				}
				$device = $a;
			}
			$devs = implode(" ", $device);
			$result = zfs_zpool_cmd("remove", "{$pool} {$devs}", true);

			// Destroy gnop
			if ($result == 0) {
				$device = $vdevice['device'];
				$result = 0;
				if (isset($vdevice['aft4k'])) {
					foreach ($device as $dev) {
						$gnop_cmd = "gnop destroy {$dev}.nop";
						write_log("$gnop_cmd");
						$result = mwexec($gnop_cmd, true);
						if ($result != 0)
							break;
					}
				}
				if ($result != 0)
					break;
			}

			// Update config
			if ($result == 0) {
				$index = array_search_ex($pool, $config['zfs']['pools']['pool'], "name");
				if ($index !== false) {
					$a_vdevice = $config['zfs']['pools']['pool'][$index]['vdevice'];
					$new_vdevice = array();
					foreach ($a_vdevice as $vdevice) {
						if (strcmp($vdevice, $cache_device) != 0) {
							$new_vdevice[] = $vdevice;
						}
					}
					$config['zfs']['pools']['pool'][$index]['vdevice'] = $new_vdevice;
					write_config();
					echo gettext("Done.")."\n";
				}
			}
			break;
		    }
		}
		break;

		case "spare add": {
		    echo "$action...\n";
		    switch ($option) {
		    case "d":
			if ($spare_device == '')
				break;
			$index = array_search_ex($spare_device, $a_vdevice_spare, "name");
			if ($index === false)
				break;
			$vdevice = $a_vdevice_spare[$index];
			$device = $vdevice['device'];
			$result = 0;
			if (isset($vdevice['aft4k'])) {
				$a = array();
				foreach ($device as $dev) {
					$gnop_cmd = "gnop create -S 4096 {$dev}";
					write_log("$gnop_cmd");
					$result = mwexec($gnop_cmd, true);
					if ($result != 0)
						break;
					$a[] = "${dev}.nop";
				}
				$device = $a;
			}
			if ($result != 0)
				break;
			$devs = implode(" ", $device);
			$result = zfs_zpool_cmd("add", "{$pool} spare {$devs}", true);
			// Update config
			if ($result == 0) {
				$index = array_search_ex($pool, $config['zfs']['pools']['pool'], "name");
				if ($index !== false) {
					$config['zfs']['pools']['pool'][$index]['vdevice'][] = $spare_device;
					write_config();
					echo gettext("Done.")."\n";
				}
			}
			break;
		    }
		}
		break;

		case "spare remove": {
		    echo "$action...\n";
		    switch ($option) {
		    case "d":
			if ($spare_device == '')
				break;
			$index = array_search_ex($spare_device, $a_vdevice_spare, "name");
			if ($index === false)
				break;
			$vdevice = $a_vdevice_spare[$index];
			$device = $vdevice['device'];
			$result = 0;
			if (isset($vdevice['aft4k'])) {
				$a = array();
				foreach ($device as $dev) {
					$a[] = "${dev}.nop";
				}
				$device = $a;
			}
			$devs = implode(" ", $device);
			$result = zfs_zpool_cmd("remove", "{$pool} {$devs}", true);

			// Destroy gnop
			if ($result == 0) {
				$device = $vdevice['device'];
				$result = 0;
				if (isset($vdevice['aft4k'])) {
					foreach ($device as $dev) {
						$gnop_cmd = "gnop destroy {$dev}.nop";
						write_log("$gnop_cmd");
						$result = mwexec($gnop_cmd, true);
						if ($result != 0)
							break;
					}
				}
				if ($result != 0)
					break;
			}

			// Update config
			if ($result == 0) {
				$index = array_search_ex($pool, $config['zfs']['pools']['pool'], "name");
				if ($index !== false) {
					$a_vdevice = $config['zfs']['pools']['pool'][$index]['vdevice'];
					$new_vdevice = array();
					foreach ($a_vdevice as $vdevice) {
						if (strcmp($vdevice, $spare_device) != 0) {
							$new_vdevice[] = $vdevice;
						}
					}
					$config['zfs']['pools']['pool'][$index]['vdevice'] = $new_vdevice;
					write_config();
					echo gettext("Done.")."\n";
				}
			}
			break;
		    }
		}
		break;

		case "vdev add": {
		    echo "$action...\n";
		    switch ($option) {
		    case "d":
			if ($vdev_device == '')
				break;
			$index = array_search_ex($vdev_device, $a_vdevice_vdev, "name");
			if ($index === false)
				break;
			$vdevice = $a_vdevice_vdev[$index];
			$device = $vdevice['device'];
			$result = 0;
			if (isset($vdevice['aft4k'])) {
				$a = array();
				foreach ($device as $dev) {
					$gnop_cmd = "gnop create -S 4096 {$dev}";
					write_log("$gnop_cmd");
					$result = mwexec($gnop_cmd, true);
					if ($result != 0)
						break;
					$a[] = "${dev}.nop";
				}
				$device = $a;
			}
			if ($result != 0)
				break;
			$devs = implode(" ", $device);
			if ($vdevice['type'] == 'stripe') {
				$type = "";
			} else {
				$type = "{$vdevice['type']} ";
			}
			$result = zfs_zpool_cmd("add", "{$pool} {$type} {$devs}", true);
			// Update config
			if ($result == 0) {
				$index = array_search_ex($pool, $config['zfs']['pools']['pool'], "name");
				if ($index !== false) {
					$config['zfs']['pools']['pool'][$index]['vdevice'][] = $vdev_device;
					write_config();
					echo gettext("Done.")."\n";
				}
			}
			break;
		    }
		}
		break;

		}
		echo('</pre>');
	    };?>
	<?php include("formend.inc");?>
      </form>
    </td>
  </tr>
</table>
<script type="text/javascript">
command_change();
</script>
<?php include("fend.inc");?>
