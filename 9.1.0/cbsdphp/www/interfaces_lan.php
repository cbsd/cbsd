#!/usr/local/bin/php
<?php
/*
	interfaces_lan.php
	
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

$pgtitle = array(gettext("Network"), gettext("LAN Management"));

$lancfg = &$config['interfaces']['lan'];
$optcfg = &$config['interfaces']['lan']; // Required for WLAN.

// Get interface informations.
$ifinfo = get_interface_info(get_ifname($lancfg['if']));

if (strcmp($lancfg['ipaddr'], "dhcp") == 0) {
	$pconfig['type'] = "DHCP";
	$pconfig['ipaddr'] = get_ipaddr($lancfg['if']);
	$pconfig['subnet'] = get_subnet_bits($lancfg['if']);
} else {
	$pconfig['type'] = "Static";
	$pconfig['ipaddr'] = $lancfg['ipaddr'];
	$pconfig['subnet'] = $lancfg['subnet'];
}
$pconfig['ipv6_enable'] = isset($lancfg['ipv6_enable']);
if (strcmp($lancfg['ipv6addr'], "auto") == 0) {
	$pconfig['ipv6type'] = "Auto";
	$pconfig['ipv6addr'] = get_ipv6addr($lancfg['if']);
} else {
	$pconfig['ipv6type'] = "Static";
	$pconfig['ipv6addr'] = $lancfg['ipv6addr'];
	$pconfig['ipv6subnet'] = $lancfg['ipv6subnet'];
}
$pconfig['gateway'] = get_defaultgateway();
$pconfig['ipv6gateway'] = get_ipv6defaultgateway();
$pconfig['mtu'] = $lancfg['mtu'];
$pconfig['media'] = $lancfg['media'];
$pconfig['mediaopt'] = $lancfg['mediaopt'];
$pconfig['polling'] = isset($lancfg['polling']);
$pconfig['extraoptions'] = $lancfg['extraoptions'];
if (!empty($ifinfo['wolevents']))
	$pconfig['wakeon'] = $lancfg['wakeon'];

/* Wireless interface? */
if (isset($lancfg['wireless'])) {
	require("interfaces_wlan.inc");
	wireless_config_init();
}

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	// Input validation.
	$reqdfields = array();
	$reqdfieldsn = array();
	$reqdfieldst = array();

	if ($_POST['type'] === "Static") {
		$reqdfields = explode(" ", "ipaddr subnet");
		$reqdfieldsn = array(gettext("IP address"),gettext("Subnet bit count"));

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

		if (($_POST['ipaddr'] && !is_ipv4addr($_POST['ipaddr'])))
			$input_errors[] = gettext("A valid IPv4 address must be specified.");
		if ($_POST['subnet'] && !filter_var($_POST['subnet'], FILTER_VALIDATE_INT, array('options' => array('min_range' => 1, 'max_range' => 32))))
			$input_errors[] = gettext("A valid network bit count (1-32) must be specified.");
	}

	if ($_POST['ipv6_enable'] && ($_POST['ipv6type'] === "Static")) {
		$reqdfields = explode(" ", "ipv6addr ipv6subnet");
		$reqdfieldsn = array(gettext("IPv6 address"),gettext("Prefix"));

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

		if (($_POST['ipv6addr'] && !is_ipv6addr($_POST['ipv6addr'])))
			$input_errors[] = gettext("A valid IPv6 address must be specified.");
		if ($_POST['ipv6subnet'] && !filter_var($_POST['ipv6subnet'], FILTER_VALIDATE_INT, array('options' => array('min_range' => 1, 'max_range' => 128))))
			$input_errors[] = gettext("A valid prefix (1-128) must be specified.");
		if (($_POST['ipv6gatewayr'] && !is_ipv6addr($_POST['ipv6gateway'])))
			$input_errors[] = gettext("A valid IPv6 Gateway address must be specified.");
	}

	// Wireless interface?
	if (isset($lancfg['wireless'])) {
		$wi_input_errors = wireless_config_post();
		if ($wi_input_errors) {
			if (is_array($input_errors))
				$input_errors = array_merge($input_errors, $wi_input_errors);
			else
				$input_errors = $wi_input_errors;
		}
	}

	if (!$input_errors) {
		if (0 == strcmp($_POST['type'],"Static")) {
			$lancfg['ipaddr'] = $_POST['ipaddr'];
			$lancfg['subnet'] = $_POST['subnet'];
			$lancfg['gateway'] = $_POST['gateway'];
		} else if (0 == strcmp($_POST['type'],"DHCP")) {
			$lancfg['ipaddr'] = "dhcp";
		}

		$lancfg['ipv6_enable'] = $_POST['ipv6_enable'] ? true : false;

		if (0 == strcmp($_POST['ipv6type'],"Static")) {
			$lancfg['ipv6addr'] = $_POST['ipv6addr'];
			$lancfg['ipv6subnet'] = $_POST['ipv6subnet'];
			$lancfg['ipv6gateway'] = $_POST['ipv6gateway'];
		} else if (0 == strcmp($_POST['ipv6type'],"Auto")) {
			$lancfg['ipv6addr'] = "auto";
		}

		$lancfg['mtu'] = $_POST['mtu'];
		$lancfg['media'] = $_POST['media'];
		$lancfg['mediaopt'] = $_POST['mediaopt'];
		$lancfg['polling'] = $_POST['polling'] ? true : false;
		$lancfg['extraoptions'] = $_POST['extraoptions'];
		if (!empty($ifinfo['wolevents']))
			$lancfg['wakeon'] = $_POST['wakeon'];

		write_config();
		touch($d_sysrebootreqd_path);
	}
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	var endis = !(document.iform.ipv6_enable.checked || enable_change);

	if (enable_change.name == "ipv6_enable") {
		endis = !enable_change.checked;

		document.iform.ipv6type.disabled = endis;
		document.iform.ipv6addr.disabled = endis;
		document.iform.ipv6subnet.disabled = endis;
		document.iform.ipv6gateway.disabled = endis;
	} else {
		document.iform.ipv6type.disabled = endis;
		document.iform.ipv6addr.disabled = endis;
		document.iform.ipv6subnet.disabled = endis;
		document.iform.ipv6gateway.disabled = endis;
	}

	ipv6_type_change();
}

function type_change() {
	switch (document.iform.type.selectedIndex) {
		case 0: /* Static */
			document.iform.ipaddr.disabled = 0;
			document.iform.subnet.disabled = 0;
			document.iform.gateway.disabled = 0;
			break;

		case 1: /* DHCP */
			document.iform.ipaddr.disabled = 1;
			document.iform.subnet.disabled = 1;
			document.iform.gateway.disabled = 1;
			break;
	}
}

function ipv6_type_change() {
	switch (document.iform.ipv6type.selectedIndex) {
		case 0: /* Static */
			var endis = !(document.iform.ipv6_enable.checked);

			document.iform.ipv6addr.disabled = endis;
			document.iform.ipv6subnet.disabled = endis;
			document.iform.ipv6gateway.disabled = endis;
			break;

		case 1: /* Autoconfigure */
			document.iform.ipv6addr.disabled = 1;
			document.iform.ipv6subnet.disabled = 1;
			document.iform.ipv6gateway.disabled = 1;
			break;
	}
}

function media_change() {
	switch (document.iform.media.value) {
		case "autoselect":
			showElementById('mediaopt_tr','hide');
			break;

		default:
			showElementById('mediaopt_tr','show');
			break;
	}
}

<?php if (isset($lancfg['wireless'])):?>
function encryption_change() {
	switch (document.iform.encryption.value) {
		case "none":
			showElementById('wep_key_tr','hide');
			showElementById('wpa_keymgmt_tr','hide');
			showElementById('wpa_pairwise_tr','hide');
			showElementById('wpa_psk_tr','hide');
			break;

		case "wep":
			showElementById('wep_key_tr','show');
			showElementById('wpa_keymgmt_tr','hide');
			showElementById('wpa_pairwise_tr','hide');
			showElementById('wpa_psk_tr','hide');
			break;

		case "wpa":
			showElementById('wep_key_tr','hide');
			showElementById('wpa_keymgmt_tr','show');
			showElementById('wpa_pairwise_tr','show');
			showElementById('wpa_psk_tr','show');
			break;
	}
}
<?php endif;?>
// -->
</script>
<form action="interfaces_lan.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
	  <tr>
	    <td class="tabcont">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if (file_exists($d_sysrebootreqd_path)) print_info_box(get_std_save_message(0));?>
			  <table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline(gettext("IPv4 Configuration"));?>
					<?php html_combobox("type", gettext("Type"), $pconfig['type'], array("Static" => gettext("Static"), "DHCP" => "DHCP"), "", true, false, "type_change()");?>
					<?php html_ipv4addrbox("ipaddr", "subnet", gettext("IP address"), $pconfig['ipaddr'], $pconfig['subnet'], "", true);?>
					<?php html_inputbox("gateway", gettext("Gateway"), $pconfig['gateway'], "", true, 20);?>
					<?php html_separator();?>
					<?php html_titleline_checkbox("ipv6_enable", gettext("IPv6 Configuration"), $pconfig['ipv6_enable'] ? true : false, gettext("Activate"), "enable_change(this)");?>
					<?php html_combobox("ipv6type", gettext("Type"), $pconfig['ipv6type'], array("Static" => gettext("Static"), "Auto" => "Auto"), "", true, false, "ipv6_type_change()");?>
					<?php html_ipv6addrbox("ipv6addr", "ipv6subnet", gettext("IP address"), $pconfig['ipv6addr'], $pconfig['ipv6subnet'], "", true);?>
					<?php html_inputbox("ipv6gateway", gettext("Gateway"), $pconfig['ipv6gateway'], "", true, 20);?>
					<?php html_separator();?>
					<?php html_titleline(gettext("Advanced Configuration"));?>
					<?php html_inputbox("mtu", gettext("MTU"), $pconfig['mtu'], gettext("Set the maximum transmission unit of the interface to n, default is interface specific. The MTU is used to limit the size of packets that are transmitted on an interface. Not all interfaces support setting the MTU, and some interfaces have range restrictions."), false, 5);?>
<!--
					<?php html_checkbox("polling", gettext("Device polling"), $pconfig['polling'] ? true : false, gettext("Enable device polling"), gettext("Device polling is a technique that lets the system periodically poll network devices for new data instead of relying on interrupts. This can reduce CPU load and therefore increase throughput, at the expense of a slightly higher forwarding delay (the devices are polled 1000 times per second). Not all NICs support polling."), false);?>
-->
					<?php html_combobox("media", gettext("Media"), $pconfig['media'], array("autoselect" => gettext("Autoselect"), "10baseT/UTP" => "10baseT/UTP", "100baseTX" => "100baseTX", "1000baseTX" => "1000baseTX", "1000baseSX" => "1000baseSX",), "", false, false, "media_change()");?>
					<?php html_combobox("mediaopt", gettext("Duplex"), $pconfig['mediaopt'], array("half-duplex" => "half-duplex", "full-duplex" => "full-duplex"), "", false);?>
					<?php if (!empty($ifinfo['wolevents'])):?>
					<?php $wakeonoptions = array("off" => gettext("Off"), "wol" => gettext("On")); foreach ($ifinfo['wolevents'] as $woleventv) { $wakeonoptions[$woleventv] = $woleventv; };?>
					<?php html_combobox("wakeon", gettext("Wake On LAN"), $pconfig['wakeon'], $wakeonoptions, "", false);?>
					<?php endif;?>
					<?php html_inputbox("extraoptions", gettext("Extra options"), $pconfig['extraoptions'], gettext("Extra options to ifconfig (usually empty)."), false, 40);?>
					<?php if (isset($lancfg['wireless'])) wireless_config_print();?>
			  </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>" onclick="enable_change(true)" />
				</div>
				<div id="remarks">
					<?php html_remark("warning", gettext("Warning"), sprintf(gettext("After you click &quot;Save&quot;, you may also have to do one or more of the following steps before you can access %s again: <ul><li>change the IP address of your computer</li><li>access the webGUI with the new IP address</li></ul>"), get_product_name()));?>
				</div>
			</td>
		</tr>
	</table>
	<?php include("formend.inc");?>
</form>
<script type="text/javascript">
<!--
type_change();
ipv6_type_change();
media_change();
enable_change(false);
<?php if (isset($lancfg['wireless'])):?>
encryption_change();
<?php endif;?>
//-->
</script>
<?php include("fend.inc");?>
