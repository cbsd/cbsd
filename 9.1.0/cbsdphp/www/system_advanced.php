#!/usr/local/bin/php
<?php
/*
	system_advanced.php

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

$pgtitle = array(gettext("System"), gettext("Advanced"));

$pconfig['disableconsolemenu'] = isset($config['system']['disableconsolemenu']);
$pconfig['disablefm'] = isset($config['system']['disablefm']);
$pconfig['disablefirmwarecheck'] = isset($config['system']['disablefirmwarecheck']);
$pconfig['disablebeep'] = isset($config['system']['disablebeep']);
$pconfig['tune_enable'] = isset($config['system']['tune']);
$pconfig['zeroconf'] = isset($config['system']['zeroconf']);
$pconfig['powerd'] = isset($config['system']['powerd']);
$pconfig['motd'] = base64_decode($config['system']['motd']);
$pconfig['sysconsaver'] = isset($config['system']['sysconsaver']['enable']);
$pconfig['sysconsaverblanktime'] = $config['system']['sysconsaver']['blanktime'];
$pconfig['enableserialconsole'] = isset($config['system']['enableserialconsole']);

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	// Input validation.
	if ($_POST['sysconsaver']) {
		$reqdfields = explode(" ", "sysconsaverblanktime");
		$reqdfieldsn = array(gettext("Blank time"));
		$reqdfieldst = explode(" ", "numeric");

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);
	}

	if (!$input_errors) {
		// Process system tuning.
		if ($_POST['tune_enable']) {
			sysctl_tune(1);
		} else if (isset($config['system']['tune']) && (!$_POST['tune_enable'])) {
			// Simply force a reboot to reset to default values.
			// This makes programming easy :-) Also we are sure that
			// system will use origin values (maybe default values
			// change from one FreeBSD release to the next. This will
			// reduce maintenance).
			sysctl_tune(0);
			touch($d_sysrebootreqd_path);
		}
		$bootconfig="boot.config";
		if (!isset($_POST['enableserialconsole'])) {
			if (file_exists("/$bootconfig")) {
				unlink("/$bootconfig");
			}
			if (file_exists("{$g['cf_path']}/mfsroot.gz")
			    && file_exists("{$g['cf_path']}/$bootconfig")) {
				config_lock();
				conf_mount_rw();
				unlink("{$g['cf_path']}/$bootconfig");
				conf_mount_ro();
				config_unlock();
			}
		} else {
			if (file_exists("/$bootconfig")) {
				unlink("/$bootconfig");
			}
			file_put_contents("/$bootconfig", "-Dh\n");
			if (file_exists("{$g['cf_path']}/mfsroot.gz")) {
				config_lock();
				conf_mount_rw();
				if (file_exists("{$g['cf_path']}/$bootconfig")) {
					unlink("{$g['cf_path']}/$bootconfig");
				}
				file_put_contents("{$g['cf_path']}/$bootconfig", "-Dh\n");
				conf_mount_ro();
				config_unlock();
			}
		}
		if ((isset($config['system']['disablefm']) && (!$_POST['disablefm']))
		    || (!isset($config['system']['disablefm']) && ($_POST['disablefm']))) {
			// need restarting WebGUI
			touch($d_sysrebootreqd_path);
		}

		$config['system']['disableconsolemenu'] = $_POST['disableconsolemenu'] ? true : false;
		$config['system']['disablefm'] = $_POST['disablefm'] ? true : false;
		$config['system']['disablefirmwarecheck'] = $_POST['disablefirmwarecheck'] ? true : false;
		$config['system']['webgui']['noantilockout'] = $_POST['noantilockout'] ? true : false;
		$config['system']['disablebeep'] = $_POST['disablebeep'] ? true : false;
		$config['system']['tune'] = $_POST['tune_enable'] ? true : false;
		$config['system']['zeroconf'] = $_POST['zeroconf'] ? true : false;
		$config['system']['powerd'] = $_POST['powerd'] ? true : false;
		$config['system']['motd'] = base64_encode($_POST['motd']); // Encode string, otherwise line breaks will get lost
		$config['system']['sysconsaver']['enable'] = $_POST['sysconsaver'] ? true : false;
		$config['system']['sysconsaver']['blanktime'] = $_POST['sysconsaverblanktime'];
		$config['system']['enableserialconsole'] = $_POST['enableserialconsole'] ? true : false;

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_exec_service("rcconf");
			$retval |= rc_update_service("powerd");
			$retval |= rc_update_service("mdnsresponder");
			$retval |= rc_exec_service("motd");
			if (isset($config['system']['tune']))
				$retval |= rc_update_service("sysctl");
			$retval |= rc_update_service("syscons");
			$retval |= rc_update_service("webfm");
			config_unlock();
		}

		$savemsg = get_std_save_message($retval);
	}
}

function sysctl_tune($mode) {
	global $config;

	if (!is_array($config['system']['sysctl']['param']))
		$config['system']['sysctl']['param'] = array();

	array_sort_key($config['system']['sysctl']['param'], "name");
	$a_sysctlvar = &$config['system']['sysctl']['param'];

	$a_mib = array(
		"net.inet.tcp.delayed_ack" => 0,
		"net.inet.tcp.rfc1323" => 1,
		"net.inet.tcp.sendspace" => 262144,
		"net.inet.tcp.recvspace" => 262144,
		"net.inet.tcp.sendbuf_max" => 4194304,
		"net.inet.tcp.sendbuf_inc" => 262144,
		"net.inet.tcp.sendbuf_auto" => 1,
		"net.inet.tcp.recvbuf_max" => 4194304,
		"net.inet.tcp.recvbuf_inc" => 262144,
		"net.inet.tcp.recvbuf_auto" => 1,
		"net.inet.udp.recvspace" => 65536,
		"net.inet.udp.maxdgram" => 57344,
		"net.local.stream.recvspace" => 65536,
		"net.local.stream.sendspace" => 65536,
		"kern.ipc.maxsockbuf" => 16777216,
		"kern.ipc.somaxconn" => 8192,
		"kern.ipc.nmbclusters" => 262144,
		"kern.ipc.nmbjumbop" => 262144,
		"kern.ipc.nmbjumbo9" => 131072,
		"kern.ipc.nmbjumbo16" => 65536,
		"kern.maxfiles" => 65536,
		"kern.maxfilesperproc" => 32768,
		"net.inet.icmp.icmplim" => 300,
		"net.inet.icmp.icmplim_output" => 1,
		//"net.inet.tcp.inflight.enable" => 0,
		"net.inet.tcp.path_mtu_discovery" => 0,
		"hw.intr_storm_threshold" => 9000,
	);

	switch ($mode) {
		case 0:
			// Remove system tune MIB's.
			while (list($name, $value) = each($a_mib)) {
				$id = array_search_ex($name, $a_sysctlvar, "name");
				if (false === $id)
					continue;
				unset($a_sysctlvar[$id]);
			}
			break;

		case 1:
			// Add system tune MIB's.
			while (list($name, $value) = each($a_mib)) {
				$id = array_search_ex($name, $a_sysctlvar, "name");
				if (false !== $id)
					continue;

				$param = array();
				$param['uuid'] = uuid();
				$param['name'] = $name;
				$param['value'] = $value;
				$param['comment'] = gettext("System tuning");
				$param['enable'] = true;

				$a_sysctlvar[] = $param;
			}
			break;
	}
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function sysconsaver_change() {
	switch (document.iform.sysconsaver.checked) {
		case true:
			showElementById('sysconsaverblanktime_tr','show');
			break;

		case false:
			showElementById('sysconsaverblanktime_tr','hide');
			break;
	}
}
//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav">
				<li class="tabact"><a href="system_advanced.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Advanced");?></span></a></li>
				<li class="tabinact"><a href="system_email.php"><span><?=gettext("Email");?></span></a></li>
				<li class="tabinact"><a href="system_proxy.php"><span><?=gettext("Proxy");?></span></a></li>
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
			<form action="system_advanced.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_checkbox("disableconsolemenu", gettext("Console menu"), $pconfig['disableconsolemenu'] ? true : false, gettext("Disable console menu"), gettext("Changes to this option will take effect after a reboot."));?>
					<?php html_checkbox("enableserialconsole", gettext("Serial Console"), $pconfig['enableserialconsole'] ? true : false, gettext("Enable serial console"), sprintf("<span class='red'><strong>%s</strong></span><br />%s", gettext("The COM port in BIOS has to be enabled before enabling this option."), gettext("Changes to this option will take effect after a reboot.")));?>
					<?php html_checkbox("sysconsaver", gettext("Console screensaver"), $pconfig['sysconsaver'] ? true : false, gettext("Enable console screensaver"), "", false, "sysconsaver_change()");?>
					<?php html_inputbox("sysconsaverblanktime", gettext("Blank time"), $pconfig['sysconsaverblanktime'], gettext("Turn the monitor to standby after N seconds."), true, 5);?>
					<?php html_checkbox("disablefm", gettext("File Manager"), $pconfig['disablefm'] ? true : false, gettext("Disable File Manager"));?>
					<?php if ("full" !== $g['platform']):?>
					<?php html_checkbox("disablefirmwarecheck", gettext("Firmware version check"), $pconfig['disablefirmwarecheck'] ? true : false, gettext("Disable firmware version check"), sprintf(gettext("This will cause %s not to check for newer firmware versions when the <a href='%s'>%s</a> page is viewed."), get_product_name(), "system_firmware.php", gettext("System").": ".gettext("Firmware")));?>
					<?php endif;?>
					<?php html_checkbox("disablebeep", gettext("System Beep"), $pconfig['disablebeep'] ? true : false, gettext("Disable speaker beep on startup and shutdown"));?>
					<?php html_checkbox("tune_enable", gettext("Tuning"), $pconfig['tune_enable'] ? true : false, gettext("Enable tuning of some kernel variables"));?>
					<?php html_checkbox("powerd", gettext("Power Daemon"), $pconfig['powerd'] ? true : false, gettext("Enable the system power control utility"), gettext("The powerd utility monitors the system state and sets various power control options accordingly."));?>
					<?php html_checkbox("zeroconf", gettext("Zeroconf/Bonjour"), $pconfig['zeroconf'] ? true : false, gettext("Enable Zeroconf/Bonjour to advertise services of this device"));?>
					<?php html_textarea("motd", gettext("MOTD"), $pconfig['motd'], gettext("Message of the day."), false, 65, 7, false, false);?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>" />
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<script type="text/javascript">
<!--
sysconsaver_change();
//-->
</script>
<?php include("fend.inc");?>
