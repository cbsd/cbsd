#!/usr/local/bin/php
<?php
/*
	system_firewall.php

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
require_once("XML/Serializer.php");
require_once("XML/Unserializer.php");

$pgtitle = array(gettext("Network"), gettext("Firewall"));

$pconfig['enable'] = isset($config['system']['firewall']['enable']);

if ($_POST['export']) {
	$options = array(
		XML_SERIALIZER_OPTION_XML_DECL_ENABLED => true,
		XML_SERIALIZER_OPTION_INDENT           => "\t",
		XML_SERIALIZER_OPTION_LINEBREAKS       => "\n",
		XML_SERIALIZER_OPTION_XML_ENCODING     => "UTF-8",
		XML_SERIALIZER_OPTION_ROOT_NAME        => get_product_name(),
		XML_SERIALIZER_OPTION_ROOT_ATTRIBS     => array("version" => get_product_version(), "revision" => get_product_revision()),
		XML_SERIALIZER_OPTION_DEFAULT_TAG      => "rule",
		XML_SERIALIZER_OPTION_MODE             => XML_SERIALIZER_MODE_DEFAULT,
		XML_SERIALIZER_OPTION_IGNORE_FALSE     => true,
		XML_SERIALIZER_OPTION_CONDENSE_BOOLS   => true,
	);

	$serializer = &new XML_Serializer($options);
	$status = $serializer->serialize($config['system']['firewall']['rule']);

	if (PEAR::isError($status)) {
		$errormsg = $status->getMessage();
	} else {
		$ts = date("YmdHis");
		$fn = "firewall-{$config['system']['hostname']}.{$config['system']['domain']}-{$ts}.rules";
		$data = $serializer->getSerializedData();
		$fs = strlen($data);

		header("Content-Type: application/octet-stream");
		header("Content-Disposition: attachment; filename={$fn}");
		header("Content-Length: {$fs}");
		header("Pragma: hack");
		echo $data;

		exit;
	}
} else if ($_POST['import']) {
	if (is_uploaded_file($_FILES['rulesfile']['tmp_name'])) {
		$options = array(
			XML_UNSERIALIZER_OPTION_COMPLEXTYPE => 'array',
			XML_UNSERIALIZER_OPTION_ATTRIBUTES_PARSE => true,
			XML_UNSERIALIZER_OPTION_FORCE_ENUM  => $listtags,
		);

		$unserializer = &new XML_Unserializer($options);
		$status = $unserializer->unserialize($_FILES['rulesfile']['tmp_name'], true);

		if (PEAR::isError($status)) {
			$errormsg = $status->getMessage();
		} else {
			// Take care array already exists.
			if (!is_array($config['system']['firewall']['rule']))
				$config['system']['firewall']['rule'] = array();

			$data = $unserializer->getUnserializedData();

			// Import rules.
			foreach ($data['rule'] as $rule) {
				// Check if rule already exists.
				$index = array_search_ex($rule['uuid'], $config['system']['firewall']['rule'], "uuid");
				if (false !== $index) {
					// Create new uuid and mark rule as duplicate (modify description).
					$rule['uuid'] = uuid();
					$rule['desc'] = gettext("*** Imported duplicate ***") . " {$rule['desc']}";
				}
				$config['system']['firewall']['rule'][] = $rule;

				updatenotify_set("firewall", UPDATENOTIFY_MODE_NEW, $rule['uuid']);
			}

			write_config();

			header("Location: system_firewall.php");
			exit;
		}
	} else {
		$errormsg = sprintf("%s %s", gettext("Failed to upload file."),
			$g_file_upload_error[$_FILES['rulesfile']['error']]);
	}
} else if ($_POST) {
	$pconfig = $_POST;

	$config['system']['firewall']['enable'] = $_POST['enable'] ? true : false;

	write_config();

	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		$retval |= updatenotify_process("firewall", "firewall_process_updatenotification");
		config_lock();
		$retval |= rc_update_service("ipfw");
		config_unlock();
	}
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		updatenotify_delete("firewall");
	}
}

if (!isset($config['system']['firewall']['rule']) || !is_array($config['system']['firewall']['rule']))
	$config['system']['firewall']['rule'] = array();


array_sort_key($config['system']['firewall']['rule'], "ruleno");
$a_rule = &$config['system']['firewall']['rule'];

if ($_GET['act'] === "del") {
	if ($_GET['uuid'] === "all") {
		foreach ($a_rule as $rulek => $rulev) {
			updatenotify_set("firewall", UPDATENOTIFY_MODE_DIRTY, $a_rule[$rulek]['uuid']);
		}
	} else {
		updatenotify_set("firewall", UPDATENOTIFY_MODE_DIRTY, $_GET['uuid']);
	}
	header("Location: system_firewall.php");
	exit;
}

function firewall_process_updatenotification($mode, $data) {
	global $config;

	$retval = 0;

	switch ($mode) {
		case UPDATENOTIFY_MODE_NEW:
		case UPDATENOTIFY_MODE_MODIFIED:
			break;
		case UPDATENOTIFY_MODE_DIRTY:
			$cnid = array_search_ex($data, $config['system']['firewall']['rule'], "uuid");
			if (false !== $cnid) {
				unset($config['system']['firewall']['rule'][$cnid]);
				write_config();
			}
			break;
	}

	return $retval;
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	var endis = !(document.iform.enable.checked || enable_change);
}
//-->
</script>
<form action="system_firewall.php" method="post" name="iform" id="iform" enctype="multipart/form-data">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
		<tr>
			<td class="tabcont">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($errormsg) print_error_box($errormsg);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<?php if (updatenotify_exists("firewall")) print_config_change_box();?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gettext("System firewall"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
					<tr>
						<td width="22%" valign="top" class="vncell"><?=gettext("Rules");?></td>
						<td width="78%" class="vtable">
							<table width="100%" border="0" cellpadding="0" cellspacing="0">
								<tr>
									<td width="4%" class="listhdrlr">&nbsp;</td>
									<td width="5%" class="listhdrr"><?=gettext("Proto");?></td>
									<td width="20%" class="listhdrr"><?=gettext("Source");?></td>
									<td width="5%" class="listhdrr"><?=gettext("Port");?></td>
									<td width="20%" class="listhdrr"><?=gettext("Destination");?></td>
									<td width="5%" class="listhdrr"><?=gettext("Port");?></td>
									<td width="5%" class="listhdrr"><?=htmlspecialchars(gettext("<->"));?></td>
									<td width="26%" class="listhdrr"><?=gettext("Description");?></td>
									<td width="10%" class="list"></td>
								</tr>
								<?php foreach ($a_rule as $rule):?>
								<?php $notificationmode = updatenotify_get_mode("firewall", $rule['uuid']);?>
								<tr>
									<?php $enable = isset($rule['enable']);
									switch ($rule['action']) {
										case "allow":
											$actionimg = "fw_action_allow.gif";
											break;
										case "deny":
											$actionimg = "fw_action_deny.gif";
											break;
										case "unreach host":
											$actionimg = "fw_action_reject.gif";
											break;
									}
									?>
									<td class="<?=$enable?"listlr":"listlrd";?>"><img src="<?=$actionimg;?>" alt="" /></td>
									<td class="<?=$enable?"listr":"listrd";?>"><?=strtoupper($rule['protocol']);?>&nbsp;</td>
									<td class="<?=$enable?"listr":"listrd";?>"><?=htmlspecialchars(empty($rule['src']) ? "*" : $rule['src']);?>&nbsp;</td>
									<td class="<?=$enable?"listr":"listrd";?>"><?=htmlspecialchars(empty($rule['srcport']) ? "*" : $rule['srcport']);?>&nbsp;</td>
									<td class="<?=$enable?"listr":"listrd";?>"><?=htmlspecialchars(empty($rule['dst']) ? "*" : $rule['dst']);?>&nbsp;</td>
									<td class="<?=$enable?"listr":"listrd";?>"><?=htmlspecialchars(empty($rule['dstport']) ? "*" : $rule['dstport']);?>&nbsp;</td>
									<td class="<?=$enable?"listrc":"listrcd";?>"><?=empty($rule['direction']) ? "*" : strtoupper($rule['direction']);?>&nbsp;</td>
									<td class="listbg"><?=htmlspecialchars($rule['desc']);?>&nbsp;</td>
									<?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
									<td valign="middle" nowrap="nowrap" class="list">
										<a href="system_firewall_edit.php?uuid=<?=$rule['uuid'];?>"><img src="e.gif" title="<?=gettext("Edit rule");?>" border="0" alt="<?=gettext("Edit rule");?>" /></a>
										<a href="system_firewall.php?act=del&amp;uuid=<?=$rule['uuid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this rule?");?>')"><img src="x.gif" title="<?=gettext("Delete rule");?>" border="0" alt="<?=gettext("Delete rule");?>" /></a>
									</td>
									<?php else:?>
									<td valign="middle" nowrap="nowrap" class="list">
										<img src="del.gif" border="0" alt="" />
									</td>
									<?php endif;?>
								</tr>
								<?php endforeach;?>
								<tr>
									<td class="list" colspan="8"></td>
									<td class="list">
										<a href="system_firewall_edit.php"><img src="plus.gif" title="<?=gettext("Add rule");?>" border="0" alt="<?=gettext("Add rule");?>" /></a>
										<?php if (!empty($a_rule)):?>
										<a href="system_firewall.php?act=del&amp;uuid=all" onclick="return confirm('<?=gettext("Do you really want to delete all rules?");?>')"><img src="x.gif" title="<?=gettext("Delete all rules");?>" border="0" alt="<?=gettext("Delete all rules");?>" /></a>
										<?php endif;?>
									</td>
								</tr>
							</table>
						</td>
					</tr>
					<tr>
						<td width="22%" valign="top" class="vncell">&nbsp;</td>
						<td width="78%" class="vtable">
							<?=gettext("Download firewall rules.");?><br />
							<div id="submit">
								<input name="export" type="submit" class="formbtn" value="<?=gettext("Export");?>" /><br />
							</div>
						</td>
					</tr>
					<tr>
						<td width="22%" valign="top" class="vncell">&nbsp;</td>
						<td width="78%" class="vtable">
							<?=gettext("Import firewall rules.");?><br />
							<div id="submit">
								<input name="rulesfile" type="file" class="formfld" id="rulesfile" size="40" accept="*.rules" />&nbsp;
								<input name="import" type="submit" class="formbtn" id="import" value="<?=gettext("Import");?>" /><br />
							</div>
						</td>
					</tr>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save and Restart");?>" />
				</div>
			</td>
		</tr>
	</table>
	<?php include("formend.inc");?>
</form>
<?php include("fend.inc");?>
