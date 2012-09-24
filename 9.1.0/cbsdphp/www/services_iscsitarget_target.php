#!/usr/local/bin/php
<?php
/*
	services_iscsitarget_target.php

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	All rights reserved.
	
	Portions of m0n0wall (http://m0n0.ch/wall)
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

$pgtitle = array(gettext("Services"), gettext("iSCSI Target"), gettext("Target"));

$pconfig['enable'] = isset($config['iscsitarget']['enable']);

if ($_POST) {
	$pconfig = $_POST;

	//$config['iscsitarget']['enable'] = $_POST['enable'] ? true : false;

	if ($_POST['apply']) {
		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			$retval |= updatenotify_process("iscsitarget_extent", "iscsitargetextent_process_updatenotification");
			$retval |= updatenotify_process("iscsitarget_target", "iscsitargettarget_process_updatenotification");
			config_lock();
			$retval |= rc_update_reload_service("iscsi_target");
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
		if ($retval == 0) {
			$savemsg .= "<br>";
			$savemsg .= sprintf(gettext("The reloading request has been sent to the daemon. You can see the result by <a href=\"%s\">Log</a>."), "diag_log.php?log=2");
			updatenotify_delete("iscsitarget_extent");
			updatenotify_delete("iscsitarget_target");
		}
	}
}

if (!isset($config['iscsitarget']['portalgroup']) || !is_array($config['iscsitarget']['portalgroup']))
	$config['iscsitarget']['portalgroup'] = array();

if (!isset($config['iscsitarget']['initiatorgroup']) || !is_array($config['iscsitarget']['initiatorgroup']))
	$config['iscsitarget']['initiatorgroup'] = array();

if (!isset($config['iscsitarget']['authgroup']) || !is_array($config['iscsitarget']['authgroup']))
	$config['iscsitarget']['authgroup'] = array();

function cmp_tag($a, $b) {
	if ($a['tag'] == $b['tag'])
		return 0;
	return ($a['tag'] > $b['tag']) ? 1 : -1;
}
usort($config['iscsitarget']['portalgroup'], "cmp_tag");
usort($config['iscsitarget']['initiatorgroup'], "cmp_tag");
usort($config['iscsitarget']['authgroup'], "cmp_tag");

if (!isset($config['iscsitarget']['extent']) || !is_array($config['iscsitarget']['extent']))
	$config['iscsitarget']['extent'] = array();

if (!isset($config['iscsitarget']['device']) || !is_array($config['iscsitarget']['device']))
	$config['iscsitarget']['device'] = array();

if (!isset($config['iscsitarget']['target']) || !is_array($config['iscsitarget']['target']))
	$config['iscsitarget']['target'] = array();

array_sort_key($config['iscsitarget']['extent'], "name");
array_sort_key($config['iscsitarget']['device'], "name");
//array_sort_key($config['iscsitarget']['target'], "name");

function get_fulliqn($name) {
	global $config;
	$fullname = $name;
	$basename = $config['iscsitarget']['nodebase'];
	if (strncasecmp("iqn.", $name, 4) != 0
		&& strncasecmp("eui.", $name, 4) != 0
		&& strncasecmp("naa.", $name, 4) != 0) {
		if (strlen($basename) != 0) {
			$fullname = $basename.":".$name;
		}
	}
	return $fullname;
}

function cmp_target($a, $b) {
	$aname = get_fulliqn($a['name']);
	$bname = get_fulliqn($b['name']);
	return strcasecmp($aname, $bname);
}
usort($config['iscsitarget']['target'], "cmp_target");

if ($_GET['act'] === "del") {
	switch ($_GET['type']) {
		case "extent":
			$index = array_search_ex($_GET['uuid'], $config['iscsitarget']['extent'], "uuid");
			if ($index !== false) {
				$extent = $config['iscsitarget']['extent'][$index];
				foreach ($config['iscsitarget']['device'] as $device) {
					if (isset($device['storage'])) {
						foreach ($device['storage'] as $storage) {
							if ($extent['name'] === $storage) {
								$input_errors[] = gettext("This extent is used.");
							}
						}
					}
				}
				foreach ($config['iscsitarget']['target'] as $target) {
					if (isset($target['storage'])) {
						foreach ($target['storage'] as $storage) {
							if ($extent['name'] === $storage) {
								$input_errors[] = gettext("This extent is used.");
							}
						}
					}
					if (isset($target['lunmap'])) {
						foreach ($target['lunmap'] as $lunmap) {
							if ($extent['name'] === $lunmap['extentname']) {
								$input_errors[] = gettext("This extent is used.");
							}
						}
					}
				}
			}
			if (!$input_errors) {
				updatenotify_set("iscsitarget_extent", UPDATENOTIFY_MODE_DIRTY, $_GET['uuid']);
				header("Location: services_iscsitarget_target.php");
				exit;
			}
			break;

		case "target":
			updatenotify_set("iscsitarget_target", UPDATENOTIFY_MODE_DIRTY, $_GET['uuid']);
			header("Location: services_iscsitarget_target.php");
			exit;
			break;
	}
}

function iscsitargetextent_process_updatenotification($mode, $data) {
	global $config;

	$retval = 0;

	switch ($mode) {
		case UPDATENOTIFY_MODE_DIRTY:
			$cnid = array_search_ex($data, $config['iscsitarget']['extent'], "uuid");
			if (FALSE !== $cnid) {
				unset($config['iscsitarget']['extent'][$cnid]);
				write_config();
			}
			break;
	}

	return $retval;
}

function iscsitargettarget_process_updatenotification($mode, $data) {
	global $config;

	$retval = 0;

	switch ($mode) {
		case UPDATENOTIFY_MODE_DIRTY:
			$cnid = array_search_ex($data, $config['iscsitarget']['target'], "uuid");
			if (FALSE !== $cnid) {
				unset($config['iscsitarget']['target'][$cnid]);
				write_config();
			}
			break;
	}

	return $retval;
}
?>
<?php include("fbegin.inc");?>
<form action="services_iscsitarget_target.php" method="post" name="iform" id="iform">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
				<li class="tabinact"><a href="services_iscsitarget.php"><span><?=gettext("Settings");?></span></a></li>
				<li class="tabact"><a href="services_iscsitarget_target.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Targets");?></span></a></li>
				<li class="tabinact"><a href="services_iscsitarget_pg.php"><span><?=gettext("Portals");?></span></a></li>
				<li class="tabinact"><a href="services_iscsitarget_ig.php"><span><?=gettext("Initiators");?></span></a></li>
				<li class="tabinact"><a href="services_iscsitarget_ag.php"><span><?=gettext("Auths");?></span></a></li>
				<li class="tabinact"><a href="services_iscsitarget_media.php"><span><?=gettext("Media");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
      <?php if ($input_errors) print_input_errors($input_errors);?>
      <?php if ($savemsg) print_info_box($savemsg);?>
      <?php if (updatenotify_exists("iscsitarget_extent") || updatenotify_exists("iscsitarget_target")) print_config_change_box();?>
      <table width="100%" border="0" cellpadding="6" cellspacing="0">
      <tr>
        <td colspan="2" valign="top" class="listtopic"><?=gettext("Targets");?></td>
      </tr>
      <tr>
        <td width="22%" valign="top" class="vncell"><?=gettext("Extent");?></td>
        <td width="78%" class="vtable">
        <table width="100%" border="0" cellpadding="0" cellspacing="0">
        <tr>
          <td width="20%" class="listhdrlr"><?=gettext("Name");?></td>
          <td width="50%" class="listhdrr"><?=gettext("Path");?></td>
          <td width="20%" class="listhdrr"><?=gettext("Size");?></td>
          <td width="10%" class="list"></td>
        </tr>
        <?php foreach($config['iscsitarget']['extent'] as $extent):?>
        <?php $sizeunit = $extent['sizeunit']; if (!$sizeunit) { $sizeunit = "MB"; }?>
        <?php if ($sizeunit === "MB") { $psizeunit = gettext("MiB"); } else if ($sizeunit === "GB") { $psizeunit = gettext("GiB"); } else if ($sizeunit === "TB") { $psizeunit = gettext("TiB"); } else if ($sizeunit === "auto") { $psizeunit = gettext("Auto"); } else { $psizeunit = $sizeunit; }?>
        <?php $notificationmode = updatenotify_get_mode("iscsitarget_extent", $extent['uuid']);?>
        <tr>
          <td class="listlr"><?=htmlspecialchars($extent['name']);?>&nbsp;</td>
          <td class="listr"><?=htmlspecialchars($extent['path']);?>&nbsp;</td>
          <td class="listr"><?=htmlspecialchars($extent['size']);?><?=htmlspecialchars($psizeunit)?>&nbsp;</td>
          <?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
          <td valign="middle" nowrap="nowrap" class="list">
            <a href="services_iscsitarget_extent_edit.php?uuid=<?=$extent['uuid'];?>"><img src="e.gif" title="<?=gettext("Edit extent");?>" border="0" alt="<?=gettext("Edit extent");?>" /></a>
            <a href="services_iscsitarget_target.php?act=del&amp;type=extent&amp;uuid=<?=$extent['uuid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this extent?");?>')"><img src="x.gif" title="<?=gettext("Delete extent");?>" border="0" alt="<?=gettext("Delete extent");?>" /></a>
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
          <td class="list"><a href="services_iscsitarget_extent_edit.php"><img src="plus.gif" title="<?=gettext("Add extent");?>" border="0" alt="<?=gettext("Add extent");?>" /></a></td>
        </tr>
        </table>
        <?=gettext("Extents must be defined before they can be used, and extents cannot be used more than once.");?>
        </td>
      </tr>
      <tr>
        <td width="22%" valign="top" class="vncell"><?=gettext("Target");?></td>
        <td width="78%" class="vtable">
        <table width="100%" border="0" cellpadding="0" cellspacing="0">
        <tr>
          <td width="35%" class="listhdrlr"><?=gettext("Name");?></td>
          <td width="4%" class="listhdrr"><?=gettext("Flags");?></td>
          <td width="30%" class="listhdrr"><?=gettext("LUNs");?></td>
          <td width="7%" class="listhdrr"><?=gettext("PG");?></td>
          <td width="7%" class="listhdrr"><?=gettext("IG");?></td>
          <td width="7%" class="listhdrr"><?=gettext("AG");?></td>
          <td width="10%" class="list"></td>
        </tr>
        <?php foreach($config['iscsitarget']['target'] as $target):?>
        <?php
			$pgtag = $target['pgigmap'][0]['pgtag'];
			$igtag = $target['pgigmap'][0]['igtag'];
			$agtag = $target['agmap'][0]['agtag'];
			$name = get_fulliqn($target['name']);
			$disabled = !isset($target['enable']) ? sprintf("(%s)", gettext("Disabled")) : "";
			if ($pgtag == 0 && count($config['iscsitarget']['portalgroup']) != 0)
				$pgtag = 1;
			if ($igtag == 0 && count($config['iscsitarget']['initiatorgroup']) != 0)
				$igtag = 1;
			$LUNs = array();
			$LUNs['0'] = "N/A";
			if (isset($target['lunmap'])) {
				foreach ($target['lunmap'] as $lunmap) {
					$index = array_search_ex($lunmap['extentname'], $config['iscsitarget']['extent'], "name");
					if (false !== $index) {
						$LUNs[$lunmap['lun']] = $config['iscsitarget']['extent'][$index]['path'];
					}
				}
			} else {
				if (isset($target['storage'])) {
					foreach ($target['storage'] as $storage) {
						$index = array_search_ex($storage, $config['iscsitarget']['extent'], "name");
						if (false !== $index) {
							$LUNs['0'] = $config['iscsitarget']['extent'][$index]['path'];
						}
					}
				}
			}
        ?>
        <?php $notificationmode = updatenotify_get_mode("iscsitarget_target", $target['uuid']);?>
        <tr>
          <td class="listlr"><?=htmlspecialchars($name);?> <?=htmlspecialchars($disabled);?>&nbsp;</td>
          <td class="listr"><?=htmlspecialchars($target['flags']);?>&nbsp;</td>
          <td class="listr">
          <?php
				foreach ($LUNs as $key => $val) {
					echo sprintf("%s%s=%s<br />", gettext("LUN"), $key, $val);
				}
          ?>
          </td>
          <td class="listr">
          <?php
				if ($pgtag == 0) {
					echo htmlspecialchars(gettext("none"));
				} else {
					echo htmlspecialchars($pgtag);
				}
          ?>
          </td>
          <td class="listr">
          <?php
				if ($igtag == 0) {
					echo htmlspecialchars(gettext("none"));
				} else {
					echo htmlspecialchars($igtag);
				}
          ?>
          </td>
          <td class="listr">
          <?php
				if ($agtag == 0) {
					echo htmlspecialchars(gettext("none"));
				} else {
					echo htmlspecialchars($agtag);
				}
          ?>
          </td>
          <?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
          <td valign="middle" nowrap="nowrap" class="list">
            <a href="services_iscsitarget_target_edit.php?uuid=<?=$target['uuid'];?>"><img src="e.gif" title="<?=gettext("Edit target");?>" border="0" alt="<?=gettext("Edit target");?>" /></a>
            <a href="services_iscsitarget_target.php?act=del&amp;type=target&amp;uuid=<?=$target['uuid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this target?");?>')"><img src="x.gif" title="<?=gettext("Delete target");?>" border="0" alt="<?=gettext("Delete target");?>" /></a>
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
          <td class="list"><a href="services_iscsitarget_target_edit.php"><img src="plus.gif" title="<?=gettext("Add target");?>" border="0" alt="<?=gettext("Add target");?>" /></a></td>
        </tr>
        </table>
        <?=gettext("At the highest level, a target is what is presented to the initiator, and is made up of one or more extents.");?>
        </td>
      </tr>
      </table>
      <div id="remarks">
        <?php html_remark("note", gettext("Note"), gettext("To configure the target, you must add at least Portal Group and Initiator Group and Extent.<br />Portal Group which is identified by tag number defines IP addresses and listening TCP ports.<br />Initiator Group which is identified by tag number defines authorised initiator names and networks.<br />Auth Group which is identified by tag number and is optional if the target does not use CHAP authentication defines authorised users and secrets for additional security.<br />Extent defines the storage area of the target."));?>
      </div>
    </td>
  </tr>
</table>
<?php include("formend.inc");?>
</form>
<?php include("fend.inc");?>
