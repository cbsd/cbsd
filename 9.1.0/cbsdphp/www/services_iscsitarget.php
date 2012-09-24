#!/usr/local/bin/php
<?php
/*
	services_iscsitarget.php
	
	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	All rights reserved.
	
	Portions of m0n0wall (http://m0n0.ch/wall)
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

$pgtitle = array(gettext("Services"), gettext("iSCSI Target"));

if (!isset($config['iscsitarget']['portalgroup']) || !is_array($config['iscsitarget']['portalgroup']))
	$config['iscsitarget']['portalgroup'] = array();

if (!isset($config['iscsitarget']['initiatorgroup']) || !is_array($config['iscsitarget']['initiatorgroup']))
	$config['iscsitarget']['initiatorgroup'] = array();

if (!isset($config['iscsitarget']['authgroup']) || !is_array($config['iscsitarget']['authgroup']))
	$config['iscsitarget']['authgroup'] = array();

function cmp_tag($a, $b) {
	if ($a['tag'] == $b['tag'])
		return 0;
	return ($a['tag'] > $b['tag']) ? 1 : -1;
}
usort($config['iscsitarget']['portalgroup'], "cmp_tag");
usort($config['iscsitarget']['initiatorgroup'], "cmp_tag");
usort($config['iscsitarget']['authgroup'], "cmp_tag");

$pconfig['enable'] = isset($config['iscsitarget']['enable']);
$pconfig['nodebase'] = $config['iscsitarget']['nodebase'];
$pconfig['discoveryauthmethod'] = $config['iscsitarget']['discoveryauthmethod'];
$pconfig['discoveryauthgroup'] = $config['iscsitarget']['discoveryauthgroup'];
$pconfig['timeout'] = $config['iscsitarget']['timeout'];
$pconfig['nopininterval'] = $config['iscsitarget']['nopininterval'];
$pconfig['maxr2t'] = $config['iscsitarget']['maxr2t'];
$pconfig['maxsessions'] = $config['iscsitarget']['maxsessions'];
$pconfig['maxconnections'] = $config['iscsitarget']['maxconnections'];
$pconfig['firstburstlength'] = $config['iscsitarget']['firstburstlength'];
$pconfig['maxburstlength'] = $config['iscsitarget']['maxburstlength'];
$pconfig['maxrecvdatasegmentlength'] = $config['iscsitarget']['maxrecvdatasegmentlength'];
$pconfig['maxoutstandingr2t'] = $config['iscsitarget']['maxoutstandingr2t'];
$pconfig['defaulttime2wait'] = $config['iscsitarget']['defaulttime2wait'];
$pconfig['defaulttime2retain'] = $config['iscsitarget']['defaulttime2retain'];

$pconfig['uctlenable'] = isset($config['iscsitarget']['uctlenable']);
$pconfig['uctladdress'] = $config['iscsitarget']['uctladdress'];
$pconfig['uctlport'] = $config['iscsitarget']['uctlport'];
$pconfig['uctlnetmask'] = $config['iscsitarget']['uctlnetmask'];
$pconfig['uctlauthmethod'] = $config['iscsitarget']['uctlauthmethod'];
$pconfig['uctlauthgroup'] = $config['iscsitarget']['uctlauthgroup'];
$pconfig['mediadirectory'] = $config['iscsitarget']['mediadirectory'];

if (!isset($pconfig['uctladdress']) || $pconfig['uctladdress'] == '') {
	$pconfig['uctladdress'] = "127.0.0.1";
	$pconfig['uctlport'] = "3261";
	$pconfig['uctlnetmask'] = "127.0.0.1/8";
	$pconfig['uctlauthmethod'] = "CHAP";
	$pconfig['uctlauthgroup'] = 0;
	$pconfig['mediadirectory'] = "/mnt";
}

if ($_POST) {
	unset($input_errors);
	unset($errormsg);

	$pconfig = $_POST;

	// Input validation.
	$reqdfields = explode(" ", "nodebase discoveryauthmethod discoveryauthgroup");
	$reqdfieldsn = array(gettext("Node Base"),
		gettext("Discovery Auth Method"),
		gettext("Discovery Auth Group"));
	$reqdfieldst = explode(" ", "string string numericint");

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	$reqdfields = explode(" ", "timeout nopininterval maxr2t maxsessions maxconnections firstburstlength maxburstlength maxrecvdatasegmentlength maxoutstandingr2t defaulttime2wait defaulttime2retain");
	$reqdfieldsn = array(gettext("I/O Timeout"),
		gettext("NOPIN Interval"),
		gettext("Max. sessions"),
		gettext("Max. connections"),
		gettext("Max. pre-send R2T"),
		gettext("FirstBurstLength"),
		gettext("MaxBurstLength"),
		gettext("MaxRecvDataSegmentLength"),
		gettext("MaxOutstandingR2T"),
		gettext("DefaultTime2Wait"),
		gettext("DefaultTime2Retain"));
	$reqdfieldst = explode(" ", "numericint numericint numericint numericint numericint numericint numericint numericint numericint numericint numericint");

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if ((strcasecmp("Auto", $pconfig['discoveryauthmethod']) != 0
	   && strcasecmp("None", $pconfig['discoveryauthmethod']) != 0)
		&& $pconfig['discoveryauthgroup'] == 0) {
		$input_errors[] = sprintf(gettext("The attribute '%s' is required."), gettext("Discovery Auth Group"));
	}

	$reqdfields = explode(" ", "uctladdress uctlport uctlnetmask uctlauthmethod uctlauthgroup mediadirectory");
	$reqdfieldsn = array(gettext("Controller IP address"),
		gettext("Controller TCP Port"),
		gettext("Controller Authorised network"),
		gettext("Controller Auth Method"),
		gettext("Controller Auth Group"),
		gettext("Media Directory"));
	$reqdfieldst = explode(" ", "string numericint string string numericint string");

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if (isset($_POST['uctlenable'])) {
		if ((strcasecmp("Auto", $pconfig['uctlauthmethod']) != 0
		   && strcasecmp("None", $pconfig['uctlauthmethod']) != 0)
			&& $pconfig['uctlauthgroup'] == 0) {
			if (count($config['iscsitarget']['authgroup']) == 0) {
				$errormsg .= sprintf(gettext("No configured Auth Group. Please add new <a href='%s'>Auth Group</a> first."), "services_iscsitarget_ag.php")."<br />\n";
			}
			$input_errors[] = sprintf(gettext("The attribute '%s' is required."), gettext("Controller Auth Group"));
		}
	}

	$nodebase = $_POST['nodebase'];
	$nodebase = preg_replace('/\s/', '', $nodebase);
	$pconfig['nodebase'] = $nodebase;

	if (!$input_errors) {
		$config['iscsitarget']['enable'] = $_POST['enable'] ? true : false;
		$config['iscsitarget']['nodebase'] = $nodebase;
		$config['iscsitarget']['discoveryauthmethod'] = $_POST['discoveryauthmethod'];
		$config['iscsitarget']['discoveryauthgroup'] = $_POST['discoveryauthgroup'];
		$config['iscsitarget']['timeout'] = $_POST['timeout'];
		$config['iscsitarget']['nopininterval'] = $_POST['nopininterval'];
		$config['iscsitarget']['maxr2t'] = $_POST['maxr2t'];
		$config['iscsitarget']['maxsessions'] = $_POST['maxsessions'];
		$config['iscsitarget']['maxconnections'] = $_POST['maxconnections'];
		$config['iscsitarget']['firstburstlength'] = $_POST['firstburstlength'];
		$config['iscsitarget']['maxburstlength'] = $_POST['maxburstlength'];
		$config['iscsitarget']['maxrecvdatasegmentlength'] = $_POST['maxrecvdatasegmentlength'];
		$config['iscsitarget']['maxoutstandingr2t'] = $_POST['maxoutstandingr2t'];
		$config['iscsitarget']['defaulttime2wait'] = $_POST['defaulttime2wait'];
		$config['iscsitarget']['defaulttime2retain'] = $_POST['defaulttime2retain'];

		$config['iscsitarget']['uctlenable'] = $_POST['uctlenable'] ? true : false;
		$config['iscsitarget']['uctladdress'] = $_POST['uctladdress'];
		$config['iscsitarget']['uctlport'] = $_POST['uctlport'];
		$config['iscsitarget']['uctlnetmask'] = $_POST['uctlnetmask'];
		$config['iscsitarget']['uctlauthmethod'] = $_POST['uctlauthmethod'];
		$config['iscsitarget']['uctlauthgroup'] = $_POST['uctlauthgroup'];
		$config['iscsitarget']['mediadirectory'] = $_POST['mediadirectory'];

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("iscsi_target");
			config_unlock();
		}

		$savemsg = get_std_save_message($retval);
	}
}

if (!is_array($config['iscsitarget']['portalgroup']))
	$config['iscsitarget']['portalgroup'] = array();

if (!is_array($config['iscsitarget']['initiatorgroup']))
	$config['iscsitarget']['initiatorgroup'] = array();

if (!is_array($config['iscsitarget']['authgroup']))
	$config['iscsitarget']['authgroup'] = array();
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	var endis = !(document.iform.enable.checked || enable_change);
	document.iform.nodebase.disabled = endis;
	document.iform.discoveryauthmethod.disabled = endis;
	document.iform.discoveryauthgroup.disabled = endis;
	document.iform.timeout.disabled = endis;
	document.iform.nopininterval.disabled = endis;
	document.iform.maxr2t.disabled = endis;
	document.iform.maxsessions.disabled = endis;
	document.iform.maxconnections.disabled = endis;
	document.iform.firstburstlength.disabled = endis;
	document.iform.maxburstlength.disabled = endis;
	document.iform.maxrecvdatasegmentlength.disabled = endis;
	document.iform.maxoutstandingr2t.disabled = endis;
	document.iform.defaulttime2wait.disabled = endis;
	document.iform.defaulttime2retain.disabled = endis;

	document.iform.uctladdress.disabled = endis;
	document.iform.uctlport.disabled = endis;
	document.iform.uctlnetmask.disabled = endis;
	document.iform.uctlauthmethod.disabled = endis;
	document.iform.uctlauthgroup.disabled = endis;
	document.iform.mediadirectory.disabled = endis;
}

function uctlenable_change(enable_change) {
	var endis = !(document.iform.enable.checked || enable_change);
	var endis2 = !(document.iform.uctlenable.checked || enable_change);

	if (!endis2) {
		showElementById("uctladdress_tr", 'show');
		showElementById("uctlport_tr", 'show');
		showElementById("uctlnetmask_tr", 'show');
		showElementById("uctlauthmethod_tr", 'show');
		showElementById("uctlauthgroup_tr", 'show');
		showElementById("mediadirectory_tr", 'show');
	} else {
		showElementById("uctladdress_tr", 'hide');
		showElementById("uctlport_tr", 'hide');
		showElementById("uctlnetmask_tr", 'hide');
		showElementById("uctlauthmethod_tr", 'hide');
		showElementById("uctlauthgroup_tr", 'hide');
		showElementById("mediadirectory_tr", 'hide');
	}
}
//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
	<li class="tabact"><a href="services_iscsitarget.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Settings");?></span></a></li>
	<li class="tabinact"><a href="services_iscsitarget_target.php"><span><?=gettext("Targets");?></span></a></li>
	<li class="tabinact"><a href="services_iscsitarget_pg.php"><span><?=gettext("Portals");?></span></a></li>
	<li class="tabinact"><a href="services_iscsitarget_ig.php"><span><?=gettext("Initiators");?></span></a></li>
	<li class="tabinact"><a href="services_iscsitarget_ag.php"><span><?=gettext("Auths");?></span></a></li>
	<li class="tabinact"><a href="services_iscsitarget_media.php"><span><?=gettext("Media");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
      <form action="services_iscsitarget.php" method="post" name="iform" id="iform">
	<?php if ($errormsg) print_error_box($errormsg);?>
	<?php if ($input_errors) print_input_errors($input_errors);?>
	<?php if ($savemsg) print_info_box($savemsg);?>
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
	<?php html_titleline_checkbox("enable", gettext("iSCSI Target"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
	<?php html_inputbox("nodebase", gettext("Base Name"), $pconfig['nodebase'], gettext("The base name (e.g. iqn.2007-09.jp.ne.peach.istgt) will append the target name that is not starting with 'iqn.'."), true, 60, false);?>
	<?php html_combobox("discoveryauthmethod", gettext("Discovery Auth Method"), $pconfig['discoveryauthmethod'], array("Auto" => gettext("Auto"), "CHAP" => gettext("CHAP"), "CHAP Mutual" => gettext("Mutual CHAP"), "None" => gettext("None")), gettext("The method can be accepted in discovery session. Auto means both none and authentication."), true);?>
	<?php
	    $ag_list = array();
	    $ag_list['0'] = gettext("None");
	    foreach($config['iscsitarget']['authgroup'] as $ag) {
		if ($ag['comment']) {
		    $l = sprintf(gettext("Tag%d (%s)"), $ag['tag'], $ag['comment']);
		} else {
		    $l = sprintf(gettext("Tag%d"), $ag['tag']);
		}
		$ag_list[$ag['tag']] = htmlspecialchars($l);
	    }
	?>
	<?php html_combobox("discoveryauthgroup", gettext("Discovery Auth Group"), $pconfig['discoveryauthgroup'], $ag_list, gettext("The initiator can discover the targets with correct user and secret in specific Auth Group."), true);?>
	<?php html_separator();?>
	<?php html_titleline(gettext("Advanced settings"));?>
	<?php html_inputbox("timeout", gettext("I/O Timeout"), $pconfig['timeout'], sprintf(gettext("I/O timeout in seconds (%d by default)."), 30), true, 30, false);?>
	<?php html_inputbox("nopininterval", gettext("NOPIN Interval"), $pconfig['nopininterval'], sprintf(gettext("NOPIN sending interval in seconds (%d by default)."), 20), true, 30, false);?>
	<?php html_inputbox("maxsessions", gettext("Max. sessions"), $pconfig['maxsessions'], sprintf(gettext("Maximum number of sessions holding at same time (%d by default)."), 16), true, 30, false);?>
	<?php html_inputbox("maxconnections", gettext("Max. connections"), $pconfig['maxconnections'], sprintf(gettext("Maximum number of connections in each session (%d by default)."), 4), true, 30, false);?>
	<?php html_inputbox("maxr2t", gettext("Max. pre-send R2T"), $pconfig['maxr2t'], sprintf(gettext("Maximum number of pre-send R2T in each connection (%d by default). The actual number is limited to QueueDepth of the target."), 32), true, 30, false);?>
	<?php html_inputbox("firstburstlength", gettext("FirstBurstLength"), $pconfig['firstburstlength'], sprintf(gettext("iSCSI initial parameter (%d by default)."), 262144), true, 30, false);?>
	<?php html_inputbox("maxburstlength", gettext("MaxBurstLength"), $pconfig['maxburstlength'], sprintf(gettext("iSCSI initial parameter (%d by default)."), 1048576), true, 30, false);?>
	<?php html_inputbox("maxrecvdatasegmentlength", gettext("MaxRecvDataSegmentLength"), $pconfig['maxrecvdatasegmentlength'], sprintf(gettext("iSCSI initial parameter (%d by default)."), 262144), true, 30, false);?>
	<?php html_inputbox("maxoutstandingr2t", gettext("MaxOutstandingR2T"), $pconfig['maxoutstandingr2t'], sprintf(gettext("iSCSI initial parameter (%d by default)."), 16), true, 30, false);?>
	<?php html_inputbox("defaulttime2wait", gettext("DefaultTime2Wait"), $pconfig['defaulttime2wait'], sprintf(gettext("iSCSI initial parameter (%d by default)."), 2), true, 30, false);?>
	<?php html_inputbox("defaulttime2retain", gettext("DefaultTime2Retain"), $pconfig['defaulttime2retain'], sprintf(gettext("iSCSI initial parameter (%d by default)."), 60), true, 30, false);?>
	<?php html_separator();?>
	<?php html_titleline_checkbox("uctlenable", gettext("iSCSI Target Logical Unit Controller"), $pconfig['uctlenable'] ? true : false, gettext("Enable"), "uctlenable_change(false)");?>
	<?php html_inputbox("uctladdress", gettext("Controller IP address"), $pconfig['uctladdress'], sprintf(gettext("Logical Unit Controller IP address (%s by default)"), "127.0.0.1(localhost)"), true, 30, false);?>
	<?php html_inputbox("uctlport", gettext("Controller TCP Port"), $pconfig['uctlport'], sprintf(gettext("Logical Unit Controller TCP port (%d by default)"), 3261), true, 15, false);?>
	<?php html_inputbox("uctlnetmask", gettext("Controller Authorised network"), $pconfig['uctlnetmask'], sprintf(gettext("Logical Unit Controller Authorised network (%s by default)"), "127.0.0.1/8"), true, 30, false);?>
	<?php html_combobox("uctlauthmethod", gettext("Controller Auth Method"), $pconfig['uctlauthmethod'], array("CHAP" => gettext("CHAP"), "CHAP mutual" => gettext("Mutual CHAP"), "None" => gettext("None")), gettext("The method can be accepted in the controller."), true);?>
	<?php
	    $ag_list = array();
	    $ag_list['0'] = gettext("Must choose one");
	    foreach($config['iscsitarget']['authgroup'] as $ag) {
		if ($ag['comment']) {
		    $l = sprintf(gettext("Tag%d (%s)"), $ag['tag'], $ag['comment']);
		} else {
		    $l = sprintf(gettext("Tag%d"), $ag['tag']);
		}
		$ag_list[$ag['tag']] = htmlspecialchars($l);
	    }
	?>
	<?php html_combobox("uctlauthgroup", gettext("Controller Auth Group"), $pconfig['uctlauthgroup'], $ag_list, gettext("The istgtcontrol can access the targets with correct user and secret in specific Auth Group."), true);?>
	<?php html_filechooser("mediadirectory", gettext("Media Directory"), $pconfig['mediadirectory'], gettext("Directory that contains removable media. (e.g /mnt/iscsi/)"), $g['media_path'], true);?>
	</table>
	<div id="submit">
	  <input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save and Restart");?>" onclick="enable_change(true)" />
	</div>
	<div id="remarks">
	  <?php html_remark("note", gettext("Note"), sprintf(gettext("You must have a minimum of %dMB of RAM for using iSCSI target."), 512));?>
	</div>
	<?php include("formend.inc");?>
      </form>
    </td>
  </tr>
</table>
<script type="text/javascript">
<!--
enable_change();
uctlenable_change();
//-->
</script>
<?php include("fend.inc");?>
