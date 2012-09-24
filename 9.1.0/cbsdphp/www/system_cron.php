#!/usr/local/bin/php
<?php
/*
	system_cron.php

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	All rights reserved.
	
	Portions of m0n0wall (http://m0n0.ch/wall).
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

$pgtitle = array(gettext("System"), gettext("Advanced"), gettext("Cron"));

if ($_POST) {
	if ($_POST['apply']) {
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			$retval |= updatenotify_process("cronjob", "cronjob_process_updatenotification");
			config_lock();
			$retval |= rc_update_service("cron");
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
		if ($retval == 0) {
			updatenotify_delete("cronjob");
		}
	}
}

if (!isset($config['cron']['job']) || !is_array($config['cron']['job']))
	$config['cron']['job'] = array();

$a_cron = &$config['cron']['job'];

if ($_GET['act'] === "del") {
	updatenotify_set("cronjob", UPDATENOTIFY_MODE_DIRTY, $_GET['uuid']);
	header("Location: system_cron.php");
	exit;
}

function cronjob_process_updatenotification($mode, $data) {
	global $config;

	$retval = 0;

	switch ($mode) {
		case UPDATENOTIFY_MODE_NEW:
		case UPDATENOTIFY_MODE_MODIFIED:
			break;
		case UPDATENOTIFY_MODE_DIRTY:
			if (is_array($config['cron']['job'])) {
				$index = array_search_ex($data, $config['cron']['job'], "uuid");
				if (false !== $index) {
					unset($config['cron']['job'][$index]);
					write_config();
				}
			}
			break;
	}

	return $retval;
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
      	<li class="tabinact"><a href="system_advanced.php"><span><?=gettext("Advanced");?></span></a></li>
      	<li class="tabinact"><a href="system_email.php"><span><?=gettext("Email");?></span></a></li>
      	<li class="tabinact"><a href="system_proxy.php"><span><?=gettext("Proxy");?></span></a></li>
      	<li class="tabinact"><a href="system_swap.php"><span><?=gettext("Swap");?></span></a></li>
      	<li class="tabinact"><a href="system_rc.php"><span><?=gettext("Command scripts");?></span></a></li>
        <li class="tabact"><a href="system_cron.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Cron");?></span></a></li>
        <li class="tabinact"><a href="system_rcconf.php"><span><?=gettext("rc.conf");?></span></a></li>
        <li class="tabinact"><a href="system_sysctl.php"><span><?=gettext("sysctl.conf");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
    	<form action="system_cron.php" method="post">
    		<?php if ($savemsg) print_info_box($savemsg);?>
	    	<?php if (updatenotify_exists("cronjob")) print_config_change_box();?>
	      <table width="100%" border="0" cellpadding="0" cellspacing="0">
	        <tr>
						<td width="40%" class="listhdrlr"><?=gettext("Command");?></td>
						<td width="10%" class="listhdrr"><?=gettext("Who");?></td>
						<td width="40%" class="listhdrr"><?=gettext("Description");?></td>
						<td width="10%" class="list"></td>
	        </tr>
				  <?php foreach($a_cron as $job):?>
				  <?php $notificationmode = updatenotify_get_mode("cronjob", $job['uuid']);?>
	        <tr>
	        	<?php $enable = isset($job['enable']);?>
	        	<td class="<?=$enable?"listlr":"listlrd";?>"><?=htmlspecialchars($job['command']);?>&nbsp;</td>
	          <td class="<?=$enable?"listr":"listrd";?>"><?=htmlspecialchars($job['who']);?>&nbsp;</td>
	          <td class="listbg"><?=htmlspecialchars($job['desc']);?>&nbsp;</td>
	          <?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
	          <td valign="middle" nowrap="nowrap" class="list">
							<a href="system_cron_edit.php?uuid=<?=$job['uuid'];?>"><img src="e.gif" title="<?=gettext("Edit job");?>" border="0" alt="<?=gettext("Edit job");?>" /></a>
							<a href="system_cron.php?act=del&amp;uuid=<?=$job['uuid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this cron job?");?>')"><img src="x.gif" title="<?=gettext("Delete job");?>" border="0" alt="<?=gettext("Delete job");?>" /></a>
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
	          <td class="list">
							<a href="system_cron_edit.php"><img src="plus.gif" title="<?=gettext("Add job");?>" border="0" alt="<?=gettext("Add job");?>" /></a>
						</td>
	        </tr>
	      </table>
	      <?php include("formend.inc");?>
			</form>
    </td>
  </tr>
</table>
<?php include("fend.inc");?>
