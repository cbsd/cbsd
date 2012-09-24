#!/usr/local/bin/php
<?php
/*
	system_swap.php

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

$pgtitle = array(gettext("System"), gettext("Advanced"), gettext("Swap"));

$pconfig['enable'] = isset($config['system']['swap']['enable']);
$pconfig['type'] = $config['system']['swap']['type'];
$pconfig['mountpoint'] = $config['system']['swap']['mountpoint'];
$pconfig['devicespecialfile'] = $config['system']['swap']['devicespecialfile'];
$pconfig['size'] = $config['system']['swap']['size'];

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['enable']) {
		$reqdfields = explode(" ", "type");
		$reqdfieldsn = array(gettext("Type"));
		$reqdfieldst = explode(" ", "string");

		if ("device" === $_POST['type']) {
			$reqdfields = array_merge($reqdfields, explode(" ", "devicespecialfile"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Device")));
			$reqdfieldst = array_merge($reqdfieldst, explode(" ", "string"));
		} else {
			$reqdfields = array_merge($reqdfields, explode(" ", "mountpoint size"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Mount point"), gettext("Size")));
			$reqdfieldst = array_merge($reqdfieldst, explode(" ", "string numeric"));
		}

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);
	}

	if (!$input_errors) {
		$config['system']['swap']['enable'] = $_POST['enable'] ? true : false;
		$config['system']['swap']['type'] = $_POST['type'];
		$config['system']['swap']['mountpoint'] = $_POST['mountpoint'];
		$config['system']['swap']['devicespecialfile'] = $_POST['devicespecialfile'];
		$config['system']['swap']['size'] = $_POST['size'];

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("swap");
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
	document.iform.type.disabled = endis;
	document.iform.mountpoint.disabled = endis;
	document.iform.size.disabled = endis;
	document.iform.devicespecialfile.disabled = endis;
}

function type_change() {
	switch (document.iform.type.value) {
	case "file":
		showElementById('mountpoint_tr','show');
		showElementById('size_tr','show');
		showElementById('devicespecialfile_tr','hide');
		break;

	case "device":
		showElementById('mountpoint_tr','hide');
		showElementById('size_tr','hide');
		showElementById('devicespecialfile_tr','show');
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
				<li class="tabinact"><a href="system_proxy.php"><span><?=gettext("Proxy");?></span></a></li>
				<li class="tabact"><a href="system_swap.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Swap");?></span></a></li>
				<li class="tabinact"><a href="system_rc.php"><span><?=gettext("Command scripts");?></span></a></li>
				<li class="tabinact"><a href="system_cron.php"><span><?=gettext("Cron");?></span></a></li>
				<li class="tabinact"><a href="system_rcconf.php"><span><?=gettext("rc.conf");?></span></a></li>
				<li class="tabinact"><a href="system_sysctl.php"><span><?=gettext("sysctl.conf");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="system_swap.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors); ?>
				<?php if ($savemsg) print_info_box($savemsg); ?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gettext("Swap memory"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
					<?php html_combobox("type", gettext("Type"), $pconfig['type'], array("file" => gettext("File"), "device" => gettext("Device")), "", true, false, "type_change()");?>
					<?php html_mountcombobox("mountpoint", gettext("Mount point"), $pconfig['mountpoint'], gettext("Select mount point where to create the swap file."), true);?>
					<?php html_inputbox("size", gettext("Size"), $pconfig['size'], gettext("The size of the swap file in MB."), true, 10);?>
					<?php html_inputbox("devicespecialfile", gettext("Device"), $pconfig['devicespecialfile'], sprintf(gettext("Name of the device to use as swap device, e.g. %s."), "/dev/ad0s3"), true, 20);?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>" onclick="enable_change(true)" />
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<script type="text/javascript">
<!--
enable_change(false);
type_change(false);
//-->
</script>
<?php include("fend.inc");?>
