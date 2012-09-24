#!/usr/local/bin/php
<?php
/*
	acces_ldap.php
	Part of NAS4Free (http://www.nas4free.org)
	Copyright (C) 2012 by nas4free team <info@nas4free.org>.
	All rights reserved.

	portions of FreeNAS (http://www.freenas.org)
	Copyright (C) 2005-2010 Olivier Cochard-Labbe <olivier@freenas.org>.
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

$pgtitle = array(gettext("Access"), gettext("LDAP"));

if (!isset($config['ldap']) || !is_array($config['ldap'])) {
	$config['ldap'] = array();
}

if (!isset($config['samba']) || !is_array($config['samba'])) {
	$config['samba'] = array();

}

#LDAP take priority over MS ActiveDirectory (NAS4Free choicee), then disable AD:
if (!is_array($config['ad'])) {
	$config['ad'] = array();
}

$pconfig['enable'] = isset($config['ldap']['enable']);
$pconfig['hostname'] = $config['ldap']['hostname'];
$pconfig['base'] = $config['ldap']['base'];
$pconfig['anonymousbind'] = isset($config['ldap']['anonymousbind']);
$pconfig['binddn'] = $config['ldap']['binddn'];
$pconfig['bindpw'] = $config['ldap']['bindpw'];
$pconfig['bindpw2'] = $config['ldap']['bindpw'];
$pconfig['rootbinddn'] = $config['ldap']['rootbinddn'];
$pconfig['rootbindpw'] = $config['ldap']['rootbindpw'];
$pconfig['rootbindpw2'] = $config['ldap']['rootbindpw'];
$pconfig['user_suffix'] = $config['ldap']['user_suffix'];
$pconfig['password_suffix'] = $config['ldap']['password_suffix'];
$pconfig['group_suffix'] = $config['ldap']['group_suffix'];
$pconfig['machine_suffix'] = $config['ldap']['machine_suffix'];
$pconfig['pam_password'] = $config['ldap']['pam_password'];
if (is_array($config['ldap']['auxparam']))
	$pconfig['auxparam'] = implode("\n", $config['ldap']['auxparam']);

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	// Input validation.
	if ($_POST['enable']) {
		$reqdfields = explode(" ", "hostname base rootbinddn rootbindpw user_suffix group_suffix password_suffix machine_suffix");
		$reqdfieldsn = array(gettext("Host name"), gettext("Base DN"), gettext("Root bind DN"), gettext("Root bind password"), gettext("User suffix"), gettext("Group suffix"), gettext("Password suffix"), gettext("Machine suffix"));
		$reqdfieldst = explode(" ", "string string string password string string string string");

		if (!$_POST['anonymousbind']) {
			$reqdfields = array_merge($reqdfields, explode(" ", "binddn bindpw"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Bind DN"), gettext("Bind password")));
			$reqdfieldst = array_merge($reqdfieldst, explode(" ", "string password"));
		}

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);
	}

	if (($_POST['bindpw'] !== $_POST['bindpw2'])) {
		$input_errors[] = gettext("The confimed password does not match. Please ensure the passwords match exactly.");
	}

	if (!$input_errors) {
		$config['ldap']['enable'] = $_POST['enable'] ? true : false;
		$config['ldap']['hostname'] = $_POST['hostname'];
		$config['ldap']['base'] = $_POST['base'];
		$config['ldap']['anonymousbind'] = $_POST['anonymousbind'] ? true : false;
		$config['ldap']['binddn'] = $_POST['binddn'];
		$config['ldap']['bindpw'] = $_POST['bindpw'];
		$config['ldap']['rootbinddn'] = $_POST['rootbinddn'];
		$config['ldap']['rootbindpw'] = $_POST['rootbindpw'];
		$config['ldap']['user_suffix'] = $_POST['user_suffix'];
		$config['ldap']['password_suffix'] = $_POST['password_suffix'];
		$config['ldap']['group_suffix'] = $_POST['group_suffix'];
		$config['ldap']['machine_suffix'] = $_POST['machine_suffix'];
		$config['ldap']['pam_password'] = $_POST['pam_password'];

		# Write additional parameters.
		unset($config['ldap']['auxparam']);
		foreach (explode("\n", $_POST['auxparam']) as $auxparam) {
			$auxparam = trim($auxparam, "\t\n\r");
			if (!empty($auxparam))
				$config['ldap']['auxparam'][] = $auxparam;
		}

		// Disable AD
		if ($config['ldap']['enable']) {
			$config['samba']['security'] = "user";
			$config['ad']['enable'] = false;
		}

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			rc_exec_service("pam");
			rc_exec_service("ldap");
			rc_start_service("nsswitch");
			rc_update_service("samba");
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
	}
}
?>
<?php include("fbegin.inc"); ?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	var endis = !(document.iform.enable.checked || enable_change);
	document.iform.hostname.disabled = endis;
	document.iform.base.disabled = endis;
	document.iform.anonymousbind.disabled = endis;
	document.iform.binddn.disabled = endis;
	document.iform.bindpw.disabled = endis;
	document.iform.bindpw2.disabled = endis;
	document.iform.rootbinddn.disabled = endis;
	document.iform.rootbindpw.disabled = endis;
	document.iform.rootbindpw2.disabled = endis;
	document.iform.user_suffix.disabled = endis;
	document.iform.password_suffix.disabled = endis;
	document.iform.group_suffix.disabled = endis;
	document.iform.machine_suffix.disabled = endis;
	document.iform.pam_password.disabled = endis;
	document.iform.auxparam.disabled = endis;
}

function anonymousbind_change() {
	switch (document.iform.anonymousbind.checked) {
		case false:
			showElementById('binddn_tr','show');
			showElementById('bindpw_tr','show');
			break;

		case true:
			showElementById('binddn_tr','hide');
			showElementById('bindpw_tr','hide');
			break;
	}
}
//-->
</script>
<form action="access_ldap.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
		<tr>
			<td class="tabcont">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gettext("Lightweight Directory Access Protocol"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
					<?php html_inputbox("hostname", gettext("Host name"), $pconfig['hostname'], gettext("The name or IP address of the LDAP server."), true, 20);?>
					<?php html_inputbox("base", gettext("Base DN"), $pconfig['base'], sprintf(gettext("The default base distinguished name (DN) to use for searches, e.g. %s"), "dc=test,dc=org"), true, 40);?>
					<?php html_checkbox("anonymousbind", gettext("Anonymous bind"), $pconfig['anonymousbind'] ? true : false, gettext("Enable anonymous bind."), "", true, "anonymousbind_change()");?>
					<?php html_inputbox("binddn", gettext("Bind DN"), $pconfig['binddn'], sprintf(gettext("The distinguished name with which to bind to the directory server, e.g. %s"), "cn=admin,dc=test,dc=org"), true, 40);?>
					<?php html_passwordconfbox("bindpw", "bindpw2", gettext("Bind password"), $pconfig['bindpw'], $pconfig['bindpw2'], gettext("The cleartext credentials with which to bind."), true);?>
					<?php html_inputbox("rootbinddn", gettext("Root bind DN"), $pconfig['rootbinddn'], sprintf(gettext("The distinguished name with which to bind to the directory server, e.g. %s"), "cn=admin,dc=test,dc=org"), true, 40);?>
					<?php html_passwordconfbox("rootbindpw", "rootbindpw2", gettext("Root bind password"), $pconfig['rootbindpw'], $pconfig['rootbindpw2'], gettext("The credentials with which to bind."), true);?>
					<?php html_combobox("pam_password", gettext("Password encryption"), $pconfig['pam_password'], array("clear" => "clear", "crypt" => "crypt", "md5" => "md5", "nds" => "nds", "racf" => "racf", "ad" => "ad", "exop" => "exop"), gettext("The password change protocol to use."), true);?>
					<?php html_inputbox("user_suffix", gettext("User suffix"), $pconfig['user_suffix'], sprintf(gettext("This parameter specifies the suffix that is used for users when these are added to the LDAP directory, e.g. %s"), "ou=Users"), true, 20);?>
					<?php html_inputbox("group_suffix", gettext("Group suffix"), $pconfig['group_suffix'], sprintf(gettext("This parameter specifies the suffix that is used for groups when these are added to the LDAP directory, e.g. %s"), "ou=Groups"), true, 20);?>
					<?php html_inputbox("password_suffix", gettext("Password suffix"), $pconfig['password_suffix'], sprintf(gettext("This parameter specifies the suffix that is used for passwords when these are added to the LDAP directory, e.g. %s"), "ou=Users"), true, 20);?>
					<?php html_inputbox("machine_suffix", gettext("Machine suffix"), $pconfig['machine_suffix'], sprintf(gettext("This parameter specifies the suffix that is used for machines when these are added to the LDAP directory, e.g. %s"), "ou=Computers"), true, 20);?>
					<?php html_textarea("auxparam", gettext("Auxiliary parameters"), $pconfig['auxparam'], sprintf(gettext("These parameters are added to %s."), "ldap.conf"), false, 65, 5, false, false);?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>" onclick="enable_change(true)" />
				</div>
			</td>
		</tr>
	</table>
	<?php include("formend.inc");?>
</form>
<script type="text/javascript">
<!--
anonymousbind_change();
enable_change(false);
//-->
</script>
<?php include("fend.inc");?>
