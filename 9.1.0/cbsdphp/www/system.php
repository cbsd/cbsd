#!/usr/local/bin/php
<?php
/*
	system.php
	
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
require("services.inc");

$pgtitle = array(gettext("System"), gettext("General Setup"));

$pconfig['hostname'] = $config['system']['hostname'];
$pconfig['domain'] = $config['system']['domain'];
list($pconfig['dns1'],$pconfig['dns2']) = get_ipv4dnsserver();
list($pconfig['ipv6dns1'],$pconfig['ipv6dns2']) = get_ipv6dnsserver();
$pconfig['username'] = $config['system']['username'];
$pconfig['webguiproto'] = $config['system']['webgui']['protocol'];
$pconfig['webguiport'] = $config['system']['webgui']['port'];
$pconfig['language'] = $config['system']['language'];
$pconfig['timezone'] = $config['system']['timezone'];
$pconfig['ntp_enable'] = isset($config['system']['ntp']['enable']);
$pconfig['ntp_timeservers'] = $config['system']['ntp']['timeservers'];
$pconfig['ntp_updateinterval'] = $config['system']['ntp']['updateinterval'];
$pconfig['language'] = $config['system']['language'];
$pconfig['certificate'] = base64_decode($config['system']['webgui']['certificate']);
$pconfig['privatekey'] = base64_decode($config['system']['webgui']['privatekey']);

// Set default values if necessary.
if (!$pconfig['language'])
	$pconfig['language'] = "English";
if (!$pconfig['timezone'])
	$pconfig['timezone'] = "Etc/UTC";
if (!$pconfig['webguiproto'])
	$pconfig['webguiproto'] = "http";
if (!$pconfig['username'])
	$pconfig['username'] = "admin";
if (!$pconfig['ntp_timeservers'])
	$pconfig['ntp_timeservers'] = "pool.ntp.org";
if (!isset($pconfig['ntp_updateinterval']))
	$pconfig['ntp_updateinterval'] = 300;

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	// Input validation.
	$reqdfields = explode(" ", "hostname username");
	$reqdfieldsn = array(gettext("Hostname"), gettext("Username"));
	$reqdfieldst = explode(" ", "hostname alias");

	if (!empty($_POST['domain'])) {
		$reqdfields = array_merge($reqdfields, array("domain"));
		$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Domain")));
		$reqdfieldst = array_merge($reqdfieldst, array("domain"));
	}

	if (isset($_POST['ntp_enable'])) {
		$reqdfields = array_merge($reqdfields, explode(" ", "ntp_timeservers ntp_updateinterval"));
		$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("NTP time server"), gettext("Time update interval")));
		$reqdfieldst = array_merge($reqdfieldst, explode(" ", "string numeric"));
	}

	if ("https" === $_POST['webguiproto']) {
		$reqdfields = array_merge($reqdfields, explode(" ", "certificate privatekey"));
		$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Certificate"), gettext("Private key")));
		$reqdfieldst = array_merge($reqdfieldst, explode(" ", "certificate privatekey"));
	}

	if (!empty($_POST['webguiport'])) {
		$reqdfields = array_merge($reqdfields, array("webguiport"));
		$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Port")));
		$reqdfieldst = array_merge($reqdfieldst, array("port"));
	}

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if (($_POST['dns1'] && !is_ipv4addr($_POST['dns1'])) || ($_POST['dns2'] && !is_ipv4addr($_POST['dns2']))) {
		$input_errors[] = gettext("A valid IPv4 address must be specified for the primary/secondary DNS server.");
	}

	if (($_POST['ipv6dns1'] && !is_ipv6addr($_POST['ipv6dns1'])) || ($_POST['ipv6dns2'] && !is_ipv6addr($_POST['ipv6dns2']))) {
		$input_errors[] = gettext("A valid IPv6 address must be specified for the primary/secondary DNS server.");
	}

	if (isset($_POST['ntp_enable'])) {
		$t = (int)$_POST['ntp_updateinterval'];
		if (($t < 0) || (($t > 0) && ($t < 6)) || ($t > 1440)) {
			$input_errors[] = gettext("The time update interval must be either between 6 and 1440.");
		}

		foreach (explode(' ', $_POST['ntp_timeservers']) as $ts) {
			if (!is_domain($ts)) {
				$input_errors[] = gettext("A NTP time server name may only contain the characters a-z, 0-9, '-' and '.'.");
			}
		}
	}

	// Check if port is already used.
	if (services_is_port_used(!empty($_POST['webguiport']) ? $_POST['webguiport'] : 80, "sysgui")) {
		$input_errors[] = sprintf(gettext("Port %ld is already used by another service."), (!empty($_POST['webguiport']) ? $_POST['webguiport'] : 80));
	}

	// Check Webserver document root if auth is required
	if (isset($config['websrv']['enable'])
	    && isset($config['websrv']['authentication']['enable'])
	    && !is_dir($config['websrv']['documentroot'])) {
		$input_errors[] = gettext("Webserver document root is missing.");
	}

	if (!$input_errors) {
		// Store old values for later processing.
		$oldcert = $config['system']['webgui']['certificate'];
		$oldkey = $config['system']['webgui']['privatekey'];
		$oldwebguiproto = $config['system']['webgui']['protocol'];
		$oldwebguiport = $config['system']['webgui']['port'];
		$oldlanguage = $config['system']['language'];

		$config['system']['hostname'] = strtolower($_POST['hostname']);
		$config['system']['domain'] = strtolower($_POST['domain']);
		$config['system']['username'] = $_POST['username'];
		$config['system']['webgui']['protocol'] = $_POST['webguiproto'];
		$config['system']['webgui']['port'] = $_POST['webguiport'];
		$config['system']['language'] = $_POST['language'];
		$config['system']['timezone'] = $_POST['timezone'];
		$config['system']['ntp']['enable'] = $_POST['ntp_enable'] ? true : false;
		$config['system']['ntp']['timeservers'] = strtolower($_POST['ntp_timeservers']);
		$config['system']['ntp']['updateinterval'] = $_POST['ntp_updateinterval'];
		$config['system']['webgui']['certificate'] = base64_encode($_POST['certificate']);
		$config['system']['webgui']['privatekey'] =  base64_encode($_POST['privatekey']);

		unset($config['system']['dnsserver']);
		// Only store IPv4 DNS servers when using static IPv4.
		if ("dhcp" !== $config['interfaces']['lan']['ipaddr']) {
			unset($config['system']['dnsserver']);
			if ($_POST['dns1'])
				$config['system']['dnsserver'][] = $_POST['dns1'];
			if ($_POST['dns2'])
				$config['system']['dnsserver'][] = $_POST['dns2'];
		}
		// Only store IPv6 DNS servers when using static IPv6.
		if ("auto" !== $config['interfaces']['lan']['ipv6addr']) {
			unset($config['system']['ipv6dnsserver']);
			if ($_POST['ipv6dns1'])
				$config['system']['ipv6dnsserver'][] = $_POST['ipv6dns1'];
			if ($_POST['ipv6dns2'])
				$config['system']['ipv6dnsserver'][] = $_POST['ipv6dns2'];
		}

		$olddnsallowoverride = $config['system']['dnsallowoverride'];
		$config['system']['dnsallowoverride'] = $_POST['dnsallowoverride'] ? true : false;

		write_config();
		set_php_timezone();

		// Check if a reboot is required.
		if (($oldwebguiproto != $config['system']['webgui']['protocol']) ||
			($oldwebguiport != $config['system']['webgui']['port'])) {
			touch($d_sysrebootreqd_path);
		}
		if (($config['system']['webgui']['certificate'] != $oldcert) || ($config['system']['webgui']['privatekey'] != $oldkey)) {
			touch($d_sysrebootreqd_path);
		}

		$retval = 0;

		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_exec_service("rcconf");
			$retval |= rc_exec_service("timezone");
			$retval |= rc_exec_service("resolv");
			$retval |= rc_exec_service("hosts");
			$retval |= rc_restart_service("hostname");
			$retval |= rc_exec_service("userdb");
			$retval |= rc_exec_service("htpasswd");
			$retval |= rc_exec_service("websrv_htpasswd");
 			$retval |= rc_update_service("ntpdate");
 			$retval |= rc_update_service("mdnsresponder");
 			$retval |= rc_update_service("bsnmpd");
 			$retval |= rc_update_service("cron");
			config_unlock();
		}

		if (($pconfig['systime'] !== "Not Set") && (!empty($pconfig['systime']))) {
			$timestamp = strtotime($pconfig['systime']);
			if (FALSE !== $timestamp) {
				$timestamp = strftime("%g%m%d%H%M", $timestamp);
				// The date utility exits 0 on success, 1 if unable to set the date,
				// and 2 if able to set the local date, but unable to set it globally.
				$retval |= mwexec("/bin/date -n {$timestamp}");
				$pconfig['systime'] = "Not Set";
			}
		}

		$savemsg = get_std_save_message($retval);

		// Update DNS server controls.
		list($pconfig['dns1'],$pconfig['dns2']) = get_ipv4dnsserver();
		list($pconfig['ipv6dns1'],$pconfig['ipv6dns2']) = get_ipv6dnsserver();

		// Reload page if language has been changed, otherwise page is displayed
		// in previous selected language.
		if ($oldlanguage !== $config['system']['language']) {
			header("Location: system.php");
			exit;
		}
	}
}

$pglocalheader = <<< EOD
<link rel="stylesheet" type="text/css" href="datechooser.css" />
<script type="text/javascript" src="datechooser.js"></script>
EOD;
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function ntp_change() {
	switch(document.iform.ntp_enable.checked) {
		case false:
			showElementById('ntp_timeservers_tr','hide');
			showElementById('ntp_updateinterval_tr','hide');
			break;

		case true:
			showElementById('ntp_timeservers_tr','show');
			showElementById('ntp_updateinterval_tr','show');
			break;
	}
}

function webguiproto_change() {
	switch(document.iform.webguiproto.selectedIndex) {
		case 0:
			showElementById('privatekey_tr','hide');
			showElementById('certificate_tr','hide');
			break;

		default:
			showElementById('privatekey_tr','show');
			showElementById('certificate_tr','show');
			break;
	}
}
//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
      	<li class="tabact"><a href="system.php" title="<?=gettext("Reload page");?>"><span><?=gettext("General");?></span></a></li>
      	<li class="tabinact"><a href="system_password.php"><span><?=gettext("Password");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
			<form action="system.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
			  <table width="100%" border="0" cellpadding="6" cellspacing="0">
			  	<tr>
						<td colspan="2" valign="top" class="listtopic"><?=gettext("Hostname");?></td>
					</tr>
					<?php html_inputbox("hostname", gettext("Hostname"), $pconfig['hostname'], sprintf(gettext("Name of the NAS host, without domain part e.g. %s."), "<em>" . strtolower(get_product_name()) ."</em>"), true, 40);?>
					<?php html_inputbox("domain", gettext("Domain"), $pconfig['domain'], sprintf(gettext("e.g. %s"), "<em>com, local</em>"), false, 40);?>
					<?php html_separator();?>
					<?php html_titleline(gettext("DNS settings"));?>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("IPv4 DNS servers");?></td>
			      <td width="78%" class="vtable">
							<?php $readonly = ("dhcp" === $config['interfaces']['lan']['ipaddr']) ? "readonly=\"readonly\"" : "";?>
							<input name="dns1" type="text" class="formfld" id="dns1" size="20" value="<?=htmlspecialchars($pconfig['dns1']);?>" <?=$readonly;?> /><br />
							<input name="dns2" type="text" class="formfld" id="dns2" size="20" value="<?=htmlspecialchars($pconfig['dns2']);?>" <?=$readonly;?> /><br />
							<span class="vexpl"><?=gettext("IPv4 addresses");?></span><br />
			      </td>
			    </tr>
				  <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("IPv6 DNS servers");?></td>
			      <td width="78%" class="vtable">
							<?php $readonly = (!isset($config['interfaces']['lan']['ipv6_enable']) || ("auto" === $config['interfaces']['lan']['ipv6addr'])) ? "readonly=\"readonly\"" : "";?>
							<input name="ipv6dns1" type="text" class="formfld" id="ipv6dns1" size="20" value="<?=htmlspecialchars($pconfig['ipv6dns1']);?>" <?=$readonly;?> /><br />
							<input name="ipv6dns2" type="text" class="formfld" id="ipv6dns2" size="20" value="<?=htmlspecialchars($pconfig['ipv6dns2']);?>" <?=$readonly;?> /><br />
							<span class="vexpl"><?=gettext("IPv6 addresses");?></span><br />
			      </td>
			    </tr>
			    <?php html_separator();?>
			    <?php html_titleline(gettext("WebGUI"));?>
					<?php html_inputbox("username", gettext("Username"), $pconfig['username'], gettext("If you want to change the username for accessing the WebGUI, enter it here."), false, 20);?>
					<?php html_combobox("webguiproto", gettext("Protocol"), $pconfig['webguiproto'], array("http" => "HTTP", "https" => "HTTPS"), "", false, false, "webguiproto_change()");?>
					<?php html_inputbox("webguiport", gettext("Port"), $pconfig['webguiport'], gettext("Enter a custom port number for the WebGUI above if you want to override the default (80 for HTTP, 443 for HTTPS)."), false, 5);?>
					<?php html_textarea("certificate", gettext("Certificate"), $pconfig['certificate'], gettext("Paste a signed certificate in X.509 PEM format here."), true, 65, 7, false, false);?>
					<?php html_textarea("privatekey", gettext("Private key"), $pconfig['privatekey'], gettext("Paste an private key in PEM format here."), true, 65, 7, false, false);?>
					<?php html_languagecombobox("language", gettext("Language"), $pconfig['language'], "", false);?>
					<?php html_separator();?>
					<?php html_titleline(gettext("Time"));?>
					<?php html_timezonecombobox("timezone", gettext("Time zone"), $pconfig['timezone'], gettext("Select the location closest to you."), false);?>
			    <tr>
						<td width="22%" valign="top" class="vncell"><?=gettext("System time");?></td>
						<td width="78%" class="vtable">
							<input id="systime" size="20" maxlength="20" name="systime" type="text" value="" />
							<img src="cal.gif" onclick="showChooser(this, 'systime', 'chooserSpan', 1950, 2020, Date.patterns.Default, true);" alt="" />
							<div id="chooserSpan" class="dateChooser select-free" style="display: none; visibility: hidden; width: 160px;"></div><br />
							<span class="vexpl"><?=gettext("Enter desired system time directly (format mm/dd/yyyy hh:mm) or use icon to select it.");?></span>
						</td>
			    </tr>
					<?php html_checkbox("ntp_enable", gettext("Enable NTP"), $pconfig['ntp_enable'] ? true : false, gettext("Use the specified NTP server."), "", false, "ntp_change()");?>
					<?php html_inputbox("ntp_timeservers", gettext("NTP time server"), $pconfig['ntp_timeservers'], gettext("Use a space to separate multiple hosts (only one required). Remember to set up at least one DNS server if you enter a host name here!"), true, 40);?>
					<?php html_inputbox("ntp_updateinterval", gettext("Time update interval"), $pconfig['ntp_updateinterval'], gettext("Minutes between network time sync."), true, 20);?>
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
ntp_change();
webguiproto_change();
//-->
</script>
<?php include("fend.inc");?>
