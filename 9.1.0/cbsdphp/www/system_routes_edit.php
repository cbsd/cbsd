#!/usr/local/bin/php
<?php 
/*
	system_routes_edit.php

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

$uuid = $_GET['uuid'];
if (isset($_POST['uuid']))
	$uuid = $_POST['uuid'];

$pgtitle = array(gettext("Network"), gettext("Static routes"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['staticroutes']['route']) || !is_array($config['staticroutes']['route']))
	$config['staticroutes']['route'] = array();

array_sort_key($config['staticroutes']['route'], "network");
$a_routes = &$config['staticroutes']['route'];

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_routes, "uuid")))) {
	$pconfig['uuid'] = $a_routes[$cnid]['uuid'];
	$pconfig['interface'] = $a_routes[$cnid]['interface'];
	list($pconfig['network'],$pconfig['network_subnet']) = 
		explode('/', $a_routes[$cnid]['network']);
	$pconfig['gateway'] = $a_routes[$cnid]['gateway'];
	$pconfig['descr'] = $a_routes[$cnid]['descr'];
} else {
	$pconfig['uuid'] = uuid();
	$pconfig['gateway'] = "";
	$pconfig['descr'] = "";
}

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: system_routes.php");
		exit;
	}

	// Input validation
	$reqdfields = explode(" ", "interface network network_subnet gateway");
	$reqdfieldsn = array(gettext("Interface"), gettext("Destination network"), gettext("Destination network bit count"), gettext("Gateway"));
	
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	
	if (($_POST['network'] && !is_ipaddr($_POST['network']))) {
		$input_errors[] = gettext("A valid destination network must be specified.");
	}
	
	if (($_POST['network'] && is_ipv4addr($_POST['network'])) && $_POST['network_subnet'])  {
		if (filter_var($_POST['network_subnet'], FILTER_VALIDATE_INT, array('options' => array('min_range' => 1, 'max_range' => 32))) == false)
			$input_errors[] = gettext("A valid IPv4 network bit count must be specified.");
	}

	if (($_POST['network'] && is_ipv6addr($_POST['network'])) && $_POST['network_subnet'])  {
		if (filter_var($_POST['network_subnet'], FILTER_VALIDATE_INT, array('options' => array('min_range' => 1, 'max_range' => 128))) == false)
			$input_errors[] = gettext("A valid IPv6 prefix must be specified.");
	}

	if (($_POST['gateway'] && !is_ipaddr($_POST['gateway']))) {
		$input_errors[] = gettext("A valid gateway IP address must be specified.");
	}
	
	if ($_POST['gateway'] && $_POST['network']) {
		if (is_ipv4addr($_POST['gateway']) && !is_ipv4addr($_POST['network'])) {
			$input_errors[] = gettext("You must enter the same IP type for network and gateway.");
		} else if (is_ipv6addr($_POST['gateway']) && !is_ipv6addr($_POST['network'])) {
			$input_errors[] = gettext("IP type mismatch for network and gateway.");
		}
	}
	
	// Check for overlaps
	// gen_subnet work for IPv4 only... This function permit to fix user input error for network number.
	if (is_ipv4addr($_POST['network'])) {
		$osn = gen_subnet($_POST['network'], $_POST['network_subnet']) . "/" . $_POST['network_subnet'];
	} else {
		$osn = $_POST['network'] . "/" . $_POST['network_subnet'] ;
	}

	$index = array_search_ex($osn, $a_routes, "network");
	if (FALSE !== $index) {
		if (!((FALSE !== $cnid) && ($a_routes[$cnid]['uuid'] === $a_routes[$index]['uuid']))) {
			$input_errors[] = gettext("A route to this destination network already exists.");
		}
	}

	if (!$input_errors) {
		$route = array();
		$route['uuid'] = $_POST['uuid'];
		$route['interface'] = $_POST['interface'];
		$route['network'] = $osn;
		$route['gateway'] = $_POST['gateway'];
		$route['descr'] = $_POST['descr'];

		if (isset($uuid) && (FALSE !== $cnid)) {
			$a_routes[$cnid] = $route;
			$mode = UPDATENOTIFY_MODE_MODIFIED;
		} else {
			$a_routes[] = $route;
			$mode = UPDATENOTIFY_MODE_NEW;
		}
		
		updatenotify_set("routes", $mode, $route['uuid']);
		write_config();
		
		header("Location: system_routes.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabcont">
      <form action="system_routes_edit.php" method="post" name="iform" id="iform">
      	<?php if ($input_errors) print_input_errors($input_errors); ?>
        <table width="100%" border="0" cellpadding="6" cellspacing="0">
          <?php $interfaces = array('lan' => 'LAN'); for ($i = 1; isset($config['interfaces']['opt' . $i]); $i++) { $interfaces['opt' . $i] = $config['interfaces']['opt' . $i]['descr']; }?>
          <?php html_combobox("interface", gettext("Interface"), $pconfig['interface'], $interfaces, gettext("Choose which interface this route applies to."), true);?>
          <tr>
            <td width="22%" valign="top" class="vncellreq"><?=gettext("Destination network");?></td>
            <td width="78%" class="vtable"> 
							<input name="network" type="text" class="formfld" id="network" size="20" value="<?=htmlspecialchars($pconfig['network']);?>" /> 
							/
							<select name="network_subnet" class="formfld" id="network_subnet">
								<?php for ($i = 32; $i > 0; $i--):?>
								<option value="<?=$i;?>" <?php if ($i == $pconfig['network_subnet']) echo "selected=\"selected\"";?>><?=$i;?></option>
								<?php endfor;?>
							</select>
							<br /><span class="vexpl"><?=gettext("Destination network for this static route");?></span>
						</td>
          </tr>
          <?php html_inputbox("gateway", gettext("Gateway"), $pconfig['gateway'], gettext("Gateway to be used to reach the destination network."), true, 40);?>
          <?php html_inputbox("descr", gettext("Description"), $pconfig['descr'], gettext("You may enter a description here for your reference."), false, 40);?>
        </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=(isset($uuid) && (FALSE !== $cnid)) ? gettext("Save") : gettext("Add")?>" />
					<input name="Cancel" type="submit" class="formbtn" value="<?=gettext("Cancel");?>" />
					<input name="uuid" type="hidden" value="<?=$pconfig['uuid'];?>" />
			  </div>
			  <?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
