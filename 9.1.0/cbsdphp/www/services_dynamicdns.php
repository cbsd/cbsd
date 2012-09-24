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

$pgtitle = array(gettext("Services"), gettext("Dynamic DNS"));

if (!isset($config['dynamicdns']) || !is_array($config['dynamicdns']))
	$config['dynamicdns'] = array();

$pconfig['enable'] = isset($config['dynamicdns']['enable']);
$pconfig['provider'] = $config['dynamicdns']['provider'];
$pconfig['domainname'] = $config['dynamicdns']['domainname'];
$pconfig['username'] = $config['dynamicdns']['username'];
$pconfig['password'] = $config['dynamicdns']['password'];
$pconfig['updateperiod'] = $config['dynamicdns']['updateperiod'];
$pconfig['forcedupdateperiod'] = $config['dynamicdns']['forcedupdateperiod'];
$pconfig['wildcard'] = isset($config['dynamicdns']['wildcard']);
if (is_array($config['dynamicdns']['auxparam']))
	$pconfig['auxparam'] = implode("\n", $config['dynamicdns']['auxparam']);

if ($_POST) {
	unset($input_errors);

	$pconfig = $_POST;

	/* input validation */
	if ($_POST['enable']) {
		$reqdfields = explode(" ", "provider domainname username password");
		$reqdfieldsn = array(gettext("Provider"), gettext("Domain name"), gettext("Username"), gettext("Password"));
		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

		$reqdfields = array_merge($reqdfields, explode(" ", "updateperiod forcedupdateperiod"));
		$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Update period"),gettext("Forced update period")));
		$reqdfieldst = explode(" ", "string string string string numeric numeric");
		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);
	}

	if (!$input_errors) {
		$config['dynamicdns']['enable'] = $_POST['enable'] ? true : false;
		$config['dynamicdns']['provider'] = $_POST['provider'];
		$config['dynamicdns']['domainname'] = $_POST['domainname'];
		$config['dynamicdns']['username'] = $_POST['username'];
		$config['dynamicdns']['password'] = $_POST['password'];
		$config['dynamicdns']['updateperiod'] = $_POST['updateperiod'];
		$config['dynamicdns']['forcedupdateperiod'] = $_POST['forcedupdateperiod'];
		$config['dynamicdns']['wildcard'] = $_POST['wildcard'] ? true : false;

		# Write additional parameters.
		unset($config['dynamicdns']['auxparam']);
		foreach (explode("\n", $_POST['auxparam']) as $auxparam) {
			$auxparam = trim($auxparam, "\t\n\r");
			if (!empty($auxparam))
				$config['dynamicdns']['auxparam'][] = $auxparam;
		}

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("inadyn");
			config_unlock();
		}

		$savemsg = get_std_save_message($retval);
	}
}
// Get list of available interfaces.
$a_interface = get_interface_list();
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	var endis = !(document.iform.enable.checked || enable_change);
	document.iform.provider.disabled = endis;
	document.iform.domainname.disabled = endis;
	document.iform.username.disabled = endis;
	document.iform.password.disabled = endis;
	document.iform.updateperiod.disabled = endis;
	document.iform.forcedupdateperiod.disabled = endis;
	document.iform.wildcard.disabled = endis;
	document.iform.auxparam.disabled = endis;
}

function provider_change() {
	switch(document.iform.provider.value) {
		case "dyndns.org":
		case "3322.org":
		case "easydns.com":
		case "custom":
			showElementById('wildcard_tr','show');
			break;

		default:
			showElementById('wildcard_tr','hide');
			break;
	}
}
//-->
</script>
<form action="services_dynamicdns.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
	  <tr>
	    <td class="tabcont">
	    	<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
			  <table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gettext("Dynamic DNS"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
					<?php html_combobox("provider", gettext("Provider"), $pconfig['provider'], array("dyndns.org" => "dyndns.org", "freedns.afraid.org" => "freedns.afraid.org", "zoneedit.com" => "zoneedit.com", "no-ip.com" => "no-ip.com", "easydns.com" => "easydns.com", "3322.org" => "3322.org", "custom" => gettext("Custom")), "", true, false, "provider_change()");?>
					<?php html_inputbox("domainname", gettext("Domain name"), $pconfig['domainname'], gettext("A host name alias. This option can appear multiple times, for each domain that has the same IP. Use a space to separate multiple alias names."), true, 40);?>
					<?php html_inputbox("username", gettext("Username"), $pconfig['username'], "", true, 20);?>
					<?php html_passwordbox("password", gettext("Password"), $pconfig['password'], "", true, 20);?>
					<?php html_inputbox("updateperiod", gettext("Update period"), $pconfig['updateperiod'], gettext("How often the IP is checked. The period is in seconds (max. is 10 days)."), false, 20);?>
					<?php html_inputbox("forcedupdateperiod", gettext("Forced update period"), $pconfig['forcedupdateperiod'], gettext("How often the IP is updated even if it is not changed. The period is in seconds (max. is 10 days)."), false, 20);?>
					<?php html_checkbox("wildcard", gettext("Wildcard"), $pconfig['wildcard'] ? true : false, gettext("Enable domain wildcarding."), "", false);?>
					<?php html_textarea("auxparam", gettext("Auxiliary parameters"), $pconfig['auxparam'], sprintf(gettext("These parameters will be added to global settings in %s."), "inadyn.conf"), false, 65, 3, false, false);?>
			  </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save and Restart");?>" onclick="enable_change(true)" />
				</div>
			</td>
		</tr>
	</table>
	<?php include("formend.inc");?>
</form>
<script type="text/javascript">
<!--
enable_change(false);
provider_change();
//-->
</script>
<?php include("fend.inc");?>
