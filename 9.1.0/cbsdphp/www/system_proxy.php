#!/usr/local/bin/php
<?php
/*
	system_proxy.php

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

$pgtitle = array(gettext("System"),gettext("Advanced"),gettext("Proxy"));

if (!isset($config['system']['proxy']['http']) || !is_array($config['system']['proxy']['http']))
	$config['system']['proxy']['http'] = array();

if (!isset($config['system']['proxy']['ftp']) || !is_array($config['system']['proxy']['ftp']))
	$config['system']['proxy']['ftp'] = array();

$pconfig['http_enable'] = isset($config['system']['proxy']['http']['enable']);
$pconfig['http_address'] = $config['system']['proxy']['http']['address'];
$pconfig['http_port'] = $config['system']['proxy']['http']['port'];
$pconfig['http_auth'] = isset($config['system']['proxy']['http']['auth']);
$pconfig['http_username'] = $config['system']['proxy']['http']['username'];
$pconfig['http_password'] = $config['system']['proxy']['http']['password'];

$pconfig['ftp_enable'] = isset($config['system']['proxy']['ftp']['enable']);
$pconfig['ftp_address'] = $config['system']['proxy']['ftp']['address'];
$pconfig['ftp_port'] = $config['system']['proxy']['ftp']['port'];
$pconfig['ftp_auth'] = isset($config['system']['proxy']['ftp']['auth']);
$pconfig['ftp_username'] = $config['system']['proxy']['ftp']['username'];
$pconfig['ftp_password'] = $config['system']['proxy']['ftp']['password'];

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	$reqdfields = array();
	$reqdfieldsn = array();
	$reqdfieldst = array();

	if ($_POST['http_enable']) {
		$reqdfields = array_merge($reqdfields, explode(" ", "http_address http_port"));
		$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Address"),gettext("Port")));
		$reqdfieldst = array_merge($reqdfieldst,array("string","numeric"));

		if ($_POST['http_auth']) {
			$reqdfields = array_merge($reqdfields, explode(" ", "http_username http_password"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("User"),gettext("Password")));
			$reqdfieldst = array_merge($reqdfieldst,array("string","password"));
		}
	}

	if ($_POST['ftp_enable']) {
		$reqdfields = array_merge($reqdfields, explode(" ", "ftp_address ftp_port"));
		$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Address"),gettext("Port")));
		$reqdfieldst = array_merge($reqdfieldst,array("string","numeric"));

		if ($_POST['ftp_auth']) {
			$reqdfields = array_merge($reqdfields, explode(" ", "ftp_username ftp_password"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("User"),gettext("Password")));
			$reqdfieldst = array_merge($reqdfieldst,array("string","password"));
		}
	}

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if ($_POST['http_auth']) {
		if (($_POST['password'] && !is_validpassword($_POST['password']))) {
			$input_errors[] = gettext("The password contains the illegal character ':'.");
		}
	}

	if (!$input_errors) {
		$config['system']['proxy']['http']['enable'] = $pconfig['http_enable'] ? true : false;
		$config['system']['proxy']['http']['address'] = $pconfig['http_address'];
		$config['system']['proxy']['http']['port'] = $pconfig['http_port'];
		$config['system']['proxy']['http']['auth'] = $pconfig['http_auth'] ? true : false;
		$config['system']['proxy']['http']['username'] = $pconfig['http_username'];
		$config['system']['proxy']['http']['password'] = $pconfig['http_password'];

		$config['system']['proxy']['ftp']['enable'] = $pconfig['ftp_enable'] ? true : false;
		$config['system']['proxy']['ftp']['address'] = $pconfig['ftp_address'];
		$config['system']['proxy']['ftp']['port'] = $pconfig['ftp_port'];
		$config['system']['proxy']['ftp']['auth'] = $pconfig['ftp_auth'] ? true : false;
		$config['system']['proxy']['ftp']['username'] = $pconfig['ftp_username'];
		$config['system']['proxy']['ftp']['password'] = $pconfig['ftp_password'];

		write_config();
		touch($d_sysrebootreqd_path);
	}
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	if (enable_change.name == "http_enable") {
		var endis = !enable_change.checked;
		document.iform.http_address.disabled = endis;
		document.iform.http_port.disabled = endis;
		document.iform.http_auth.disabled = endis;
		document.iform.http_username.disabled = endis;
		document.iform.http_password.disabled = endis;
	} else if (enable_change.name == "ftp_enable") {
		var endis = !enable_change.checked;
		document.iform.ftp_address.disabled = endis;
		document.iform.ftp_port.disabled = endis;
		document.iform.ftp_auth.disabled = endis;
		document.iform.ftp_username.disabled = endis;
		document.iform.ftp_password.disabled = endis;
	} else {
		var endis = !(document.iform.http_enable.checked || enable_change);
		document.iform.http_address.disabled = endis;
		document.iform.http_port.disabled = endis;
		document.iform.http_auth.disabled = endis;
		document.iform.http_username.disabled = endis;
		document.iform.http_password.disabled = endis;

		endis = !(document.iform.ftp_enable.checked || enable_change);
		document.iform.ftp_address.disabled = endis;
		document.iform.ftp_port.disabled = endis;
		document.iform.ftp_auth.disabled = endis;
		document.iform.ftp_username.disabled = endis;
		document.iform.ftp_password.disabled = endis;
	}
}

function proxy_auth_change() {
	switch(document.iform.http_auth.checked) {
		case false:
      showElementById('http_username_tr','hide');
  		showElementById('http_password_tr','hide');
      break;

    case true:
      showElementById('http_username_tr','show');
  		showElementById('http_password_tr','show');
      break;
	}

	switch(document.iform.ftp_auth.checked) {
		case false:
      showElementById('ftp_username_tr','hide');
  		showElementById('ftp_password_tr','hide');
      break;

    case true:
      showElementById('ftp_username_tr','show');
  		showElementById('ftp_password_tr','show');
      break;
	}
}
//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
      	<li class="tabinact"><a href="system_advanced.php"><span><?=gettext("Advanced");?></span></a></li>
      	<li class="tabinact"><a href="system_email.php"><span><?=gettext("Email");?></span></a></li>
      	<li class="tabact"><a href="system_proxy.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Proxy");?></span></a></li>
      	<li class="tabinact"><a href="system_swap.php"><span><?=gettext("Swap");?></span></a></li>
      	<li class="tabinact"><a href="system_rc.php"><span><?=gettext("Command scripts");?></span></a></li>
        <li class="tabinact"><a href="system_cron.php"><span><?=gettext("Cron");?></span></a></li>
        <li class="tabinact"><a href="system_rcconf.php"><span><?=gettext("rc.conf");?></span></a></li>
        <li class="tabinact"><a href="system_sysctl.php"><span><?=gettext("sysctl.conf");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
    	<form action="system_proxy.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<?php if (file_exists($d_sysrebootreqd_path)) print_info_box(get_std_save_message(0));?>
			  <table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("http_enable", gettext("HTTP Proxy"), $pconfig['http_enable'] ? true : false, gettext("Enable"), "enable_change(this)");?>
          <?php html_inputbox("http_address", gettext("Address"), $pconfig['http_address'], "", true, 40);?>
          <?php html_inputbox("http_port", gettext("Port"), $pconfig['http_port'], "", true, 10);?>
					<?php html_checkbox("http_auth", gettext("Authentication"), $pconfig['http_auth'] ? true : false, gettext("Enable proxy authentication."), "", false, "proxy_auth_change()");?>
          <?php html_inputbox("http_username", gettext("User"), $pconfig['http_username'], "", true, 20);?>
			    <?php html_inputbox("http_password", gettext("Password"), $pconfig['http_password'], "", true, 20);?>
					<?php html_separator();?>
					<?php html_titleline_checkbox("ftp_enable", gettext("FTP Proxy"), $pconfig['ftp_enable'] ? true : false, gettext("Enable"), "enable_change(this)");?>
          <?php html_inputbox("ftp_address", gettext("Address"), $pconfig['ftp_address'], "", true, 40);?>
          <?php html_inputbox("ftp_port", gettext("Port"), $pconfig['ftp_port'], "", true, 10);?>
          <?php html_checkbox("ftp_auth", gettext("Authentication"), $pconfig['ftp_auth'] ? true : false, gettext("Enable proxy authentication."), "", false, "proxy_auth_change()");?>
          <?php html_inputbox("ftp_username", gettext("User"), $pconfig['ftp_username'], "", true, 20);?>
			    <?php html_inputbox("ftp_password", gettext("Password"), $pconfig['ftp_password'], "", true, 20);?>
			  </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>" onclick="enable_change(true)" />
			  </div>
			  <div id="remarks">
			  	<?php html_remark("note", gettext("Note"), gettext("If the server is behind a proxy set this parameters to give local services access to the internet via proxy."));?>
			  </div>
			  <?php include("formend.inc");?>
			</form>
		</td>
  </tr>
</table>
<script type="text/javascript">
<!--
proxy_auth_change();
enable_change(false);
//-->
</script>
<?php include("fend.inc");?>
