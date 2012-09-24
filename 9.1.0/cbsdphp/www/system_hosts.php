#!/usr/local/bin/php
<?php
/*
	system_hosts.php

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

$pgtitle = array(gettext("Network"), gettext("Hosts"));

if ($_POST) {
	if ($_POST['Submit']) {
		unset($input_errors);
		$pconfig = $_POST;

		if (!$input_errors) {
			unset($config['system']['hostsacl']['rule']);
			foreach (explode("\n", $_POST['hostsacl']) as $rule) {
				$rule = trim($rule, "\t\n\r");
				if (!empty($rule))
					$config['system']['hostsacl']['rule'][] = $rule;
			}

			write_config();
		}
	}

	if ($_POST['apply'] || $_POST['Submit']) {
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			$retval |= updatenotify_process("hosts", "hosts_process_updatenotification");
			config_lock();
			$retval |= rc_exec_service("hosts"); // Update /etc/hosts
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
		if ($retval == 0) {
			updatenotify_delete("hosts");
		}
	}
}

if (!isset($config['system']['hosts']) || !is_array($config['system']['hosts']))
	$config['system']['hosts'] = array();

if (!isset($config['system']['hostsacl']['rule']) || !is_array($config['system']['hostsacl']['rule']))
	$config['system']['hostsacl']['rule'] = array();


array_sort_key($config['system']['hosts'], "name");

$a_hosts = $config['system']['hosts'];

if (is_array($config['system']['hostsacl']['rule']))
	$pconfig['hostsacl'] = implode("\n", $config['system']['hostsacl']['rule']);

if ($_GET['act'] === "del") {
	updatenotify_set("hosts", UPDATENOTIFY_MODE_DIRTY, $_GET['uuid']);
	header("Location: system_hosts.php");
	exit;
}

function hosts_process_updatenotification($mode, $data) {
	global $config;

	$retval = 0;

	switch ($mode) {
		case UPDATENOTIFY_MODE_NEW:
		case UPDATENOTIFY_MODE_MODIFIED:
			break;
		case UPDATENOTIFY_MODE_DIRTY:
			$cnid = array_search_ex($data, $config['system']['hosts'], "uuid");
			if (FALSE !== $cnid) {
				unset($config['system']['hosts'][$cnid]);
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
			<form action="system_hosts.php" method="post">
				<?php if ($savemsg) print_info_box($savemsg);?>
				<?php if (updatenotify_exists("hosts")) print_config_change_box();?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<tr>
						<td width="22%" valign="top" class="vncell"><?=gettext("Hostname database");?></td>
						<td width="78%" class="vtable">
							<table width="100%" border="0" cellpadding="0" cellspacing="0">
								<tr>
									<td width="25%" class="listhdrlr"><?=gettext("Hostname");?></td>
									<td width="30%" class="listhdrr"><?=gettext("IP address");?></td>
									<td width="35%" class="listhdrr"><?=gettext("Description");?></td>
									<td width="10%" class="list"></td>
								</tr>
								<?php foreach ($a_hosts as $host):?>
								<?php if (empty($host['uuid'])) continue;?>
								<?php $notificationmode = updatenotify_get_mode("hosts", $host['uuid']);?>
								<tr>
									<td class="listlr"><?=htmlspecialchars($host['name']);?>&nbsp;</td>
									<td class="listr"><?=htmlspecialchars($host['address']);?>&nbsp;</td>
									<td class="listbg"><?=htmlspecialchars($host['descr']);?>&nbsp;</td>
									<?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
									<td valign="middle" nowrap="nowrap" class="list">
										<a href="system_hosts_edit.php?uuid=<?=$host['uuid'];?>"><img src="e.gif" title="<?=gettext("Edit Host");?>" border="0" alt="<?=gettext("Edit Host");?>" /></a>
										<a href="system_hosts.php?act=del&amp;uuid=<?=$host['uuid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this host?");?>')"><img src="x.gif" title="<?=gettext("Delete Host");?>" border="0" alt="<?=gettext("Delete Host");?>" /></a>
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
									<td class="list"><a href="system_hosts_edit.php"><img src="plus.gif" title="<?=gettext("Add Host");?>" border="0" alt="<?=gettext("Add Host");?>" /></a></td>
								</tr>
							</table>
						</td>
					</tr>
					<?php html_textarea("hostsacl", gettext("Host access control"), $pconfig['hostsacl'], gettext("The basic configuration usually takes the form of 'daemon : address : action'. Where daemon is the daemon name of the service started. The address can be a valid hostname, an IP address or an IPv6 address enclosed in brackets. The action field can be either allow or deny to grant or deny access appropriately. Keep in mind that configuration works off a first rule match semantic, meaning that the configuration file is scanned in ascending order for a matching rule. When a match is found the rule is applied and the search process will halt.") . " " . sprintf(gettext("To get detailed informations about TCP Wrappers check the FreeBSD <a href='%s' target='_blank'>documentation</a>."), "http://www.freebsd.org/doc/en/books/handbook/tcpwrappers.html"), false, 80, 8, false, false);?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save and Restart");?>" />
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
