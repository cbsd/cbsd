#!/usr/local/bin/php
<?php
/*
	services_sshd.php

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

$pgtitle = array(gettext("Services"),gettext("SSH"));

if (!isset($config['sshd']) || !is_array($config['sshd']))
	$config['sshd'] = array();

$os_release = exec('uname -r | cut -d - -f1');

$pconfig['port'] = $config['sshd']['port'];
$pconfig['permitrootlogin'] = isset($config['sshd']['permitrootlogin']);
$pconfig['tcpforwarding'] = isset($config['sshd']['tcpforwarding']);
$pconfig['enable'] = isset($config['sshd']['enable']);
$pconfig['key'] = base64_decode($config['sshd']['private-key']);
$pconfig['passwordauthentication'] = isset($config['sshd']['passwordauthentication']);
$pconfig['compression'] = isset($config['sshd']['compression']);
$pconfig['subsystem'] = $config['sshd']['subsystem'];
if (is_array($config['sshd']['auxparam']))
	$pconfig['auxparam'] = implode("\n", $config['sshd']['auxparam']);

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = array();
	$reqdfieldsn = array();

	if ($_POST['enable']) {
		$reqdfields = array_merge($reqdfields, explode(" ", "port"));
		$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("TCP port")));
		$reqdfieldst = explode(" ", "port");
		
		if ($_POST['key']) {
			$reqdfields = array_merge($reqdfields, array("key"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Private key")));
			$reqdfieldst = array_merge($reqdfieldst, array("privatedsakey"));
		}
	}

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if (!$input_errors) {
		$config['sshd']['port'] = $_POST['port'];
		$config['sshd']['permitrootlogin'] = $_POST['permitrootlogin'] ? true : false;
		$config['sshd']['tcpforwarding'] = $_POST['tcpforwarding'] ? true : false;
		$config['sshd']['enable'] = $_POST['enable'] ? true : false;
		$config['sshd']['private-key'] = base64_encode($_POST['key']);
		$config['sshd']['passwordauthentication'] = $_POST['passwordauthentication'] ? true : false;
		$config['sshd']['compression'] = $_POST['compression'] ? true : false;
		$config['sshd']['subsystem'] = $_POST['subsystem'];

		# Write additional parameters.
		unset($config['sshd']['auxparam']);
		foreach (explode("\n", $_POST['auxparam']) as $auxparam) {
			$auxparam = trim($auxparam, "\t\n\r");
			if (!empty($auxparam))
				$config['sshd']['auxparam'][] = $auxparam;
		}

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("sshd");
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
	document.iform.key.disabled = endis;
	document.iform.permitrootlogin.disabled = endis;
	document.iform.passwordauthentication.disabled = endis;
	document.iform.tcpforwarding.disabled = endis;
	document.iform.compression.disabled = endis;
	document.iform.subsystem.disabled = endis;
	document.iform.auxparam.disabled = endis;
}
//-->
</script>
<form action="services_sshd.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
	  <tr>
	    <td class="tabcont">
		    <?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
			  <table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gettext("Secure Shell"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
			    <tr>
			      <td width="22%" valign="top" class="vncellreq"><?=gettext("TCP port");?></td>
			      <td width="78%" class="vtable">
							<input name="port" type="text" class="formfld" id="port" size="20" value="<?=htmlspecialchars($pconfig['port']);?>" />
							<br /><?=gettext("Alternate TCP port. Default is 22");?></td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Permit root login");?></td>
			      <td width="78%" class="vtable">
			        <input name="permitrootlogin" type="checkbox" id="permitrootlogin" value="yes" <?php if ($pconfig['permitrootlogin']) echo "checked=\"checked\""; ?> />
			        <?=gettext("Specifies whether it is allowed to login as superuser (root) directly.");?></td>
			    </tr>
					<tr>
						<td width="22%" valign="top" class="vncell"><?=gettext("Password authentication");?></td>
						<td width="78%" class="vtable">
							<input name="passwordauthentication" type="checkbox" id="passwordauthentication" value="yes" <?php if ($pconfig['passwordauthentication']) echo "checked=\"checked\""; ?> />
							<?=gettext("Enable keyboard-interactive authentication.");?></td>
					</tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("TCP forwarding");?></td>
			      <td width="78%" class="vtable">
			        <input name="tcpforwarding" type="checkbox" id="tcpforwarding" value="yes" <?php if ($pconfig['tcpforwarding']) echo "checked=\"checked\""; ?> />
			        <?=gettext("Permit to do SSH Tunneling.");?></td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Compression");?></td>
			      <td width="78%" class="vtable">
			        <input name="compression" type="checkbox" id="compression" value="yes" <?php if ($pconfig['compression']) echo "checked=\"checked\""; ?> />
			        <?=gettext("Enable compression.");?><br />
			        <span class="vexpl"><?=gettext("Compression is worth using if your connection is slow. The efficiency of the compression depends on the type of the file, and varies widely. Useful for internet transfer only.");?></span></td>
			    </tr>
					<?php html_textarea("key", gettext("Private Key"), $pconfig['key'], gettext("Paste a DSA PRIVATE KEY in PEM format here."), false, 65, 7, false, false);?>
			    <?php html_inputbox("subsystem", gettext("Subsystem"), $pconfig['subsystem'], gettext("Leave this field empty to use default settings."), false, 40);?>
			    <?php html_textarea("auxparam", gettext("Extra options"), $pconfig['auxparam'], gettext("Extra options to /etc/ssh/sshd_config (usually empty). Note, incorrect entered options prevent SSH service to be started.") . " " . sprintf(gettext("Please check the <a href='%s' target='_blank'>documentation</a>."), "http://www.freebsd.org/cgi/man.cgi?query=sshd_config&amp;apropos=0&amp;sektion=0&amp;manpath=FreeBSD+${os_release}-RELEASE&amp;format=html"), false, 65, 5, false, false);?>
			  </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save and Restart");?>" onclick="enable_change(true)" />
				</div>
			</td>
		</tr>
	</table>
	<?php include("formend.inc");?>
</form>
<script type="text/javascript">
<!--
enable_change(false);
//-->
</script>
<?php include("fend.inc");?>
