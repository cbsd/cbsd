#!/usr/local/bin/php
<?php
/*
	access_users_edit.php
	
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

$uuid = $_GET['uuid'];
if (isset($_POST['uuid']))
	$uuid = $_POST['uuid'];

$pgtitle = array(gettext("Access"), gettext("Users"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['access']['user']) || !is_array($config['access']['user']))
	$config['access']['user'] = array();

array_sort_key($config['access']['user'], "login");
$a_user = &$config['access']['user'];
$a_user_system = system_get_user_list();
$a_group = system_get_group_list();

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_user, "uuid")))) {
	$pconfig['uuid'] = $a_user[$cnid]['uuid'];
	$pconfig['login'] = $a_user[$cnid]['login'];
	$pconfig['fullname'] = $a_user[$cnid]['fullname'];
	$pconfig['password'] = $a_user[$cnid]['password'];
	$pconfig['passwordconf'] = $pconfig['password'];
	$pconfig['userid'] = $a_user[$cnid]['id'];
	$pconfig['primarygroup'] = $a_user[$cnid]['primarygroup'];
	$pconfig['group'] = $a_user[$cnid]['group'];
	$pconfig['shell'] = $a_user[$cnid]['shell'];
	$pconfig['homedir'] = $a_user[$cnid]['homedir'];
	$pconfig['userportal'] = isset($a_user[$cnid]['userportal']);
} else {
	$pconfig['uuid'] = uuid();
	$pconfig['primarygroup'] = $a_group['guest'];
	$pconfig['userid'] = get_nextuser_id();
	$pconfig['shell'] = "nologin";
	$pconfig['userportal'] = FALSE;
}

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: access_users.php");
		exit;
	}

	$reqdfields = explode(" ", "login fullname primarygroup userid shell");
	$reqdfieldsn = array(gettext("Name"), gettext("Full Name"), gettext("Primary Group"), gettext("User ID"), gettext("Shell"));
	$reqdfieldst = explode(" ", "string string numeric numeric string");

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	// Check for valid login name.
	if (($_POST['login'] && !is_validlogin($_POST['login']))) {
		$input_errors[] = gettext("The login name contains invalid characters.");
	}
	if (($_POST['login'] && strlen($_POST['login']) > 16)) {
		$input_errors[] = gettext("The login name is limited to 16 characters.");
	}

	if (($_POST['login'] && in_array($_POST['login'], $reservedlogin))) {
		$input_errors[] = gettext("The login name is a reserved login name.");
	}

	// Check for valid Full name.
	if (($_POST['fullname'] && !is_validdesc($_POST['fullname']))) {
		$input_errors[] = gettext("The full name contains invalid characters.");
	}

	// Check for name conflicts. Only check if user is created.
	if (!(isset($uuid) && (FALSE !== $cnid)) && ((is_array($a_user_system) && array_key_exists($_POST['login'], $a_user_system)) ||
		(false !== array_search_ex($_POST['login'], $a_user, "login")))) {
		$input_errors[] = gettext("This user already exists in the user list.");
	}

	// Check for a password mismatch.
	if ($_POST['password'] != $_POST['passwordconf']) {
		$input_errors[] = gettext("Password don't match.");
	}

	// Check if primary group is also selected in additional group.
	if (is_array($_POST['group']) && in_array($_POST['primarygroup'], $_POST['group'])) {
		$input_errors[] = gettext("Primary group is also selected in additional group.");
	}

	// Check additional group count. Max=15 (Primary+14) 
	if (is_array($_POST['group']) && count($_POST['group']) > 14) {
		$input_errors[] = gettext("There are too many additional groups.");
	}

	// Validate if ID is unique. Only check if user is created.
	if (!(isset($uuid) && (FALSE !== $cnid)) && (false !== array_search_ex($_POST['userid'], $a_user, "id"))) {
		$input_errors[] = gettext("The unique user ID is already used.");
	}

	// Check Webserver document root if auth is required
	if (isset($config['websrv']['enable'])
	    && isset($config['websrv']['authentication']['enable'])
	    && !is_dir($config['websrv']['documentroot'])) {
		$input_errors[] = gettext("Webserver document root is missing.");
	}

	if (!$input_errors) {
		$user = array();
		$user['uuid'] = $_POST['uuid'];
		$user['login'] = $_POST['login'];
		$user['fullname'] = $_POST['fullname'];
		$user['password'] = $_POST['password'];
		$user['shell'] = $_POST['shell'];
		$user['primarygroup'] = $_POST['primarygroup'];
		if (is_array($_POST['group']))
			$user['group'] = $_POST['group'];
		$user['homedir'] = $_POST['homedir'];
		$user['id'] = $_POST['userid'];
		$user['userportal'] = $_POST['userportal'] ? true : false;

		if (isset($uuid) && (FALSE !== $cnid)) {
			$a_user[$cnid] = $user;
			$mode = UPDATENOTIFY_MODE_MODIFIED;
		} else {
			$a_user[] = $user;
			$mode = UPDATENOTIFY_MODE_NEW;
		}

		updatenotify_set("userdb_user", $mode, $user['uuid']);
		write_config();

		header("Location: access_users.php");
		exit;
	}
}

// Get next user id.
// Return next free user id.
function get_nextuser_id() {
	global $config;

	// Get next free user id.
	exec("/usr/sbin/pw nextuser", $output);
	$output = explode(":", $output[0]);
	$id = intval($output[0]);

	// Check if id is already in usage. If the user did not press the 'Apply'
	// button 'pw' did not recognize that there are already several new users
	// configured because the user db is not updated until 'Apply' is pressed.
	$a_user = $config['access']['user'];
	if (false !== array_search_ex(strval($id), $a_user, "id")) {
		do {
			$id++; // Increase id until a unused one is found.
		} while (false !== array_search_ex(strval($id), $a_user, "id"));
	}

	return $id;
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav">
				<li class="tabact"><a href="access_users.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Users");?></span></a></li>
				<li class="tabinact"><a href="access_users_groups.php"><span><?=gettext("Groups");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="access_users_edit.php" method="post" name="iform" id="iform">
				<?php if ($nogroup_errors) print_input_errors($nogroup_errors); ?>
				<?php if ($input_errors) print_input_errors($input_errors); ?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_inputbox("login", gettext("Name"), $pconfig['login'], gettext("Login name of user."), true, 20, isset($uuid) && (FALSE !== $cnid));?>
					<?php html_inputbox("fullname", gettext("Full Name"), $pconfig['fullname'], gettext("User full name."), true, 20);?>
					<?php html_passwordconfbox("password", "passwordconf", gettext("Password"), $pconfig['password'], $pconfig['passwordconf'], gettext("User password."), true);?>
					<?php html_inputbox("userid", gettext("User ID"), $pconfig['userid'], gettext("User numeric id."), true, 20, isset($uuid) && (FALSE !== $cnid));?>
					<?php html_combobox("shell", gettext("Shell"), $pconfig['shell'], array("nologin" => "nologin", "scponly" => "scponly", "sh" => "sh",  "csh" => "csh", "tcsh" => "tcsh", "bash" => "bash"), gettext("The user's login shell."), true);?>
					<?php $grouplist = array(); foreach ($a_group as $groupk => $groupv) { $grouplist[$groupv] = $groupk; } ?>
					<?php html_combobox("primarygroup", gettext("Primary group"), $pconfig['primarygroup'], $grouplist, gettext("Set the account's primary group to the given group."), true);?>
					<?php html_listbox("group", gettext("Additional group"), $pconfig['group'], $grouplist, gettext("Set additional group memberships for this account.")."<br />".gettext("Note: Ctrl-click (or command-click on the Mac) to select and deselect groups."));?>
					<?php html_filechooser("homedir", gettext("Home directory"), $pconfig['homedir'], gettext("Enter the path to the home directory of that user. Leave this field empty to use default path /mnt."), $g['media_path'], false, 60);?>
					<?php html_checkbox("userportal", gettext("User portal"), $pconfig['userportal'] ? true : false, gettext("Grant access to the user portal."), "", false);?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=(isset($uuid) && (FALSE !== $cnid)) ? gettext("Save") : gettext("Add")?>" />
					<input name="Cancel" type="submit" class="formbtn" value="<?=gettext("Cancel");?>" />
					<input name="uuid" type="hidden" value="<?=$pconfig['uuid'];?>" />
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
