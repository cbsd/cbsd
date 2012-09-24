#!/usr/local/bin/php
<?php
/*
	services_daap.php
	
	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of FreeNAS (http://www.freenas.org)
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
require("services.inc");

$pgtitle = array(gettext("Services"), gettext("iTunes/DAAP"));

if (!isset($config['daap']) || !is_array($config['daap']))
	$config['daap'] = array();

$pconfig['enable'] = isset($config['daap']['enable']);
$pconfig['servername'] = $config['daap']['servername'];
$pconfig['port'] = $config['daap']['port'];
$pconfig['dbdir'] = $config['daap']['dbdir'];
$pconfig['content'] = $config['daap']['content'];
$pconfig['rescaninterval'] = $config['daap']['rescaninterval'];
$pconfig['alwaysscan'] = isset($config['daap']['alwaysscan']);
$pconfig['scantype'] = $config['daap']['scantype'];
$pconfig['admin_pw'] = $config['daap']['admin_pw'];

// Set default values.
if (!$pconfig['servername']) $pconfig['servername'] = $config['system']['hostname'];
if (!$pconfig['port']) $pconfig['port'] = "3689";
if (!$pconfig['rescaninterval']) $pconfig['rescaninterval'] = "0";
if (!$pconfig['alwaysscan']) $pconfig['alwaysscan'] = false;
if (!$pconfig['scantype']) $pconfig['scantype'] = "0";

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	// Input validation.
	if ($_POST['enable']) {
		$reqdfields = explode(" ", "servername port dbdir content admin_pw");
		$reqdfieldsn = array(gettext("Server name"), gettext("Port"), gettext("Database directory"), gettext("Content"), gettext("Password"));
		$reqdfieldst = explode(" ", "string port string array password");

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

		$reqdfields = array_merge($reqdfields, array("rescaninterval"));
		$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Rescan interval")));
		$reqdfieldst = array_merge($reqdfieldst, array("numeric"));

		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

		// Check if port is already used.
		if (services_is_port_used($_POST['port'], "daap"))
			$input_errors[] = sprintf(gettext("Port %ld is already used by another service."), $_POST['port']);
	}

	if (!$input_errors) {
		$config['daap']['enable'] = $_POST['enable'] ? true : false;
		$config['daap']['servername'] = $_POST['servername'];
		$config['daap']['port'] = $_POST['port'];
		$config['daap']['dbdir'] = $_POST['dbdir'];
		$config['daap']['content'] = $_POST['content'];
		$config['daap']['rescaninterval'] = $_POST['rescaninterval'];
		$config['daap']['alwaysscan'] = $_POST['alwaysscan'] ? true : false;
		$config['daap']['scantype'] = $_POST['scantype'];
		$config['daap']['admin_pw'] = $_POST['admin_pw'];

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("mt-daapd");
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
	document.iform.servername.disabled = endis;
	document.iform.port.disabled = endis;
	document.iform.dbdir.disabled = endis;
	document.iform.dbdirbrowsebtn.disabled = endis;
	document.iform.content.disabled = endis;
	document.iform.contentaddbtn.disabled = endis;
	document.iform.contentchangebtn.disabled = endis;
	document.iform.contentdeletebtn.disabled = endis;
	document.iform.contentdata.disabled = endis;
	document.iform.contentbrowsebtn.disabled = endis;
	document.iform.rescaninterval.disabled = endis;
	document.iform.alwaysscan.disabled = endis;
	document.iform.scantype.disabled = endis;
	document.iform.admin_pw.disabled = endis;
}
//-->
</script>
<form action="services_daap.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
		<tr>
			<td class="tabcont">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<?php if (!isset($config['system']['zeroconf'])) print_error_box(sprintf(gettext("You have to activate <a href='%s'>Zeroconf/Bonjour</a> to advertise this service to clients."), "system_advanced.php"));?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gettext("Digital Audio Access Protocol"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
					<?php html_inputbox("servername", gettext("Server name"), $pconfig['servername'], gettext("This is both the name of the server as advertised via Zeroconf/Bonjour/Rendezvous, and the name of the database exported via DAAP."), true, 20);?>
					<?php html_inputbox("port", gettext("Port"), $pconfig['port'], gettext("Port to listen on. Default iTunes port is 3689."), true, 5);?>
					<?php html_filechooser("dbdir", gettext("Database directory"), $pconfig['dbdir'], gettext("Location where the content database file will be stored."), $g['media_path'], true, 60);?>
					<?php html_folderbox("content", gettext("Content"), $pconfig['content'], gettext("Location of the files to share."), $g['media_path'], true);?>
					<?php html_inputbox("rescaninterval", gettext("Rescan interval"), $pconfig['rescaninterval'], gettext("Scan file system every N seconds to see if any files have been added or removed. Set to 0 to disable background scanning. If background rescanning is disabled, a scan can still be forced from the status page of the administrative web interface."), false, 5);?>
					<?php html_checkbox("alwaysscan", gettext("Always scan"), $pconfig['alwaysscan'] ? true : false, "", gettext("Whether scans should be skipped if there are no users connected. This allows the drive to spin down when no users are connected."), false);?>
					<?php html_combobox("scantype", gettext("Scan type"), $pconfig['scantype'], array("0" => gettext("Normal"), "1" => gettext("Aggressive"), "2" => gettext("Painfully aggressive")), "", false);?>
					<?php html_separator();?>
					<?php html_titleline(gettext("Administrative WebGUI"));?>
					<?php html_passwordbox("admin_pw", gettext("Password"), $pconfig['admin_pw'], sprintf("%s %s", gettext("Password for the administrative pages."), gettext("Default user name is 'admin'.")), true, 20);?>
					<?php
					$if = get_ifname($config['interfaces']['lan']['if']);
					$ipaddr = get_ipaddr($if);
					$url = htmlspecialchars("http://{$ipaddr}:{$pconfig['port']}");
					$text = "<a href='{$url}' target='_blank'>{$url}</a>";
					?>
					<?php html_text("url", gettext("URL"), $text);?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save and Restart");?>" onclick="onsubmit_content(); enable_change(true)" />
				</div>
				<div id="remarks">
					<?php html_remark("note", gettext("Note"), sprintf(gettext("You have to activate <a href='%s'>Zeroconf/Bonjour</a> to advertise this service to clients."), "system_advanced.php"));?>
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
