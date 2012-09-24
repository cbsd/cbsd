#!/usr/local/bin/php
<?php
/*
	system_routes.php

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

$pgtitle = array(gettext("Network"), gettext("Static routes"));

if ($_POST) {
	$pconfig = $_POST;

	if ($_POST['apply']) {
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			$retval |= updatenotify_process("routes", "routes_process_updatenotification");
			$retval |= rc_start_service("routing");
		}
		$savemsg = get_std_save_message($retval);
		if ($retval == 0) {
			updatenotify_delete("routes");
		}
	}
}

if (!isset($config['staticroutes']['route']) || !is_array($config['staticroutes']['route']))
	$config['staticroutes']['route'] = array();

array_sort_key($config['staticroutes']['route'], "network");
$a_routes = &$config['staticroutes']['route'];

if ($_GET['act'] === "del") {
	updatenotify_set("routes", UPDATENOTIFY_MODE_DIRTY, $_GET['uuid']);
	header("Location: system_routes.php");
	exit;
}

function routes_process_updatenotification($mode, $data) {
	global $config;

	$retval = 0;

	switch ($mode) {
		case UPDATENOTIFY_MODE_NEW:
		case UPDATENOTIFY_MODE_MODIFIED:
			break;
		case UPDATENOTIFY_MODE_DIRTY:
			$cnid = array_search_ex($data, $config['staticroutes']['route'], "uuid");
			if (FALSE !== $index) {
				rc_exec_service("routing delete conf_" . strtr($config['staticroutes']['route'][$cnid]['uuid'], "-", "_"));
				unset($config['staticroutes']['route'][$cnid]);
				write_config();
			}
			break;
	}

	return $retval;
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabcont">
			<form action="system_routes.php" method="post">
				<?php if ($savemsg) print_info_box($savemsg); ?>
				<?php if (updatenotify_exists("routes")) print_config_change_box();?>
				<table width="100%" border="0" cellpadding="0" cellspacing="0">
					<tr>
						<td width="15%" class="listhdrlr"><?=gettext("Interface");?></td>
						<td width="25%" class="listhdrr"><?=gettext("Network");?></td>
						<td width="20%" class="listhdrr"><?=gettext("Gateway");?></td>
						<td width="30%" class="listhdrr"><?=gettext("Description");?></td>
						<td width="10%" class="list"></td>
					</tr>
					<?php foreach ($a_routes as $route):?>
					<?php $notificationmode = updatenotify_get_mode("routes", $route['uuid']);?>
					<tr>
						<td class="listlr">
							<?php
					  	$iflabels = array('lan' => 'LAN', 'wan' => 'WAN', 'pptp' => 'PPTP');
					  	for ($j = 1; isset($config['interfaces']['opt' . $j]); $j++)
					  	$iflabels['opt' . $j] = $config['interfaces']['opt' . $j]['descr'];
					  	echo htmlspecialchars($iflabels[$route['interface']]);?>
						</td>
	          <td class="listr"><?=strtolower($route['network']);?>&nbsp;</td>
	          <td class="listr"><?=strtolower($route['gateway']);?>&nbsp;</td>
	          <td class="listbg"><?=htmlspecialchars($route['descr']);?>&nbsp;</td>
	          <?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
	          <td valign="middle" nowrap="nowrap" class="list">
							<a href="system_routes_edit.php?uuid=<?=$route['uuid'];?>"><img src="e.gif" title="<?=gettext("Edit Route");?>" border="0" alt="<?=gettext("Edit Route");?>" /></a>
	          	<a href="system_routes.php?act=del&amp;uuid=<?=$route['uuid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this route?");?>')"><img src="x.gif" title="<?=gettext("Delete Route");?>" border="0" alt="<?=gettext("Delete Route");?>" /></a>
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
						<td class="list">
							<a href="system_routes_edit.php"><img src="plus.gif" title="<?=gettext("Add Route");?>" border="0" alt="<?=gettext("Add Route");?>" /></a>
						</td>
					</tr>
				</table>
				<?php include("formend.inc");?>
      </form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
