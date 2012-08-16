#!/usr/local/bin/php
<?php
/*
	services_rsyncd_client.php

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.
	
	Modified for XHTML by Daisuke Aoyama <aoyama@peach.ne.jp>
	Copyright (C) 2010 Daisuke Aoyama <aoyama@peach.ne.jp>.	
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

$pgtitle = array(gettext("Services"), gettext("Rsync"), gettext("Client"));

if ($_POST) {
	$pconfig = $_POST;

	if ($_POST['apply']) {
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			$retval |= updatenotify_process("rsyncclient", "rsyncclient_process_updatenotification");
			config_lock();
			$retval |= rc_exec_service("rsync_client");
			$retval |= rc_update_service("cron");
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
		if ($retval == 0) {
			updatenotify_delete("rsyncclient");
		}
	}
}

if (!isset($config['rsync']) || !is_array($config['rsync'])) {
	$config['rsync'] = array();
	if (!isset($config['rsync']['rsyncclient']) || !is_array($config['rsync']['rsyncclient']))
		$config['rsync']['rsyncclient'] = array();
} else if (!isset($config['rsync']['rsyncclient']) || !is_array($config['rsync']['rsyncclient'])) {
	$config['rsync']['rsyncclient'] = array();
}

$a_rsyncclient = &$config['rsync']['rsyncclient'];

if ($_GET['act'] === "del") {
	updatenotify_set("rsyncclient", UPDATENOTIFY_MODE_DIRTY, $_GET['uuid']);
	header("Location: services_rsyncd_client.php");
	exit;
}

function rsyncclient_process_updatenotification($mode, $data) {
	global $config;

	$retval = 0;

	switch ($mode) {
		case UPDATENOTIFY_MODE_NEW:
		case UPDATENOTIFY_MODE_MODIFIED:
			break;
		case UPDATENOTIFY_MODE_DIRTY:
			$cnid = array_search_ex($data, $config['rsync']['rsyncclient'], "uuid");
			if (FALSE !== $cnid) {
				unset($config['rsync']['rsyncclient'][$cnid]);
				write_config();
			}
			@unlink("/var/run/rsync_client_{$data}.sh");
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
				<li class="tabinact"><a href="services_rsyncd.php"><span><?php echo gettext("Server");?></span></a></li>
				<li class="tabact"><a href="services_rsyncd_client.php" title="<?php echo gettext("Reload page");?>"><span><?php echo gettext("Client");?></span></a></li>
				<li class="tabinact"><a href="services_rsyncd_local.php"><span><?php echo gettext("Local");?></span></a></li>
			</ul>
		</td>
	</tr>
  <tr>
    <td class="tabcont">
      <form action="services_rsyncd_client.php" method="post">
        <?php if ($savemsg) print_info_box($savemsg);?>
        <?php if (updatenotify_exists("rsyncclient")) print_config_change_box();?>
        <table width="100%" border="0" cellpadding="0" cellspacing="0">
          <tr>
						<td width="20%" class="listhdrlr"><?php echo gettext("Remote module (source)");?></td>
						<td width="15%" class="listhdrr"><?php echo gettext("Remote address");?></td>
						<td width="15%" class="listhdrr"><?php echo gettext("Local share (destination)");?></td>
						<td width="10%" class="listhdrr"><?php echo gettext("Who");?></td>
						<td width="30%" class="listhdrr"><?php echo gettext("Description");?></td>
            <td width="10%" class="list"></td>
          </tr>
  			  <?php foreach($a_rsyncclient as $rsyncclient):?>
  			  <?php $notificationmode = updatenotify_get_mode("rsyncclient", $rsyncclient['uuid']);?>
          <tr>
          	<?php $enable = isset($rsyncclient['enable']);?>
						<td class="<?php $enable?"listlr":"listlrd";?>"><?php htmlspecialchars($rsyncclient['remoteshare']);?>&nbsp;</td>
						<td class="<?php $enable?"listr":"listrd";?>"><?php htmlspecialchars($rsyncclient['rsyncserverip']);?>&nbsp;</td>
						<td class="<?php $enable?"listr":"listrd";?>"><?php htmlspecialchars($rsyncclient['localshare']);?>&nbsp;</td>
						<td class="<?php $enable?"listr":"listrd";?>"><?php htmlspecialchars($rsyncclient['who']);?>&nbsp;</td>
						<td class="listbg"><?php htmlspecialchars($rsyncclient['description']);?>&nbsp;</td>
						<?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
            <td valign="middle" nowrap="nowrap" class="list">
							<a href="services_rsyncd_client_edit.php?uuid=<?php $rsyncclient['uuid'];?>"><img src="e.gif" title="<?php echo gettext("Edit Rsync job");?>" border="0" alt="<?php echo gettext("Edit Rsync job");?>" /></a>&nbsp;
              <a href="services_rsyncd_client.php?act=del&amp;uuid=<?php $rsyncclient['uuid'];?>" onclick="return confirm('<?php echo gettext("Do you really want to delete this Rsync job?");?>')"><img src="x.gif" title="<?php echo gettext("Delete Rsync job"); ?>" border="0" alt="<?php echo gettext("Delete Rsync job"); ?>" /></a>
            </td>
            <?php else:?>
						<td valign="middle" nowrap="nowrap" class="list">
							<img src="del.gif" border="0" alt="" />
						</td>
						<?php endif;?>
          </tr>
          <?php endforeach;?>
          <tr> 
            <td class="list" colspan="5"></td>
            <td class="list"><a href="services_rsyncd_client_edit.php"><img src="plus.gif" title="<?php echo gettext("Add Rsync job");?>" border="0" alt="<?php echo gettext("Add Rsync job");?>" /></a></td>
			    </tr>
        </table>
        <?php include("formend.inc");?>
      </form>
	  </td>
  </tr>
</table>
<?php include("fend.inc");?>
