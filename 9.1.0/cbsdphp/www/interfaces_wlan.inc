<?php
/*
	interfaces_wlan.inc

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	All rights reserved.
	
	portions of m0n0wall (http://m0n0.ch/wall).
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
require("interfaces.inc");

function wireless_config_init() {
	global $optcfg, $pconfig;

	$pconfig['standard'] = $optcfg['wireless']['standard'];
	$pconfig['ssid'] = $optcfg['wireless']['ssid'];
	$pconfig['channel'] = $optcfg['wireless']['channel'];

	$pconfig['encryption'] = "none";
	if (isset($optcfg['wireless']['wep']['enable'])) {
		$pconfig['encryption'] = "wep";
	} else if (isset($optcfg['wireless']['wpa']['enable'])) {
		$pconfig['encryption'] = "wpa";
	}

	if (is_array($optcfg['wireless']['wep'])) {
		$pconfig['wep_key'] = $optcfg['wireless']['wep']['key'];
	}

	if (is_array($optcfg['wireless']['wpa'])) {
		$pconfig['wpa_keymgmt'] = $optcfg['wireless']['wpa']['keymgmt'];
		$pconfig['wpa_pairwise'] = $optcfg['wireless']['wpa']['pairwise'];
		$pconfig['wpa_psk'] = $optcfg['wireless']['wpa']['psk'];
	}
}

function wireless_config_post() {
	global $optcfg, $pconfig;

	unset($input_errors);

	// Input validation
	if ($_POST['enable']) {
		$reqdfields = explode(" ", "standard ssid channel");
		$reqdfieldsn = array(gettext("Standard"), gettext("SSID"), gettext("Channel"));

		switch ($_POST['encryption']) {
			case "wep":
				$reqdfields = array_merge($reqdfields, array("wep_key"));
				$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Key")));
				break;

			case "wpa":
				$reqdfields = array_merge($reqdfields, array("wpa_psk"));
				$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("PSK")));
				break;

			default;
				break;
		}

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	}

	if (!$input_errors) {
		$optcfg['wireless']['standard'] = $_POST['standard'];
		$optcfg['wireless']['ssid'] = $_POST['ssid'];
		$optcfg['wireless']['channel'] = $_POST['channel'];
		$optcfg['wireless']['wep']['key'] = $_POST['wep_key'];
		$optcfg['wireless']['wpa']['keymgmt'] = $_POST['wpa_keymgmt'];
		$optcfg['wireless']['wpa']['pairwise'] = $_POST['wpa_pairwise'];
		$optcfg['wireless']['wpa']['psk'] = $_POST['wpa_psk'];

		switch ($_POST['encryption']) {
			case "wep":
				$optcfg['wireless']['wep']['enable'] = true;
				$optcfg['wireless']['wpa']['enable'] = false;
				break;

			case "wpa":
				$optcfg['wireless']['wep']['enable'] = false;
				$optcfg['wireless']['wpa']['enable'] = true;
				break;

			default:
				$optcfg['wireless']['wep']['enable'] = false;
				$optcfg['wireless']['wpa']['enable'] = false;
				break;
		}
	}

	return $input_errors;
}

function wireless_config_print() {
	global $optcfg, $pconfig;
?>
								<?php html_separator();?>
								<?php html_titleline(gettext("Wireless configuration"));?>
								<?php $a_standard = array(); foreach (interfaces_wireless_get_standards($optcfg['if']) as $standardv) { $a_standard[$standardv] = "802.{$standardv}"; }?>
								<?php html_combobox("standard", gettext("Standard"), $pconfig['standard'], $a_standard, "", true);?>
								<?php html_inputbox("ssid", gettext("SSID"), $pconfig['ssid'], gettext("Set the desired Service Set Identifier (aka network name)."), true, 20);?>
								<tr>
									<td valign="top" class="vncellreq"><?=gettext("Channel");?></td>
									<td class="vtable">
										<select name="channel" class="formfld" id="channel">
											<option <?php if ("any" === $pconfig['channel']) echo "selected";?> value="any"><?=gettext("Auto");?></option>
											<?php foreach (interfaces_wireless_get_channellist($optcfg['if']) as $channelk => $channelv):?>
											<?php if ($channelv['mode'] === "11g") $mode = "11b/g"; else $mode = $channelv['mode'];?>
											<option <?php if ($channelk == $pconfig['channel']) echo "selected";?> value="<?=$channelk;?>"><?="{$channelv['channel']} ({$channelv['freq']} MHz, {$mode})";?></option>
											<?php endforeach;?>
										</select>
									</td>
								</tr>
								<?php html_combobox("encryption", gettext("Encryption"), $pconfig['encryption'], array("none" => gettext("None"), "wep" => gettext("WEP"), "wpa" => gettext("WPA")), "", true, false, "encryption_change()");?>
								<?php html_passwordbox("wep_key", gettext("Key"), $pconfig['wep_key'], gettext("A key will be either 5 or 13 characters (40 or 104 bits) depending of the local network and the capabilities of the adaptor. It may be specified either as a plain string or as a string of hexadecimal digits preceded by '0x'."), true, 40);?>
								<?php html_combobox("wpa_keymgmt", gettext("Key Management Protocol"), $pconfig['wpa_keymgmt'], array("WPA-PSK" => gettext("WPA-PSK (Pre Shared Key)"), "WPA-EAP" => gettext("WPA-EAP (Extensible Authentication Protocol)"), "WPA-PSK WPA-EAP" => gettext("WPA-PSK WPA-EAP")), "", true);?>
								<?php html_combobox("wpa_pairwise", gettext("Pairwise"), $pconfig['wpa_pairwise'], array("CCMP TKIP" => gettext("CCMP TKIP"), "CCMP" => gettext("CCMP"), "TKIP" => gettext("TKIP")), gettext("List of acceptable pairwise (unicast) ciphers for WPA. One or more of: CCMP (AES in Counter mode with CBC-MAC, RFC 3610, IEEE 802.11i/D7.0), TKIP (Temporal Key Integrity Protocol, IEEE 802.11i/D7.0)."), true);?>
								<?php html_passwordbox("wpa_psk", gettext("PSK"), $pconfig['wpa_psk'], gettext("Enter the passphrase that will be used in WPA-PSK mode. This must be between 8 and 63 characters long."), true, 40);?>
<?php } ?>
