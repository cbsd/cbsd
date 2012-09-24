#!/usr/local/bin/php
<?php
/*
	services_rsyncd.php

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

$pgtitle = array(gettext("Services"), gettext("Rsync"), gettext("Server"), gettext("Settings"));

if (!isset($config['access']) || !is_array($config['access']))
	$config['access']['user'] = array();

array_sort_key($config['access']['user'], "login");
$a_user = &$config['access']['user'];

if (!isset($config['rsync']) || !is_array($config['rsync']))
	$config['rsync'] = array();

$pconfig['enable'] = isset($config['rsyncd']['enable']);
$pconfig['port'] = $config['rsyncd']['port'];
$pconfig['motd'] = base64_decode($config['rsyncd']['motd']);
$pconfig['rsyncd_user'] = $config['rsyncd']['rsyncd_user'];
if (is_array($config['rsyncd']['auxparam']))
	$pconfig['auxparam'] = implode("\n", $config['rsyncd']['auxparam']);

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['enable']) {
		// Input validation.
		$reqdfields = explode(" ", "rsyncd_user port");
		$reqdfieldsn = array(gettext("Map to user"), gettext("TCP port"));
		$reqdfieldst = explode(" ", "string port");

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);
		}	

	if (!$input_errors) {
		$config['rsyncd']['enable'] = $_POST['enable'] ? true : false;
		$config['rsyncd']['port'] = $_POST['port'];
		$config['rsyncd']['motd'] = base64_encode($_POST['motd']); // Encode string, otherwise line breaks will get lost
		$config['rsyncd']['rsyncd_user'] = $_POST['rsyncd_user'];

		# Write additional parameters.
		unset($config['rsyncd']['auxparam']);
		foreach (explode("\n", $_POST['auxparam']) as $auxparam) {
			$auxparam = trim($auxparam, "\t\n\r");
			if (!empty($auxparam))
				$config['rsyncd']['auxparam'][] = $auxparam;
		}

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("rsyncd");
			$retval |= rc_update_service("mdnsresponder");
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
	}
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	var endis = !(document.iform.enable.checked || enable_change);
	document.iform.port.disabled = endis;
	document.iform.auxparam.disabled = endis;
	document.iform.motd.disabled = endis;
	document.iform.rsyncd_user.disabled = endis;
}
//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav">
				<li class="tabact"><a href="services_rsyncd.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Server");?></span></a></li>
				<li class="tabinact"><a href="services_rsyncd_client.php"><span><?=gettext("Client");?></span></a></li>
				<li class="tabinact"><a href="services_rsyncd_local.php"><span><?=gettext("Local");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav2">
				<li class="tabact"><a href="services_rsyncd.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Settings");?></span></a></li>
				<li class="tabinact"><a href="services_rsyncd_module.php"><span><?=gettext("Modules");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="services_rsyncd.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gettext("Rsync"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
					<tr>
						<td valign="top" class="vncellreq"><?=gettext("Map to user");?></td>
						<td class="vtable">
							<select name="rsyncd_user" class="formfld" id="rsyncd_user">
								<option value="ftp" <?php if ("ftp" === $pconfig['rsyncd_user']) echo "selected=\"selected\"";?>><?=gettext("Guest");?></option>
								<?php foreach ($a_user as $user):?>
								<option value="<?=$user['login'];?>" <?php if ($user['login'] === $pconfig['rsyncd_user']) echo "selected=\"selected\"";?>><?php echo htmlspecialchars($user['login']);?></option>
								<?php endforeach;?>
							</select>
						</td>
					</tr>
					<tr>
						<td width="22%" valign="top" class="vncellreq"><?=gettext("TCP port");?></td>
						<td width="78%" class="vtable">
							<input name="port" type="text" class="formfld" id="port" size="20" value="<?=htmlspecialchars($pconfig['port']);?>" />
							<br /><?=gettext("Alternate TCP port. Default is 873");?>
						</td>
					</tr>
					<?php html_textarea("motd", gettext("MOTD"), $pconfig['motd'], gettext("Message of the day."), false, 65, 7, false, false);?>
					<?php html_textarea("auxparam", gettext("Auxiliary parameters"), $pconfig['auxparam'], sprintf(gettext("These parameters will be added to [global] settings in %s."), "rsyncd.conf") . " " . sprintf(gettext("Please check the <a href='%s' target='_blank'>documentation</a>."), "http://rsync.samba.org/ftp/rsync/rsyncd.conf.html"), false, 65, 5, false, false);?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save and Restart");?>" onclick="enable_change(true)" />
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<script type="text/javascript">
<!--
enable_change(false);
//-->
</script>
<?php include("fend.inc");?>
