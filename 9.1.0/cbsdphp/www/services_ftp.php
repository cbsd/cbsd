#!/usr/local/bin/php
<?php
/*
	services_ftp.php

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

$pgtitle = array(gettext("Services"), gettext("FTP"));

if (!isset($config['ftpd']) || !is_array($config['ftpd']))
	$config['ftpd'] = array();

$pconfig['enable'] = isset($config['ftpd']['enable']);
$pconfig['port'] = $config['ftpd']['port'];
$pconfig['numberclients'] = $config['ftpd']['numberclients'];
$pconfig['maxconperip'] = $config['ftpd']['maxconperip'];
$pconfig['maxloginattempts'] = $config['ftpd']['maxloginattempts'];
$pconfig['timeout'] = $config['ftpd']['timeout'];
$pconfig['anonymousonly'] = isset($config['ftpd']['anonymousonly']);
$pconfig['localusersonly'] = isset($config['ftpd']['localusersonly']);
$pconfig['pasv_max_port'] = $config['ftpd']['pasv_max_port'];
$pconfig['pasv_min_port'] = $config['ftpd']['pasv_min_port'];
$pconfig['pasv_address'] = $config['ftpd']['pasv_address'];
$pconfig['userbandwidthup'] = $config['ftpd']['userbandwidth']['up'];
$pconfig['userbandwidthdown'] = $config['ftpd']['userbandwidth']['down'];
$pconfig['anonymousbandwidthup'] = $config['ftpd']['anonymousbandwidth']['up'];
$pconfig['anonymousbandwidthdown'] = $config['ftpd']['anonymousbandwidth']['down'];
if ($config['ftpd']['filemask']) {
	$pconfig['filemask'] = $config['ftpd']['filemask'];
} else {
	$pconfig['filemask'] = "077";
}
if ($config['ftpd']['directorymask']) {
	$pconfig['directorymask'] = $config['ftpd']['directorymask'];
} else {
	$pconfig['directorymask'] = "022";
}
$pconfig['banner'] = $config['ftpd']['banner'];
$pconfig['fxp'] = isset($config['ftpd']['fxp']);
$pconfig['allowrestart'] = isset($config['ftpd']['allowrestart']);
$pconfig['permitrootlogin'] = isset($config['ftpd']['permitrootlogin']);
$pconfig['chrooteveryone'] = isset($config['ftpd']['chrooteveryone']);
$pconfig['identlookups'] = isset($config['ftpd']['identlookups']);
$pconfig['usereversedns'] = isset($config['ftpd']['usereversedns']);
$pconfig['tls'] = isset($config['ftpd']['tls']);
$pconfig['tlsrequired'] = isset($config['ftpd']['tlsrequired']);
$pconfig['privatekey'] = base64_decode($config['ftpd']['privatekey']);
$pconfig['certificate'] = base64_decode($config['ftpd']['certificate']);
if (is_array($config['ftpd']['auxparam']))
	$pconfig['auxparam'] = implode("\n", $config['ftpd']['auxparam']);

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['enable']) {
		// Input validation.
		$reqdfields = explode(" ", "port numberclients maxconperip timeout maxloginattempts");
		$reqdfieldsn = array(gettext("TCP port"), gettext("Number of clients"), gettext("Max. conn. per IP"), gettext("Timeout"), gettext("Max. login attempts"));
		$reqdfieldst = explode(" ", "numeric numeric numeric numeric numeric");

		if ($_POST['tls']) {
			$reqdfields = array_merge($reqdfields, explode(" ", "certificate privatekey"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Certificate"), gettext("Private key")));
			$reqdfieldst = array_merge($reqdfieldst, explode(" ", "certificate privatekey"));
		}

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

		if (!is_port($_POST['port'])) {
			$input_errors[] = gettext("The TCP port must be a valid port number.");
		}

		if ((1 > $_POST['numberclients']) || (50 < $_POST['numberclients'])) {
			$input_errors[] = gettext("The number of clients must be between 1 and 50.");
		}

		if (0 > $_POST['maxconperip']) {
			$input_errors[] = gettext("The max. connection per IP must be either 0 (unlimited) or greater.");
		}

		if (!is_numericint($_POST['timeout'])) {
			$input_errors[] = gettext("The maximum idle time be a number.");
		}

		if (("0" !== $_POST['pasv_min_port']) && (($_POST['pasv_min_port'] < 1024) || ($_POST['pasv_min_port'] > 65535))) {
			$input_errors[] = sprintf(gettext("The attribute '%s' must be in the range from %d to %d."), gettext("Min. passive port"), 1024, 65535);
		}

		if (("0" !== $_POST['pasv_max_port']) && (($_POST['pasv_max_port'] < 1024) || ($_POST['pasv_max_port'] > 65535))) {
			$input_errors[] = sprintf(gettext("The attribute '%s' must be in the range from %d to %d."), gettext("Max. passive port"), 1024, 65535);
		}

		if (("0" !== $_POST['pasv_min_port']) && ("0" !== $_POST['pasv_max_port'])) {
			if ($_POST['pasv_min_port'] >= $_POST['pasv_max_port']) {
				$input_errors[] = sprintf(gettext("The attribute '%s' must be less than '%s'."), gettext("Min. passive port"), gettext("Max. passive port"));
			}
		}

		if ($_POST['anonymousonly'] && $_POST['localusersonly']) {
			$input_errors[] = gettext("It is impossible to enable 'Anonymous users only' and 'Local users only' authentication simultaneously.");
		}
	}

	if (!$input_errors) {
		$config['ftpd']['enable'] = $_POST['enable'] ? true : false;
		$config['ftpd']['numberclients'] = $_POST['numberclients'];
		$config['ftpd']['maxconperip'] = $_POST['maxconperip'];
		$config['ftpd']['maxloginattempts'] = $_POST['maxloginattempts'];
		$config['ftpd']['timeout'] = $_POST['timeout'];
		$config['ftpd']['port'] = $_POST['port'];
		$config['ftpd']['anonymousonly'] = $_POST['anonymousonly'] ? true : false;
		$config['ftpd']['localusersonly'] = $_POST['localusersonly'] ? true : false;
		$config['ftpd']['pasv_max_port'] = $_POST['pasv_max_port'];
		$config['ftpd']['pasv_min_port'] = $_POST['pasv_min_port'];
		$config['ftpd']['pasv_address'] = $_POST['pasv_address'];
		$config['ftpd']['banner'] = $_POST['banner'];
		$config['ftpd']['filemask'] = $_POST['filemask'];
		$config['ftpd']['directorymask'] = $_POST['directorymask'];
		$config['ftpd']['fxp'] = $_POST['fxp'] ? true : false;
		$config['ftpd']['allowrestart'] = $_POST['allowrestart'] ? true : false;
		$config['ftpd']['permitrootlogin'] = $_POST['permitrootlogin'] ? true : false;
		$config['ftpd']['chrooteveryone'] = $_POST['chrooteveryone'] ? true : false;
		$config['ftpd']['identlookups'] = $_POST['identlookups'] ? true : false;
		$config['ftpd']['usereversedns'] = $_POST['usereversedns'] ? true : false;
		$config['ftpd']['tls'] = $_POST['tls'] ? true : false;
		$config['ftpd']['tlsrequired'] = $_POST['tlsrequired'] ? true : false;
		$config['ftpd']['privatekey'] = base64_encode($_POST['privatekey']);
		$config['ftpd']['certificate'] = base64_encode($_POST['certificate']);
		$config['ftpd']['userbandwidth']['up'] = $pconfig['userbandwidthup'];
		$config['ftpd']['userbandwidth']['down'] = $pconfig['userbandwidthdown'];
		$config['ftpd']['anonymousbandwidth']['up'] = $pconfig['anonymousbandwidthup'];
		$config['ftpd']['anonymousbandwidth']['down'] = $pconfig['anonymousbandwidthdown'];

		# Write additional parameters.
		unset($config['ftpd']['auxparam']);
		foreach (explode("\n", $_POST['auxparam']) as $auxparam) {
			$auxparam = trim($auxparam, "\t\n\r");
			if (!empty($auxparam))
				$config['ftpd']['auxparam'][] = $auxparam;
		}

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("proftpd");
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
	document.iform.port.disabled = endis;
	document.iform.timeout.disabled = endis;
	document.iform.permitrootlogin.disabled = endis;
	document.iform.numberclients.disabled = endis;
	document.iform.maxconperip.disabled = endis;
	document.iform.maxloginattempts.disabled = endis;
	document.iform.anonymousonly.disabled = endis;
	document.iform.localusersonly.disabled = endis;
	document.iform.banner.disabled = endis;
	document.iform.fxp.disabled = endis;
	document.iform.allowrestart.disabled = endis;
	document.iform.pasv_max_port.disabled = endis;
	document.iform.pasv_min_port.disabled = endis;
	document.iform.pasv_address.disabled = endis;
	document.iform.filemask.disabled = endis;
	document.iform.directorymask.disabled = endis;
	document.iform.chrooteveryone.disabled = endis;
	document.iform.identlookups.disabled = endis;
	document.iform.usereversedns.disabled = endis;
	document.iform.tls.disabled = endis;
	document.iform.tlsrequired.disabled = endis;
	document.iform.privatekey.disabled = endis;
	document.iform.certificate.disabled = endis;
	document.iform.userbandwidthup.disabled = endis;
	document.iform.userbandwidthdown.disabled = endis;
	document.iform.anonymousbandwidthup.disabled = endis;
	document.iform.anonymousbandwidthdown.disabled = endis;
	document.iform.auxparam.disabled = endis;
}

function tls_change() {
	switch (document.iform.tls.checked) {
		case true:
			showElementById('tlsrequired_tr','show');
			showElementById('privatekey_tr','show');
			showElementById('certificate_tr','show');
			break;

		case false:
			showElementById('tlsrequired_tr','hide');
			showElementById('privatekey_tr','hide');
			showElementById('certificate_tr','hide');
			break;
	}
}

function localusersonly_change() {
	switch (document.iform.localusersonly.checked) {
		case true:
			showElementById('anonymousbandwidthup_tr','hide');
			showElementById('anonymousbandwidthdown_tr','hide');
			break;

		case false:
			showElementById('anonymousbandwidthup_tr','show');
			showElementById('anonymousbandwidthdown_tr','show');
			break;
	}
}

function anonymousonly_change() {
	switch (document.iform.anonymousonly.checked) {
		case true:
			showElementById('userbandwidthup_tr','hide');
			showElementById('userbandwidthdown_tr','hide');
			break;

		case false:
			showElementById('userbandwidthup_tr','show');
			showElementById('userbandwidthdown_tr','show');
			break;
	}
}
//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav">
				<li class="tabact"><a href="services_ftp.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Settings");?></span></a></li>
				<li class="tabinact"><a href="services_ftp_mod.php"><span><?=gettext("Modules");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="services_ftp.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gettext("File Transfer Protocol"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
					<?php html_inputbox("port", gettext("TCP port"), $pconfig['port'], sprintf(gettext("Default is %s."), "21"), true, 4);?>
					<?php html_inputbox("numberclients", gettext("Number of clients"), $pconfig['numberclients'], gettext("Maximum number of simultaneous clients."), true, 3);?>
					<?php html_inputbox("maxconperip", gettext("Max. conn. per IP"), $pconfig['maxconperip'], gettext("Maximum number of connections per IP address (0 = unlimited)."), true, 3);?>
					<?php html_inputbox("maxloginattempts", gettext("Max. login attempts"), $pconfig['maxloginattempts'], gettext("Maximum number of allowed password attempts before disconnection."), true, 3);?>
					<?php html_inputbox("timeout", gettext("Timeout"), $pconfig['timeout'], gettext("Maximum idle time in seconds."), true, 5);?>
					<?php html_checkbox("permitrootlogin", gettext("Permit root login"), $pconfig['permitrootlogin'] ? true : false, gettext("Specifies whether it is allowed to login as superuser (root) directly."), "", false);?>
					<?php html_checkbox("anonymousonly", gettext("Anonymous users only"), $pconfig['anonymousonly'] ? true : false, gettext("Only allow anonymous users. Use this on a public FTP site with no remote FTP access to real accounts."), "", false, "anonymousonly_change()");?>
					<?php html_checkbox("localusersonly", gettext("Local users only"), $pconfig['localusersonly'] ? true : false, gettext("Only allow authenticated users. Anonymous logins are prohibited."), "", false, "localusersonly_change()");?>
					<?php html_textarea("banner", gettext("Banner"), $pconfig['banner'], gettext("Greeting banner displayed by FTP when a connection first comes in."), false, 65, 7, false, false);?>
					<?php html_separator();?>
					<?php html_titleline(gettext("Advanced settings"));?>
					<?php html_inputbox("filemask", gettext("Create mask"), $pconfig['filemask'], gettext("Use this option to override the file creation mask (077 by default)."), false, 3);?>
					<?php html_inputbox("directorymask", gettext("Directory mask"), $pconfig['directorymask'], gettext("Use this option to override the directory creation mask (022 by default)."), false, 3);?>
					<?php html_checkbox("fxp", gettext("FXP"), $pconfig['fxp'] ? true : false, gettext("Enable FXP protocol."), gettext("FXP allows transfers between two remote servers without any file data going to the client asking for the transfer (insecure!)."), false);?>
					<?php html_checkbox("allowrestart", gettext("Resume"), $pconfig['allowrestart'] ? true : false, gettext("Allow clients to resume interrupted uploads and downloads."), "", false);?>
					<?php html_checkbox("chrooteveryone", gettext("Default root"), $pconfig['chrooteveryone'] ? true : false, gettext("chroot() everyone, but root."), gettext("If default root is enabled, a chroot operation is performed immediately after a client authenticates. This can be used to effectively isolate the client from a portion of the host system filespace."), false);?>
					<?php html_checkbox("identlookups", gettext("Ident protocol"), $pconfig['identlookups'] ? true : false, gettext("Enable the ident protocol (RFC1413)."), gettext("When a client initially connects to the server the ident protocol is used to attempt to identify the remote username."), false);?>
					<?php html_checkbox("usereversedns", gettext("Reverse DNS lookup"), $pconfig['usereversedns'] ? true : false, gettext("Enable reverse DNS lookup."), gettext("Enable reverse DNS lookup performed on the remote host's IP address for incoming active mode data connections and outgoing passive mode data connections."), false);?>
					<?php html_inputbox("pasv_address", gettext("Masquerade address"), $pconfig['pasv_address'], gettext("Causes the server to display the network information for the specified IP address or DNS hostname to the client, on the assumption that that IP address or DNS host is acting as a NAT gateway or port forwarder for the server."), false, 20);?>
					<?php html_inputbox("pasv_min_port", gettext("Passive ports"), $pconfig['pasv_min_port'], gettext("The minimum port to allocate for PASV style data connections (0 = use any port)."), false, 20);?>
					<?php html_inputbox("pasv_max_port", "&nbsp;", $pconfig['pasv_max_port'], gettext("The maximum port to allocate for PASV style data connections (0 = use any port).") . "<br /><br />" . gettext("Passive ports restricts the range of ports from which the server will select when sent the PASV command from a client. The server will randomly choose a number from within the specified range until an open port is found. The port range selected must be in the non-privileged range (eg. greater than or equal to 1024). It is strongly recommended that the chosen range be large enough to handle many simultaneous passive connections (for example, 49152-65534, the IANA-registered ephemeral port range)."), true, 20);?>
					<?php html_inputbox("userbandwidthup", gettext("Local user bandwidth"), $pconfig['userbandwidthup'], gettext("Local user upload bandwith in KB/s. An empty field means infinity."), false, 5);?>
					<?php html_inputbox("userbandwidthdown", "&nbsp;", $pconfig['userbandwidthdown'], gettext("Local user download bandwith in KB/s. An empty field means infinity."), false, 5);?>
					<?php html_inputbox("anonymousbandwidthup", gettext("Anonymous user bandwidth"), $pconfig['anonymousbandwidthup'], gettext("Anonymous user upload bandwith in KB/s. An empty field means infinity."), false, 5);?>
					<?php html_inputbox("anonymousbandwidthdown", "&nbsp;", $pconfig['anonymousbandwidthdown'], gettext("Anonymous user download bandwith in KB/s. An empty field means infinity."), false, 5);?>
					<?php html_checkbox("tls", gettext("SSL/TLS"), $pconfig['tls'] ? true : false, gettext("Enable TLS/SSL connections."), "", false, "tls_change()");?>
					<?php html_textarea("certificate", gettext("Certificate"), $pconfig['certificate'], gettext("Paste a signed certificate in X.509 PEM format here."), true, 65, 7, false, false);?>
					<?php html_textarea("privatekey", gettext("Private key"), $pconfig['privatekey'], gettext("Paste an private key in PEM format here."), true, 65, 7, false, false);?>
					<?php html_checkbox("tlsrequired", gettext("SSL/TLS only"), $pconfig['tlsrequired'] ? true : false, gettext("Allow TLS/SSL connections only."), "", false);?>
					<?php html_textarea("auxparam", gettext("Auxiliary parameters"), $pconfig['auxparam'], sprintf(gettext("These parameters are added to %s."), "proftpd.conf") . " " . sprintf(gettext("Please check the <a href='%s' target='_blank'>documentation</a>."), "http://www.proftpd.org/docs/directives/linked/configuration.html"), false, 65, 5, false, false);?>
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
enable_change(false);
anonymousonly_change();
localusersonly_change();
tls_change();
//-->
</script>
<?php include("fend.inc");?>
