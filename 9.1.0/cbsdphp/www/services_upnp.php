#!/usr/local/bin/php
<?php
/*
	services_upnp.php

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
require("services.inc");

$pgtitle = array(gettext("Services"),gettext("UPnP"));

if (!isset($config['upnp']) || !is_array($config['upnp']))
	$config['upnp'] = array();

if (!isset($config['upnp']['content']) || !is_array($config['upnp']['content']))
	$config['upnp']['content'] = array();

sort($config['upnp']['content']);

$pconfig['enable'] = isset($config['upnp']['enable']);
$pconfig['name'] = $config['upnp']['name'];
$pconfig['if'] = $config['upnp']['if'];
$pconfig['port'] = $config['upnp']['port'];
$pconfig['web'] = isset($config['upnp']['web']);
$pconfig['home'] = $config['upnp']['home'];
$pconfig['profile'] = $config['upnp']['profile'];
$pconfig['deviceip'] = $config['upnp']['deviceip'];
$pconfig['transcoding'] = isset($config['upnp']['transcoding']);
$pconfig['tempdir'] = $config['upnp']['tempdir'];
$pconfig['content'] = $config['upnp']['content'];

// Set name to configured hostname if it is not set.
if (!$pconfig['name'])
	$pconfig['name'] = $config['system']['hostname'];

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	// Input validation.
	if ($_POST['enable']) {
		$reqdfields = explode(" ", "name if port content home");
		$reqdfieldsn = array(gettext("Name"), gettext("Interface"), gettext("Port"), gettext("Content"), gettext("Database directory"));
		$reqdfieldst = explode(" ", "string string port array string");

		if ("Terratec_Noxon_iRadio" === $_POST['profile']) {
			$reqdfields = array_merge($reqdfields, array("deviceip"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Device IP")));
			$reqdfieldst = array_merge($reqdfieldst, array("ipaddr"));
		}

		if (isset($_POST['transcoding'])) {
			$reqdfields = array_merge($reqdfields, array("tempdir"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Temporary directory")));
			$reqdfieldst = array_merge($reqdfieldst, array("string"));
		}

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

		// Check if port is already used.
		if (services_is_port_used($_POST['port'], "upnp"))
			$input_errors[] = sprintf(gettext("Port %ld is already used by another service."), $_POST['port']);

		// Check port range.
		if ($_POST['port'] && ((1024 > $_POST['port']) || (65535 < $_POST['port']))) {
			$input_errors[] = sprintf(gettext("The attribute '%s' must be in the range from %d to %d."), gettext("Port"), 1025, 65535);
		}
	}

	if (!$input_errors) {
		$config['upnp']['enable'] = $_POST['enable'] ? true : false;
		$config['upnp']['name'] = $_POST['name'];
		$config['upnp']['if'] = $_POST['if'];
		$config['upnp']['port'] = $_POST['port'];
		$config['upnp']['web'] = $_POST['web'] ? true : false;
		$config['upnp']['home'] = $_POST['home'];
		$config['upnp']['profile'] = $_POST['profile'];
		$config['upnp']['deviceip'] = $_POST['deviceip'];
		$config['upnp']['transcoding'] = $_POST['transcoding'] ? true : false;
		$config['upnp']['tempdir'] = $_POST['tempdir'];
		$config['upnp']['content'] = $_POST['content'];

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("fuppes");
			$retval |= rc_update_service("mdnsresponder");
			config_unlock();
		}

		$savemsg = get_std_save_message($retval);

		if ($retval == 0) {
			if (file_exists($d_upnpconfdirty_path))
				unlink($d_upnpconfdirty_path);
		}
	}
}

$a_interface = get_interface_list();

// Use first interface as default if it is not set.
if (empty($pconfig['if']) && is_array($a_interface))
	$pconfig['if'] = key($a_interface);
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	var endis = !(document.iform.enable.checked || enable_change);
	document.iform.name.disabled = endis;
	document.iform.xif.disabled = endis;
	document.iform.port.disabled = endis;
	document.iform.web.disabled = endis;
	document.iform.home.disabled = endis;
	document.iform.homebrowsebtn.disabled = endis;
	document.iform.content.disabled = endis;
	document.iform.contentaddbtn.disabled = endis;
	document.iform.contentchangebtn.disabled = endis;
	document.iform.contentdeletebtn.disabled = endis;
	document.iform.contentdata.disabled = endis;
	document.iform.contentbrowsebtn.disabled = endis;
	document.iform.profile.disabled = endis;
	document.iform.deviceip.disabled = endis;
	document.iform.transcoding.disabled = endis;
	document.iform.tempdir.disabled = endis;
}

function profile_change() {
	switch(document.iform.profile.value) {
		case "Terratec_Noxon_iRadio":
			showElementById('deviceip_tr','show');
			break;

		default:
			showElementById('deviceip_tr','hide');
			break;
	}
}

function web_change() {
	switch(document.iform.web.checked) {
		case false:
			showElementById('url_tr','hide');
			break;

		case true:
			showElementById('url_tr','show');
			break;
	}
}

function transcoding_change() {
	switch(document.iform.transcoding.checked) {
		case false:
			showElementById('tempdir_tr','hide');
			break;

		case true:
			showElementById('tempdir_tr','show');
			break;
	}
}
//-->
</script>
<form action="services_upnp.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
		<tr>
			<td class="tabcont">
				<?php if ($input_errors) print_input_errors($input_errors); ?>
				<?php if ($savemsg) print_info_box($savemsg); ?>
				<?php if (file_exists($d_upnpconfdirty_path)) print_config_change_box();?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
				<?php html_titleline_checkbox("enable", gettext("UPnP A/V Media Server"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
					<?php html_inputbox("name", gettext("Name"), $pconfig['name'], gettext("UPnP friendly name."), true, 20);?>
					<!--
					<?php html_interfacecombobox("if", gettext("Interface"), $pconfig['if'], gettext("Interface to listen to."), true);?>
					-->
				<tr>
					<td width="22%" valign="top" class="vncellreq"><?=gettext("Interface");?></td>
					<td width="78%" class="vtable">
					<select name="if" class="formfld" id="xif">
						<?php foreach($a_interface as $if => $ifinfo):?>
							<?php $ifinfo = get_interface_info($if); if (("up" == $ifinfo['status']) || ("associated" == $ifinfo['status'])):?>
							<option value="<?=$if;?>"<?php if ($if == $pconfig['if']) echo "selected=\"selected\"";?>><?=$if?></option>
							<?php endif;?>
						<?php endforeach;?>
					</select>
					<br /><?=gettext("Interface to listen to.");?>
					</td>
				</tr>
					<?php html_inputbox("port", gettext("Port"), $pconfig['port'], sprintf(gettext("Port to listen on. Only dynamic or private ports can be used (from %d through %d). Default port is %d."), 1025, 65535, 49152), true, 5);?>
					<?php html_filechooser("home", gettext("Database directory"), $pconfig['home'], gettext("Location where the content database file will be stored."), $g['media_path'], true, 60);?>
					<?php html_folderbox("content", gettext("Content"), $pconfig['content'], gettext("Location of the files to share."), $g['media_path'], true);?>
					<?php html_combobox("profile", gettext("Profile"), $pconfig['profile'], array("default" => gettext("Default"), "DLNA" => "DLNA", "Denon_AVR" => "DENON Network A/V Receiver", "PS3" => "Sony Playstation 3", "Telegent_TG100" => "Telegent TG100", "ZyXEL_DMA1000" => "ZyXEL DMA-1000", "Helios_X3000" => "Helios X3000", "DLink_DSM320" => "D-Link DSM320", "Microsoft_XBox360" => "Microsoft XBox 360", "Terratec_Noxon_iRadio" => "Terratec Noxon iRadio", "Yamaha_RXN600" => "Yamaha RX-N600", "Loewe_Connect" => "Loewe Connect"), gettext("Compliant profile to be used."), true, false, "profile_change()");?>
					<?php html_inputbox("deviceip", gettext("Device IP"), $pconfig['deviceip'], gettext("The device's IP address."), true, 20);?>
					<?php html_checkbox("transcoding", gettext("Transcoding"), $pconfig['transcoding'] ? true : false, gettext("Enable transcoding."), "", false, "transcoding_change()");?>
					<?php html_filechooser("tempdir", gettext("Temporary directory"), $pconfig['tempdir'], gettext("Temporary directory to store transcoded files."), $g['media_path'], true, 60);?>
					<?php html_separator();?>
					<?php html_titleline(gettext("Administrative WebGUI"));?>
					<?php html_checkbox("web", gettext("WebGUI"), $pconfig['web'] ? true : false, gettext("Enable web user interface."), "", false, "web_change()");?>
					<?php
					$if = get_ifname($pconfig['if']);
					$ipaddr = get_ipaddr($if);
					$url = htmlspecialchars("http://{$ipaddr}:{$pconfig['port']}");
					$text = "<a href='{$url}' target='_blank'>{$url}</a>";
					?>
					<?php html_text("url", gettext("URL"), $text);?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save and Restart");?>" onclick="onsubmit_content(); enable_change(true)" />
				</div>
			</td>
		</tr>
	</table>
	<?php include("formend.inc");?>
</form>
<script type="text/javascript">
<!--
profile_change();
web_change();
transcoding_change();
enable_change(false);
//-->
</script>
<?php include("fend.inc");?>
