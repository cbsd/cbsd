#!/usr/local/bin/php
<?php
/*
	services_ups.php

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
require("email.inc");

$pgtitle = array(gettext("Services"), gettext("UPS"));

if (!isset($config['ups']) || !is_array($config['ups']))
	$config['ups'] = array();

$pconfig['enable'] = isset($config['ups']['enable']);
$pconfig['upsname'] = $config['ups']['upsname'];
$pconfig['driver'] = $config['ups']['driver'];
$pconfig['port'] = $config['ups']['port'];
$pconfig['desc'] = $config['ups']['desc'];
$pconfig['shutdownmode'] = $config['ups']['shutdownmode'];
$pconfig['shutdowntimer'] = $config['ups']['shutdowntimer'];
$pconfig['remotemonitor'] = isset($config['ups']['remotemonitor']);
$pconfig['email_enable'] = isset($config['ups']['email']['enable']);
$pconfig['email_to'] = $config['ups']['email']['to'];
$pconfig['email_subject'] = $config['ups']['email']['subject'];
if (is_array($config['ups']['auxparam']))
	$pconfig['auxparam'] = implode("\n", $config['ups']['auxparam']);

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	// Input validation.
	if ($_POST['enable']) {
		$reqdfields = explode(" ", "upsname driver port shutdownmode");
		$reqdfieldsn = array(gettext("Identifier"), gettext("Driver"), gettext("Port"), gettext("Shutdown mode"));
		$reqdfieldst = explode(" ", "alias string string string");

		if ("onbatt" === $_POST['shutdownmode']) {
			$reqdfields = array_merge($reqdfields, explode(" ", "shutdowntimer"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Shutdown timer")));
			$reqdfieldst = array_merge($reqdfieldst, explode(" ", "numericint"));
		}

		if ($_POST['email_enable']) {
			$reqdfields = array_merge($reqdfields, explode(" ", "email_to email_subject"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("To email"), gettext("Subject")));
			$reqdfieldst = array_merge($reqdfieldst, explode(" ", "string string"));
		}

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);
	}

	if (!$input_errors) {
		$config['ups']['enable'] = $_POST['enable'] ? true : false;
		$config['ups']['upsname'] = $_POST['upsname'];
		$config['ups']['driver'] = $_POST['driver'];
		$config['ups']['port'] = $_POST['port'];
		$config['ups']['desc'] = $_POST['desc'];
		$config['ups']['shutdownmode'] = $_POST['shutdownmode'];
		$config['ups']['shutdowntimer'] = $_POST['shutdowntimer'];
		$config['ups']['remotemonitor'] = $_POST['remotemonitor'] ? true : false;
		$config['ups']['email']['enable'] = $_POST['email_enable'] ? true : false;
		$config['ups']['email']['to'] = $_POST['email_to'];
		$config['ups']['email']['subject'] = $_POST['email_subject'];

		# Write additional parameters.
		unset($config['ups']['auxparam']);
		foreach (explode("\n", $_POST['auxparam']) as $auxparam) {
			$auxparam = trim($auxparam, "\t\n\r");
			if (!empty($auxparam))
				$config['ups']['auxparam'][] = $auxparam;
		}

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("nut");
			$retval |= rc_update_service("nut_upslog");
			$retval |= rc_update_service("nut_upsmon");
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

	if (enable_change.name == "email_enable") {
		endis = !enable_change.checked;

		document.iform.email_to.disabled = endis;
		document.iform.email_subject.disabled = endis;
	} else {
		document.iform.upsname.disabled = endis;
		document.iform.driver.disabled = endis;
		document.iform.port.disabled = endis;
		document.iform.auxparam.disabled = endis;
		document.iform.desc.disabled = endis;
		document.iform.shutdownmode.disabled = endis;
		document.iform.shutdowntimer.disabled = endis;
		document.iform.remotemonitor.disabled = endis;
		document.iform.email_enable.disabled = endis;

		if (document.iform.enable.checked == true) {
			endis = !(document.iform.email_enable.checked || enable_change);
		}

		document.iform.email_to.disabled = endis;
		document.iform.email_subject.disabled = endis;
	}
}

function shutdownmode_change() {
	switch(document.iform.shutdownmode.value) {
		case "onbatt":
			showElementById('shutdowntimer_tr','show');
			break;

		default:
			showElementById('shutdowntimer_tr','hide');
			break;
	}
}
//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabcont">
    	<form action="services_ups.php" method="post" name="iform" id="iform">
				<?php if ($pconfig['enable'] && $pconfig['email_enable'] && (0 !== email_validate_settings())) print_error_box(sprintf(gettext("Make sure you have already configured your <a href='%s'>Email</a> settings."), "system_email.php"));?>
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gettext("Uninterruptible Power Supply"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
					<?php html_inputbox("upsname", gettext("Identifier"), $pconfig['upsname'], gettext("This name is used to uniquely identify your UPS on this system."), true, 30);?>
					<?php html_inputbox("driver", gettext("Driver"), $pconfig['driver'], sprintf(gettext("The driver used to communicate with your UPS. Get the list of available <a href='%s' target='_blank'>drivers</a>."), "services_ups_drv.php"), true, 30);?>
					<?php html_inputbox("port", gettext("Port"), $pconfig['port'], gettext("The serial or USB port where your UPS is connected."), true, 30);?>
					<?php html_textarea("auxparam", gettext("Auxiliary parameters"), $pconfig['auxparam'], gettext("Additional parameters to the hardware-specific part of the driver."), false, 65, 5, false, false);?>
					<?php html_inputbox("desc", gettext("Description"), $pconfig['desc'], gettext("You may enter a description here for your reference."), false, 40);?>
					<?php html_combobox("shutdownmode", gettext("Shutdown mode"), $pconfig['shutdownmode'], array("fsd" => gettext("UPS reaches low battery"), "onbatt" => gettext("UPS goes on battery")), gettext("Defines when the shutdown is initiated."), true, false, "shutdownmode_change()");?>
					<?php html_inputbox("shutdowntimer", gettext("Shutdown timer"), $pconfig['shutdowntimer'], gettext("The time in seconds until shutdown is initiated. If the UPS happens to come back before the time is up the shutdown is canceled."), true, 3);?>
					<?php html_checkbox("remotemonitor", gettext("Remote monitoring"), $pconfig['remotemonitor'] ? true : false, gettext("Enable remote monitoring of the local connected UPS."), "", false);?>
					<?php html_separator();?>
					<?php html_titleline_checkbox("email_enable", gettext("Email notification"), $pconfig['email_enable'] ? true : false, gettext("Activate"), "enable_change(this)");?>
					<?php html_inputbox("email_to", gettext("To email"), $pconfig['email_to'], sprintf("%s %s", gettext("Destination email address."), gettext("Separate email addresses by semi-colon.")), true, 40);?>
					<?php html_inputbox("email_subject", gettext("Subject"), $pconfig['email_subject'], gettext("The subject of the email.") . " " . gettext("You can use the following parameters for substitution:") . "</span>" . gettext("<div id='enumeration'><ul><li>%d - Date</li><li>%h - Hostname</li></ul></div>") . "<span>", true, 60);?>



			  </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save and Restart");?>" onclick="enable_change(true)" />
				</div>
				<div id="remarks">
					<?php html_remark("note", gettext("Note"), sprintf(gettext("This configuration settings are used to generate the ups.conf configuration file which is required by the NUT UPS daemon. To get more information how to configure your UPS please check the NUT (Network UPS Tools) <a href='%s' target='_blank'>documentation</a>."), "http://www.networkupstools.org"));?>
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
  </tr>
</table>
<script type="text/javascript">
<!--
shutdownmode_change();
enable_change(false);
//-->
</script>
<?php include("fend.inc");?>
