#!/usr/local/bin/php
<?php
/*
	services_iscsitarget_ag.php

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

$pgtitle = array(gettext("Services"), gettext("iSCSI Target"), gettext("Auth Group"));

if ($_POST) {
	$pconfig = $_POST;

	if ($_POST['apply']) {
		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			$retval |= updatenotify_process("iscsitarget_ag", "iscsitargetag_process_updatenotification");
			config_lock();
			$retval |= rc_update_reload_service("iscsi_target");
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
		if ($retval == 0) {
			$savemsg .= "<br>";
			$savemsg .= sprintf(gettext("The reloading request has been sent to the daemon. You can see the result by <a href=\"%s\">Log</a>."), "diag_log.php?log=2");
			updatenotify_delete("iscsitarget_ag");
		}
	}
}

if (!isset($config['iscsitarget']['authgroup']) || !is_array($config['iscsitarget']['authgroup']))
	$config['iscsitarget']['authgroup'] = array();

array_sort_key($config['iscsitarget']['authgroup'], "tag");
$a_iscsitarget_ag = &$config['iscsitarget']['authgroup'];

if (!isset($config['iscsitarget']['target']) || !is_array($config['iscsitarget']['target']))
	$config['iscsitarget']['target'] = array();

if ($_GET['act'] === "del") {
	$index = array_search_ex($_GET['uuid'], $config['iscsitarget']['authgroup'], "uuid");
	if ($index !== false) {
		$ag = $config['iscsitarget']['authgroup'][$index];
		if ($ag['tag'] == $config['iscsitarget']['discoveryauthgroup']) {
			$input_errors[] = gettext("This tag is used.");
		}
		foreach ($config['iscsitarget']['target'] as $target) {
			if (isset($target['agmap'])) {
				foreach ($target['agmap'] as $agmap) {
					if ($agmap['agtag'] == $ag['tag']) {
						$input_errors[] = gettext("This tag is used.");
					}
				}
			}
		}
	}

	if (!$input_errors) {
		updatenotify_set("iscsitarget_ag", UPDATENOTIFY_MODE_DIRTY, $_GET['uuid']);
		header("Location: services_iscsitarget_ag.php");
		exit;
	}
}

function iscsitargetag_process_updatenotification($mode, $data) {
	global $config;

	$retval = 0;

	switch ($mode) {
		case UPDATENOTIFY_MODE_DIRTY:
			$cnid = array_search_ex($data, $config['iscsitarget']['authgroup'], "uuid");
			if (FALSE !== $cnid) {
				unset($config['iscsitarget']['authgroup'][$cnid]);
				write_config();
			}
			break;
	}

	return $retval;
}
?>
<?php include("fbegin.inc");?>
<form action="services_iscsitarget_ag.php" method="post" name="iform" id="iform">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
        <li class="tabinact"><a href="services_iscsitarget.php"><span><?=gettext("Settings");?></span></a></li>
        <li class="tabinact"><a href="services_iscsitarget_target.php"><span><?=gettext("Targets");?></span></a></li>
        <li class="tabinact"><a href="services_iscsitarget_pg.php"><span><?=gettext("Portals");?></span></a></li>
				<li class="tabinact"><a href="services_iscsitarget_ig.php"><span><?=gettext("Initiators");?></span></a></li>
				<li class="tabact"><a href="services_iscsitarget_ag.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Auths");?></span></a></li>
				<li class="tabinact"><a href="services_iscsitarget_media.php"><span><?=gettext("Media");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
      <?php if ($input_errors) print_input_errors($input_errors);?>
      <?php if ($savemsg) print_info_box($savemsg);?>
      <?php if (updatenotify_exists("iscsitarget_ag")) print_config_change_box();?>
      <table width="100%" border="0" cellpadding="6" cellspacing="0">
      <tr>
        <td colspan="2" valign="top" class="listtopic"><?=gettext("Auth Groups");?></td>
      </tr>
      <tr>
        <td width="22%" valign="top" class="vncell"><?=gettext("Auth Group");?></td>
        <td width="78%" class="vtable">
        <table width="100%" border="0" cellpadding="0" cellspacing="0">
        <tr>
          <td width="5%" class="listhdrlr"><?=gettext("Tag");?></td>
          <td width="30%" class="listhdrr"><?=gettext("CHAP Users");?></td>
          <td width="30%" class="listhdrr"><?=gettext("Mutual CHAP Users");?></td>
          <td width="25%" class="listhdrr"><?=gettext("Comment");?></td>
          <td width="10%" class="list"></td>
        </tr>
        <?php foreach($config['iscsitarget']['authgroup'] as $ag):?>
        <?php
			if (!is_array($ag['agauth']))
				$ag['agauth'] = array();
			array_sort_key($ag['agauth'], "authuser");
        ?>
        <?php $notificationmode = updatenotify_get_mode("iscsitarget_ag", $ag['uuid']);?>
        <tr>
          <td class="listlr"><?=htmlspecialchars($ag['tag']);?>&nbsp;</td>
          <td class="listr">
          <?php if (count($ag['agauth']) == 0) echo "&nbsp;"; ?>
          <?php foreach ($ag['agauth'] as $agauth): ?>
          <?php echo htmlspecialchars($agauth['authuser'])."<br />\n"; ?>
          <?php endforeach; ?>
          </td>
          <td class="listr">
          <?php if (count($ag['agauth']) == 0) echo "&nbsp;"; ?>
          <?php foreach ($ag['agauth'] as $agauth): ?>
          <?php echo htmlspecialchars($agauth['authmuser'])."<br />\n"; ?>
          <?php endforeach; ?>
          </td>
          <td class="listr"><?=htmlspecialchars($ag['comment']);?>&nbsp;</td>
          <?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
          <td valign="middle" nowrap="nowrap" class="list">
            <a href="services_iscsitarget_ag_edit.php?uuid=<?=$ag['uuid'];?>"><img src="e.gif" title="<?=gettext("Edit auth group");?>" border="0" alt="<?=gettext("Edit auth group");?>" /></a>
            <a href="services_iscsitarget_ag.php?act=del&amp;type=ag&amp;uuid=<?=$ag['uuid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this auth group?");?>')"><img src="x.gif" title="<?=gettext("Delete auth group");?>" border="0" alt="<?=gettext("Delete auth group");?>" /></a>
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
          <td class="list"><a href="services_iscsitarget_ag_edit.php"><img src="plus.gif" title="<?=gettext("Add auth group");?>" border="0" alt="<?=gettext("Add auth group");?>" /></a></td>
        </tr>
        </table>
        <?=gettext("A Auth Group contains authorised users and secrets for additional security.");?>
        </td>
      </tr>
      </table>
    </td>
  </tr>
</table>
<?php include("formend.inc");?>
</form>
<?php include("fend.inc");?>
