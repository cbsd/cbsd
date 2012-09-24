#!/usr/local/bin/php
<?php
/*
	services_rsyncd_local.php

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

$pgtitle = array(gettext("Services"), gettext("Rsync"), gettext("Local"));

if ($_POST) {
	$pconfig = $_POST;

	if ($_POST['apply']) {
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			$retval |= updatenotify_process("rsynclocal", "rsynclocal_process_updatenotification");
			config_lock();
			$retval |= rc_exec_service("rsync_local");
			$retval |= rc_update_service("cron");
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
		if ($retval == 0) {
			updatenotify_delete("rsynclocal");
		}
	}
}

if (!isset($config['rsync']) || !is_array($config['rsync'])) {
	$config['rsync'] = array();
	if (!isset($config['rsync']['rsynclocal']) || !is_array($config['rsync']['rsynclocal']))
		$config['rsync']['rsynclocal'] = array();
} else if (!isset($config['rsync']['rsynclocal']) || !is_array($config['rsync']['rsynclocal'])) {
	$config['rsync']['rsynclocal'] = array();
}

$a_rsynclocal = &$config['rsync']['rsynclocal'];

if ($_GET['act'] === "del") {
	updatenotify_set("rsynclocal", UPDATENOTIFY_MODE_DIRTY, $_GET['uuid']);
	header("Location: services_rsyncd_local.php");
	exit;
}

function rsynclocal_process_updatenotification($mode, $data) {
	global $config;

	$retval = 0;

	switch ($mode) {
		case UPDATENOTIFY_MODE_NEW:
		case UPDATENOTIFY_MODE_MODIFIED:
			break;
		case UPDATENOTIFY_MODE_DIRTY:
			$cnid = array_search_ex($data, $config['rsync']['rsynclocal'], "uuid");
			if (FALSE !== $cnid) {
				unset($config['rsync']['rsynclocal'][$cnid]);
				write_config();
			}
			@unlink("/var/run/rsync_local_{$data}.sh");
			break;
	}

	return $retval;
}
?>
<?php include("fbegin.inc"); ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav">
				<li class="tabinact"><a href="services_rsyncd.php"><span><?=gettext("Server") ;?></span></a></li>
				<li class="tabinact"><a href="services_rsyncd_client.php"><span><?=gettext("Client") ;?></span></a></li>
				<li class="tabact"><a href="services_rsyncd_local.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Local") ;?></span></a></li>
			</ul>
		</td>
	</tr>
  <tr>
    <td class="tabcont">
      <form action="services_rsyncd_local.php" method="post">
        <?php if ($savemsg) print_info_box($savemsg);?>
        <?php if (updatenotify_exists("rsynclocal")) print_config_change_box();?>
        <table width="100%" border="0" cellpadding="0" cellspacing="0">
          <tr>
            <td width="25%" class="listhdrlr"><?=gettext("Source share");?></td>
						<td width="25%" class="listhdrr"><?=gettext("Destination share");?></td>
						<td width="10%" class="listhdrr"><?=gettext("Who");?></td>
						<td width="30%" class="listhdrr"><?=gettext("Description");?></td>
            <td width="10%" class="list"></td>
          </tr>
  			  <?php foreach($a_rsynclocal as $rsynclocal):?>
  			  <?php $notificationmode = updatenotify_get_mode("rsynclocal", $rsynclocal['uuid']);?>
          <tr>
          	<?php $enable = isset($rsynclocal['enable']);?>
            <td class="<?=$enable?"listlr":"listlrd";?>"><?=htmlspecialchars($rsynclocal['source']);?>&nbsp;</td>
						<td class="<?=$enable?"listr":"listrd";?>"><?=htmlspecialchars($rsynclocal['destination']);?>&nbsp;</td>
						<td class="<?=$enable?"listr":"listrd";?>"><?=htmlspecialchars($rsynclocal['who']);?>&nbsp;</td>
						<td class="listbg"><?=htmlspecialchars($rsynclocal['description']);?>&nbsp;</td>
						<?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
            <td valign="middle" nowrap="nowrap" class="list">
							<a href="services_rsyncd_local_edit.php?uuid=<?=$rsynclocal['uuid'];?>"><img src="e.gif" title="<?=gettext("Edit Rsync job");?>" border="0" alt="<?=gettext("Edit Rsync job");?>" /></a>&nbsp;
              <a href="services_rsyncd_local.php?act=del&amp;uuid=<?=$rsynclocal['uuid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this Rsync job?");?>')"><img src="x.gif" title="<?=gettext("Delete Rsync job");?>" border="0" alt="<?=gettext("Delete Rsync job");?>" /></a>
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
            <td class="list"><a href="services_rsyncd_local_edit.php"><img src="plus.gif" title="<?=gettext("Add Rsync job");?>" border="0" alt="<?=gettext("Add Rsync job");?>" /></a></td>
			    </tr>
        </table>
        <?php include("formend.inc");?>
      </form>
	  </td>
  </tr>
</table>
<?php include("fend.inc");?>
