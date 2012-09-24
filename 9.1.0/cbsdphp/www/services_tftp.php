#!/usr/local/bin/php
<?php
/*
	services_tftp.php

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

$pgtitle = array(gettext("Services"), gettext("TFTP"));

if (!isset($config['tftpd']) || !is_array($config['tftpd']))
	$config['tftpd'] = array();

$pconfig['enable'] = isset($config['tftpd']['enable']);
$pconfig['dir'] = $config['tftpd']['dir'];
$pconfig['allowfilecreation'] = isset($config['tftpd']['allowfilecreation']);
$pconfig['port'] = $config['tftpd']['port'];
$pconfig['username'] = $config['tftpd']['username'];
$pconfig['umask'] = $config['tftpd']['umask'];
$pconfig['timeout'] = $config['tftpd']['timeout'];
$pconfig['maxblocksize'] = $config['tftpd']['maxblocksize'];
$pconfig['extraoptions'] = $config['tftpd']['extraoptions'];

// Set defaults
if (empty($pconfig['username']))
	$pconfig['username'] = "nobody";

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	// Input validation.
	if ($_POST['enable']) {
		$reqdfields = explode(" ", "dir");
		$reqdfieldsn = array(gettext("Directory"));
		$reqdfieldst = explode(" ", "string");

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

		$reqdfields = array_merge($reqdfields, explode(" ", "port umask timeout maxblocksize"));
		$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Port"), gettext("umask"), gettext("Timeout"), gettext("Max. block size")));
		$reqdfieldst = array_merge($reqdfieldst, explode(" ", "port numeric numeric numeric"));

		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);
	}

	if ($_POST['maxblocksize'] && ((512 > $_POST['maxblocksize']) || (65464 < $_POST['maxblocksize']))) {
		$input_errors[] = sprintf(gettext("Invalid max. block size! It must be in the range from %d to %d."), 512, 65464);
	}

	if (!$input_errors) {
		$config['tftpd']['enable'] = $_POST['enable'] ? true : false;
		$config['tftpd']['dir'] = $_POST['dir'];
		$config['tftpd']['allowfilecreation'] = $_POST['allowfilecreation'] ? true : false;
		$config['tftpd']['port'] = $_POST['port'];
		$config['tftpd']['username'] = $_POST['username'];
		$config['tftpd']['umask'] = $_POST['umask'];
		$config['tftpd']['timeout'] = $_POST['timeout'];
		$config['tftpd']['maxblocksize'] = $_POST['maxblocksize'];
		$config['tftpd']['extraoptions'] = $_POST['extraoptions'];

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("tftpd");
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
	document.iform.dir.disabled = endis;
	document.iform.dirbrowsebtn.disabled = endis;
	document.iform.allowfilecreation.disabled = endis;
	document.iform.port.disabled = endis;
	document.iform.username.disabled = endis;
	document.iform.umask.disabled = endis;
	document.iform.timeout.disabled = endis;
	document.iform.maxblocksize.disabled = endis;
	document.iform.extraoptions.disabled = endis;
}
//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabcont">
			<form action="services_tftp.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gettext("Trivial File Transfer Protocol"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
					<?php html_filechooser("dir", gettext("Directory"), $pconfig['dir'], gettext("The directory containing the files you want to publish. The remote host does not need to pass along the directory as part of the transfer."), $g['media_path'], true, 60);?>
					<?php html_checkbox("allowfilecreation", gettext("Allow new files"), $pconfig['allowfilecreation'] ? true : false, gettext("Allow new files to be created."), gettext("By default, only already existing files can be uploaded."), false);?>
					<?php html_separator();?>
					<?php html_titleline(gettext("Advanced settings"));?>
					<?php html_inputbox("port", gettext("Port"), $pconfig['port'], gettext("The port to listen to. The default is to listen to the tftp port specified in /etc/services."), false, 5);?>
					<?php $a_user = array(); foreach (system_get_user_list() as $userk => $userv) { $a_user[$userk] = htmlspecialchars($userk); }?>
					<?php html_combobox("username", gettext("Username"), $pconfig['username'], $a_user, gettext("Specifies the username which the service will run as."), false);?>
					<?php html_inputbox("umask", gettext("umask"), $pconfig['umask'], gettext("Sets the umask for newly created files to the specified value. The default is zero (anyone can read or write)."), false, 4);?>
					<?php html_inputbox("timeout", gettext("Timeout"), $pconfig['timeout'], gettext("Determine the default timeout, in microseconds, before the first packet is retransmitted. The default is 1000000 (1 second)."), false, 10);?>
					<?php html_inputbox("maxblocksize", gettext("Max. block size"), $pconfig['maxblocksize'], gettext("Specifies the maximum permitted block size. The permitted range for this parameter is from 512 to 65464."), false, 5);?>
					<?php html_inputbox("extraoptions", gettext("Extra options"), $pconfig['extraoptions'], gettext("Extra options (usually empty)."), false, 40);?>
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
enable_change();
//-->
</script>
<?php include("fend.inc");?>
