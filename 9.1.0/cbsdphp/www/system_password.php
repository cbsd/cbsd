#!/usr/local/bin/php
<?php
/*
	system_password.php

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

$pgtitle = array(gettext("System"),gettext("General"),gettext("Password"));

if ($_POST) {
	unset($input_errors);

	$reqdfields = explode(" ", "password_old password_new password_confirm");
	$reqdfieldsn = array(gettext("Old password"), gettext("Password"), gettext("Password (confirmed)"));
	$reqdfieldst = explode(" ", "password password password");

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	// Validate old password.
	if ($_POST['password_old'] !== $config['system']['password']) {
		$input_errors[] = gettext("The old password is not correct.");
	}

	// Validate new password.
	if ($_POST['password_new'] !== $_POST['password_confirm']) {
		$input_errors[] = gettext("The confimed password does not match. Please ensure the passwords match exactly.");
	}

	// Check Webserver document root if auth is required
	if (isset($config['websrv']['enable'])
	    && isset($config['websrv']['authentication']['enable'])
	    && !is_dir($config['websrv']['documentroot'])) {
		$input_errors[] = gettext("Webserver document root is missing.");
	}

	if (!$input_errors) {
		$config['system']['password'] = $_POST['password_new'];

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_exec_service("userdb");
			$retval |= rc_exec_service("htpasswd");
			$retval |= rc_exec_service("websrv_htpasswd");
			$retval |= rc_exec_service("webfm");
			config_unlock();
		}

		$savemsg = get_std_save_message($retval);
	}
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
      	<li class="tabinact"><a href="system.php"><span><?=gettext("General");?></span></a></li>
      	<li class="tabact"><a href="system_password.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Password");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
			<form action="system_password.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_passwordbox("password_old", gettext("Old password"), "", "", true);?>
					<?php html_passwordconfbox("password_new", "password_confirm", gettext("Password"), "", "", gettext("If you want to change the password for accessing the WebGUI, enter it here twice."), true);?>
			  </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>" />
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
  </tr>
</table>
<?php include("fend.inc");?>
