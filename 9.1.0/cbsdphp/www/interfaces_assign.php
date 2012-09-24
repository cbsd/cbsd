#!/usr/local/bin/php
<?php
/*
	interfaces_assign.php

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

$pgtitle = array(gettext("Network"), gettext("Interface Management"));

/*
	In this file, "port" refers to the physical port name,
	while "interface" refers to LAN, WAN, or OPTn.
*/

/* get list without VLAN interfaces */
$portlist = get_interface_list();

// Add VLAN interfaces.
if (is_array($config['vinterfaces']['vlan']) && count($config['vinterfaces']['vlan'])) {
	foreach ($config['vinterfaces']['vlan'] as $vlanv) {
		$portlist[$vlanv['if']] = $vlanv;
		$portlist[$vlanv['if']]['isvirtual'] = true;
	}
}

// Add LAGG interfaces.
if (is_array($config['vinterfaces']['lagg']) && count($config['vinterfaces']['lagg'])) {
	foreach ($config['vinterfaces']['lagg'] as $laggv) {
		$portlist[$laggv['if']] = $laggv;
		$portlist[$laggv['if']]['isvirtual'] = true;
	}
}

if ($_POST) {
	unset($input_errors);

	/* Build a list of the port names so we can see how the interfaces map */
	$portifmap = array();
	foreach ($portlist as $portname => $portinfo)
		$portifmap[$portname] = array();

	/* Go through the list of ports selected by the user,
	   build a list of port-to-interface mappings in portifmap */
	foreach ($_POST as $ifname => $ifport) {
		if (($ifname == 'lan') || (substr($ifname, 0, 3) == 'opt'))
			$portifmap[$ifport][] = strtoupper($ifname);
	}

	/* Deliver error message for any port with more than one assignment */
	foreach ($portifmap as $portname => $ifnames) {
		if (count($ifnames) > 1) {
			$errstr = gettext("Port ") . $portname .
				gettext(" was assigned to ") . count($ifnames) .
				gettext(" interfaces:");

			foreach ($portifmap[$portname] as $ifn)
				$errstr .= " " . $ifn;

			$input_errors[] = $errstr;
		}
	}

	if (!$input_errors) {
		/* No errors detected, so update the config */
		foreach ($_POST as $ifname => $ifport) {
			if (($ifname == 'lan') || (substr($ifname, 0, 3) == 'opt')) {
				if (!is_array($ifport)) {
					$config['interfaces'][$ifname]['if'] = $ifport;

					/* check for wireless interfaces, set or clear ['wireless'] */
					if (preg_match($g['wireless_regex'], $ifport)) {
						if (!is_array($config['interfaces'][$ifname]['wireless']))
							$config['interfaces'][$ifname]['wireless'] = array();
					} else {
						unset($config['interfaces'][$ifname]['wireless']);
					}

					/* make sure there is a name for OPTn */
					if (substr($ifname, 0, 3) == 'opt') {
						if (!isset($config['interfaces'][$ifname]['descr']))
							$config['interfaces'][$ifname]['descr'] = strtoupper($ifname);
					}
				}
			}
		}

		write_config();
		touch($d_sysrebootreqd_path);
	}
}

if ($_GET['act'] == "del") {
	$id = $_GET['id'];

	$ifn = $config['interfaces'][$id]['if'];
	// Stop interface.
	rc_exec_service("netif stop {$ifn}");
	// Remove ifconfig_xxx and ipv6_ifconfig_xxx entries.
	mwexec("/usr/local/sbin/rconf attribute remove 'ifconfig_{$ifn}'");
	mwexec("/usr/local/sbin/rconf attribute remove 'ipv6_ifconfig_{$ifn}'");

	unset($config['interfaces'][$id]);	/* delete the specified OPTn */

	/* shift down other OPTn interfaces to get rid of holes */
	$i = substr($id, 3); /* the number of the OPTn port being deleted */
	$i++;

	/* look at the following OPTn ports */
	while (is_array($config['interfaces']['opt' . $i])) {
		$config['interfaces']['opt' . ($i - 1)] =
			$config['interfaces']['opt' . $i];

		if ($config['interfaces']['opt' . ($i - 1)]['descr'] == "OPT" . $i)
			$config['interfaces']['opt' . ($i - 1)]['descr'] = "OPT" . ($i - 1);

		unset($config['interfaces']['opt' . $i]);
		$i++;
	}

	write_config();
	touch($d_sysrebootreqd_path);
	header("Location: interfaces_assign.php");
	exit;
}

if ($_GET['act'] == "add") {
	/* find next free optional interface number */
	$i = 1;
	while (is_array($config['interfaces']['opt' . $i]))
		$i++;

	$newifname = 'opt' . $i;
	$config['interfaces'][$newifname] = array();
	$config['interfaces'][$newifname]['descr'] = "OPT" . $i;

	// Set IPv4 to 'DHCP' and IPv6 to 'Auto' per default.
	$config['interfaces'][$newifname]['ipaddr'] = "dhcp";
	$config['interfaces'][$newifname]['ipv6addr'] = "auto";

	/* Find an unused port for this interface */
	foreach ($portlist as $portname => $portinfo) {
		$portused = false;
		foreach ($config['interfaces'] as $ifname => $ifdata) {
			if ($ifdata['if'] == $portname) {
				$portused = true;
				break;
			}
		}
		if (!$portused) {
			$config['interfaces'][$newifname]['if'] = $portname;
			if (preg_match($g['wireless_regex'], $portname))
				$config['interfaces'][$newifname]['wireless'] = array();
			break;
		}
	}

	write_config();
	touch($d_sysrebootreqd_path);
	header("Location: interfaces_assign.php");
	exit;
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
		  <ul id="tabnav">
				<li class="tabact"><a href="interfaces_assign.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Management");?></span></a></li>
				<li class="tabinact"><a href="interfaces_vlan.php"><span><?=gettext("VLAN");?></span></a></li>
				<li class="tabinact"><a href="interfaces_lagg.php"><span><?=gettext("LAGG");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="interfaces_assign.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if (file_exists($d_sysrebootreqd_path)) print_info_box(get_std_save_message(0));?>
				<table border="0" cellpadding="0" cellspacing="0">
					<tr>
						<td class="listhdrlr"><?=gettext("Interface");?></td>
						<td class="listhdrr"><?=gettext("Network port");?></td>
						<td class="list">&nbsp;</td>
					</tr>
					<?php foreach ($config['interfaces'] as $ifname => $iface):
					if ($iface['descr'])
						$ifdescr = $iface['descr'];
					else
						$ifdescr = strtoupper($ifname);
					?>
					<tr>
						<td class="listlr" valign="middle"><strong><?=$ifdescr;?></strong></td>
					  <td valign="middle" class="listr">
							<select name="<?=$ifname;?>" class="formfld" id="<?=$ifname;?>">
							  <?php foreach ($portlist as $portname => $portinfo):?>
							  <option value="<?=$portname;?>" <?php if ($portname == $iface['if']) echo "selected=\"selected\"";?>>
							  	<?php
									if ($portinfo['isvirtual']) {
										$descr = $portinfo['if'];
										if ($portinfo['desc']) {
											$descr .= " ({$portinfo['desc']})";
										}
										echo htmlspecialchars($descr);
									} else {
										echo htmlspecialchars($portname . " (" . $portinfo['mac'] . ")");
									}
							  	?>
							  </option>
							  <?php endforeach;?>
							</select>
						</td>
						<td valign="middle" class="list">
							<?php if (($ifname != 'lan') && ($ifname != 'wan')):?>
							<a href="interfaces_assign.php?act=del&amp;id=<?=$ifname;?>"><img src="x.gif" title="<?=gettext("Delete interface");?>" border="0" alt="<?=gettext("Delete interface");?>" /></a>
							<?php endif;?>
						</td>
					</tr>
				  <?php endforeach;?>
				  <?php if (count($config['interfaces']) < count($portlist)):?>
				  <tr>
						<td class="list" colspan="2"></td>
						<td class="list" nowrap="nowrap">
							<a href="interfaces_assign.php?act=add"><img src="plus.gif" title="<?=gettext("Add interface");?>" border="0" alt="<?=gettext("Add interface");?>" /></a>
						</td>
				  </tr>
				  <?php else:?>
				  <tr>
					<td class="list" colspan="3" height="10"></td>
				  </tr>
				  <?php endif;?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>" />
				</div>
				<div id="remarks">
					<?php html_remark("warning", gettext("Warning"), sprintf(gettext("After you click &quot;Save&quot;, you must reboot %s to make the changes take effect. You may also have to do one or more of the following steps before you can access your NAS again: <ul><li><span class='vexpl'>change the IP address of your computer</span></li><li><span class='vexpl'>access the webGUI with the new IP address</span></li></ul>"), get_product_name()));?>
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
