#!/usr/local/bin/php
<?php
/*
	interfaces_lagg_edit.php

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

$pgtitle = array(gettext("Network"), gettext("Interface Management"), gettext("Link Aggregation and Failover"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['vinterfaces']['lagg']) || !is_array($config['vinterfaces']['lagg']))
	$config['vinterfaces']['lagg'] = array();

$a_lagg = &$config['vinterfaces']['lagg'];
array_sort_key($a_lagg, "if");

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_lagg, "uuid")))) {
	$pconfig['enable'] = isset($a_lagg[$cnid]['enable']);
	$pconfig['uuid'] = $a_lagg[$cnid]['uuid'];
	$pconfig['if'] = $a_lagg[$cnid]['if'];
	$pconfig['laggproto'] = $a_lagg[$cnid]['laggproto'];
	$pconfig['laggport'] = $a_lagg[$cnid]['laggport'];
	$pconfig['desc'] = $a_lagg[$cnid]['desc'];
} else {
	$pconfig['enable'] = true;
	$pconfig['uuid'] = uuid();
	$pconfig['if'] = "lagg" . get_nextlagg_id();
	$pconfig['laggproto'] = "failover";
	$pconfig['laggport'] = array();
	$pconfig['desc'] = "";
}

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: interfaces_lagg.php");
		exit;
	}

	// Input validation.
	$reqdfields = explode(" ", "laggproto");
	$reqdfieldsn = array(gettext("Aggregation protocol"));
	$reqdfieldst = explode(" ", "string");

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if (count($_POST['laggport']) < 1)
		$input_errors[] = gettext("There must be selected a minimum of 1 interface.");

	if (!$input_errors) {
		$lagg = array();
		$lagg['enable'] = $_POST['enable'] ? true : false;
		$lagg['uuid'] = $_POST['uuid'];
		$lagg['if'] = $_POST['if'];
		$lagg['laggproto'] = $_POST['laggproto'];
		$lagg['laggport'] = $_POST['laggport'];
		$lagg['desc'] = $_POST['desc'];

		if (isset($uuid) && (FALSE !== $cnid)) {
			$a_lagg[$cnid] = $lagg;
		} else {
			$a_lagg[] = $lagg;
		}

		write_config();
		touch($d_sysrebootreqd_path);

		header("Location: interfaces_lagg.php");
		exit;
	}
}

function get_nextlagg_id() {
	global $config;

	$id = 0;
	$a_lagg = $config['vinterfaces']['lagg'];

	if (false !== array_search_ex("lagg" . strval($id), $a_lagg, "if")) {
		do {
			$id++; // Increase ID until a unused one is found.
		} while (false !== array_search_ex("lagg" . strval($id), $a_lagg, "if"));
	}

	return $id;
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
		  <ul id="tabnav">
				<li class="tabinact"><a href="interfaces_assign.php"><span><?=gettext("Management");?></span></a></li>
				<li class="tabinact"><a href="interfaces_vlan.php"><span><?=gettext("VLAN");?></span></a></li>
				<li class="tabact"><a href="interfaces_lagg.php" title="<?=gettext("Reload page");?>"><span><?=gettext("LAGG");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="interfaces_lagg_edit.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_inputbox("if", gettext("Interface"), $pconfig['if'], "", true, 5, true);?>
					<?php html_combobox("laggproto", gettext("Aggregation protocol"), $pconfig['laggproto'], array("failover" => gettext("Failover"), "fec" => gettext("FEC (Fast EtherChannel)"), "lacp" => gettext("LACP (Link Aggregation Control Protocol)"), "loadbalance" => gettext("Loadbalance"), "roundrobin" => gettext("Roundrobin"), "none" => gettext("None")), "", true);?>
					<?php $a_port = array(); foreach (get_interface_list() as $ifk => $ifv) { if (eregi('lagg', $ifk)) { continue; } if (!(isset($uuid) && (FALSE !== $cnid)) && false !== array_search_ex($ifk, $a_lagg, "laggport")) { continue; } $a_port[$ifk] = htmlspecialchars("{$ifk} ({$ifv['mac']})"); } ?>
					<?php html_listbox("laggport", gettext("Ports"), $pconfig['laggport'], $a_port, gettext("Note: Ctrl-click (or command-click on the Mac) to select multiple entries."), true);?>
					<?php html_inputbox("desc", gettext("Description"), $pconfig['desc'], gettext("You may enter a description here for your reference."), false, 40);?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=(isset($uuid) && (FALSE !== $cnid)) ? gettext("Save") : gettext("Add")?>" />
					<input name="Cancel" type="submit" class="formbtn" value="<?=gettext("Cancel");?>" />
					<input name="enable" type="hidden" value="<?=$pconfig['enable'];?>" />
					<input name="if" type="hidden" value="<?=$pconfig['if'];?>" />
					<input name="uuid" type="hidden" value="<?=$pconfig['uuid'];?>" />
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
