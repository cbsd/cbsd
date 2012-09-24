#!/usr/local/bin/php
<?php
/*
	services_unison.php

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

*
	Unison Installation Notes

	To work, unison requires an environment variable UNISON to point at
	a writable directory. Unison keeps information there between syncs to
	speed up the process.

	When a user runs the unison client, it will try to invoke ssh to
	connect to the this server. Giving the local ssh a UNISON environment
	variable without compromising ssh turned out to be non-trivial.
	The solution is to modify the default path found in /etc/login.conf.
	The path is seeded with "UNISON=/mnt" and this updated by the
	/etc/rc.d/unison file.

	Todo:
	* 	Arguably, a full client install could be done too to
	allow NAS4Free to NAS4Free syncing.
*/
require("auth.inc");
require("guiconfig.inc");

$pgtitle = array(gettext("Services"), gettext("Unison"));

if (!isset($config['unison']) || !is_array($config['unison']))
	$config['unison'] = array();

$pconfig['enable'] = isset($config['unison']['enable']);
$pconfig['workdir'] = $config['unison']['workdir'];
$pconfig['mkdir'] = isset($config['unison']['mkdir']);

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	// Input validation
	$reqdfields = array();
	$reqdfieldsn = array();

	if ($_POST['enable']) {
		$reqdfields = array_merge($reqdfields, explode(" ", "workdir"));
		$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Working directory")));

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

		// Check if working directory exists
		if (!$_POST['mkdir'] && !file_exists($_POST['workdir'])) {
			$input_errors[] = gettext("The working directory does not exist.");
		}
	}

	if (!$input_errors) {
		$config['unison']['workdir'] = $_POST['workdir'];
		$config['unison']['enable'] = $_POST['enable'] ? true : false;
		$config['unison']['mkdir'] = $_POST['mkdir'] ? true : false;

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("unison");
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
	}
}

if (!isset($config['mounts']['mount']) || !is_array($config['mounts']['mount']))
	$config['mounts']['mount'] = array();

array_sort_key($config['mounts']['mount'], "devicespecialfile");
$a_mount = &$config['mounts']['mount'];

?>
<?php include("fbegin.inc"); ?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	var endis = !(document.iform.enable.checked || enable_change);
	document.iform.workdir.disabled = endis;
	document.iform.mkdir.disabled = endis;
}
//-->
</script>
<form action="services_unison.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
	  <tr>
	    <td class="tabcont">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>	    
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gettext("Unison File Synchronisation"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
					<?php html_filechooser("workdir", gettext("Working directory"), $pconfig['workdir'], sprintf(gettext("Location where the working files will be stored, e.g. %s/backup/.unison"), $g['media_path']), $g['media_path'], true, 60);?>
				  <?php html_checkbox("mkdir", "", $pconfig['mkdir'] ? true : false, gettext("Create work directory if it doesn't exist."), "", false);?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save and Restart");?>" onclick="enable_change(true)" />
				</div>
				<div id="remarks">
					<?php html_remark("note", gettext("Note"), sprintf(gettext("<a href='%s'>SSHD</a> must be enabled for Unison to work, and the <a href='%s'>user</a> must have shell access."), "services_sshd.php", "access_users.php"));?>
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
<?php include("fend.inc"); ?>
