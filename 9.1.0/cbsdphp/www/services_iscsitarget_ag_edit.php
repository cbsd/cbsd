#!/usr/local/bin/php
<?php
/*
	services_iscsitarget_ag_edit.php

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

$uuid = $_GET['uuid'];
if (isset($_POST['uuid']))
	$uuid = $_POST['uuid'];

$pgtitle = array(gettext("Services"), gettext("iSCSI Target"), gettext("Auth Group"), isset($uuid) ? gettext("Edit") : gettext("Add"));

$MAX_AUTHUSERS = 4;
$GROW_AUTHUSERS = 4;

if (!isset($config['iscsitarget']['authgroup']) || !is_array($config['iscsitarget']['authgroup']))
	$config['iscsitarget']['authgroup'] = array();

array_sort_key($config['iscsitarget']['authgroup'], "tag");
$a_iscsitarget_ag = &$config['iscsitarget']['authgroup'];

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_iscsitarget_ag, "uuid")))) {
	$pconfig['uuid'] = $a_iscsitarget_ag[$cnid]['uuid'];
	$pconfig['tag'] = $a_iscsitarget_ag[$cnid]['tag'];
	$pconfig['comment'] = $a_iscsitarget_ag[$cnid]['comment'];
	$i = 1;
	if (!is_array($a_iscsitarget_ag[$cnid]['agauth']))
		$a_iscsitarget_ag[$cnid]['agauth'] = array();
	array_sort_key($a_iscsitarget_ag[$cnid]['agauth'], "authuser");
	foreach ($a_iscsitarget_ag[$cnid]['agauth'] as $agauth) {
		$pconfig["user$i"] = $agauth['authuser'];
		$pconfig["secret$i"] = $agauth['authsecret'];
		$pconfig["secret2$i"] = $pconfig["secret$i"];
		$pconfig["muser$i"] = $agauth['authmuser'];
		$pconfig["msecret$i"] = $agauth['authmsecret'];
		$pconfig["msecret2$i"] = $pconfig["msecret$i"];
		$i++;
	}
	while ($i > $MAX_AUTHUSERS) {
		$MAX_AUTHUSERS += $GROW_AUTHUSERS;
	}
} else {
	// Find next unused tag.
	$tag = 1;
	$a_tags = array();
	foreach($a_iscsitarget_ag as $ag)
		$a_tags[] = $ag['tag'];

	while (true === in_array($tag, $a_tags))
		$tag += 1;

	$pconfig['uuid'] = uuid();
	$pconfig['tag'] = $tag;
	$pconfig['comment'] = "";
}

if ($_POST) {
	unset($input_errors);
	unset($errormsg);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: services_iscsitarget_ag.php");
		exit;
	}

	// Input validation.
	$reqdfields = explode(" ", "tag");
	$reqdfieldsn = array(gettext("Tag number"));
	$reqdfieldst = explode(" ", "numericint");

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if ($pconfig['tag'] < 1 || $pconfig['tag'] > 65535) {
		$input_errors[] = gettext("The tag range is invalid.");
	}
	if (!(isset($uuid) && (FALSE !== $cnid))) {
		$index = array_search_ex($pconfig['tag'], $config['iscsitarget']['authgroup'], "tag");
		if ($index !== FALSE) {
			$input_errors[] = gettext("This tag already exists.");
		}
	}

	$auths = array();
    for ($i = 1; $i <= $MAX_AUTHUSERS; $i++) {
		$delete = $_POST["delete$i"] ? true : false;
		$user = $_POST["user$i"];
		$secret = $_POST["secret$i"];
		$secret2 = $_POST["secret2$i"];
		$muser = $_POST["muser$i"];
		$msecret = $_POST["msecret$i"];
		$msecret2 = $_POST["msecret2$i"];
		if (strlen($user) != 0
			|| strlen($secret) != 0 || strlen($secret2) != 0) {
			if (strlen($user) == 0) {
				$input_errors[] = sprintf("%s%d: %s", gettext("User"), $i, sprintf(gettext("The attribute '%s' is required."), gettext("User")));
			}
			if (strcmp($secret, $secret2) != 0) {
				$input_errors[] = sprintf("%s%d: %s", gettext("User"), $i, gettext("Password don't match."));
			}
		}
		if (strlen($muser) != 0
			|| strlen($msecret) != 0 || strlen($msecret2) != 0) {
			if (strlen($user) == 0) {
				$input_errors[] = sprintf("%s%d: %s", gettext("User"), $i, sprintf(gettext("The attribute '%s' is required."), gettext("User")));
			}
			if (strlen($muser) == 0) {
				$input_errors[] = sprintf("%s%d: %s", gettext("User"), $i, sprintf(gettext("The attribute '%s' is required."), gettext("Peer User")));
			}
			if (strcmp($msecret, $msecret2) != 0) {
				$input_errors[] = sprintf("%s%d: %s", gettext("User"), $i, gettext("Password don't match."));
			}
		}
		if (strlen($user) != 0
			&& $delete === false) {
			$index = array_search_ex($user, $auths, "authuser");
			if ($index !== false) {
				$input_errors[] = sprintf("%s%d: %s", gettext("User"), $i, gettext("This user already exists."));
			} else {
				$tmp = array();
				$tmp['authuser'] = $user;
				$tmp['authsecret'] = $secret;
				$tmp['authmuser'] = $muser;
				$tmp['authmsecret'] = $msecret;
				$auths[] = $tmp;
			}
		}
	}

	if (!$input_errors) {
		$iscsitarget_ag = array();
		$iscsitarget_ag['uuid'] = $_POST['uuid'];
		$iscsitarget_ag['tag'] = $_POST['tag'];
		$iscsitarget_ag['comment'] = $_POST['comment'];
		$iscsitarget_ag['agauth'] = $auths;

		if (isset($uuid) && (FALSE !== $cnid)) {
			$a_iscsitarget_ag[$cnid] = $iscsitarget_ag;
			$mode = UPDATENOTIFY_MODE_MODIFIED;
		} else {
			$a_iscsitarget_ag[] = $iscsitarget_ag;
			$mode = UPDATENOTIFY_MODE_NEW;
		}

		updatenotify_set("iscsitarget_ag", $mode, $iscsitarget_ag['uuid']);
		write_config();

		header("Location: services_iscsitarget_ag.php");
		exit;
	}
}

function expand_ipv6addr($v6addr) {
	if (strlen($v6addr) == 0)
		return null;
	$v6str = $v6addr;

	// IPv4 mapped address
	$pos = strpos($v6str, ".");
	if ($pos !== false) {
		$pos = strrpos($v6str, ":");
		if ($pos === false) {
			return null;
		}
		$v6lstr = substr($v6str, 0, $pos);
		$v6rstr = substr($v6str, $pos + 1);
		$v4a = sscanf($v6rstr, "%d.%d.%d.%d");
		$v6rstr = sprintf("%02x%02x:%02x%02x",
						  $v4a[0], $v4a[1], $v4a[2], $v4a[3]);
		$v6str = $v6lstr.":".$v6rstr;
	}

	// replace zero for "::"
	$pos = strpos($v6str, "::");
	if ($pos !== false) {
		$v6lstr = substr($v6str, 0, $pos);
		$v6rstr = substr($v6str, $pos + 2);
		if (strlen($v6lstr) == 0) {
			$v6lstr = "0";
		}
		if (strlen($v6rstr) == 0) {
			$v6rstr = "0";
		}
		$v6lcnt = strlen(ereg_replace("[^:]", "", $v6lstr));
		$v6rcnt = strlen(ereg_replace("[^:]", "", $v6rstr));
		$v6str = $v6lstr;
		$v6ncnt = 8 - ($v6lcnt + 1 + $v6rcnt + 1);
		while ($v6ncnt > 0) {
			$v6str .= ":0";
			$v6ncnt--;
		}
		$v6str .= ":".$v6rstr;
	}

	// zero padding
	$v6a = explode(":", $v6str);
	foreach ($v6a as &$tmp) {
		$tmp = str_pad($tmp, 4, "0", STR_PAD_LEFT);
	}
	unset($tmp);
	$v6str = implode(":", $v6a);
	return $v6str;
}

function normalize_ipv6addr($v6addr) {
	if (strlen($v6addr) == 0)
		return null;
	$v6str = expand_ipv6addr($v6addr);

	// suppress prefix zero
	$v6a = explode(":", $v6str);
	foreach ($v6a as &$tmp) {
		$tmp = ereg_replace("^[0]+", "", $tmp);
		if (strlen($tmp) == 0) {
			$tmp = "0";
		}
	}
	$v6str = implode(":", $v6a);

	// replace first zero as "::"
	$replace_flag = 1;
	$found_zero = 0;
	$v6a = explode(":", $v6str);
	foreach ($v6a as &$tmp) {
		if (strcmp($tmp, "0") == 0) {
			if ($replace_flag) {
				$tmp = "z";
				$found_zero++;
			}
		} else {
			if ($found_zero) {
				$replace_flag = 0;
			}
		}
	}
	unset($tmp);
	$v6str = implode(":", $v6a);
	if ($found_zero > 1) {
		$v6str = ereg_replace("(:?z:?)+", "::", $v6str);
	} else {
		$v6str = ereg_replace("(z)+", "0", $v6str);
	}
	return $v6str;
}
?>
<?php include("fbegin.inc");?>
<form action="services_iscsitarget_ag_edit.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
	  <tr>
	    <td class="tabnavtbl">
	      <ul id="tabnav">
					<li class="tabinact"><a href="services_iscsitarget.php"><span><?=gettext("Settings");?></span></a></li>
					<li class="tabinact"><a href="services_iscsitarget_target.php"><span><?=gettext("Targets");?></span></a></li>
					<li class="tabinact"><a href="services_iscsitarget_pg.php"><span><?=gettext("Portals");?></span></a></li>
					<li class="tabinact"><a href="services_iscsitarget_ig.php"><span><?=gettext("Initiators");?></span></a></li>
					<li class="tabact"><a href="services_iscsitarget_ag.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Auths");?></span></a></li>
					<li class="tabinact"><a href="services_iscsitarget_media.php"><span><?=gettext("Media");?></span></a></li>
	      </ul>
	    </td>
	  </tr>
	  <tr>
	    <td class="tabcont">
	      <?php if ($input_errors) print_input_errors($input_errors);?>
	      <table width="100%" border="0" cellpadding="6" cellspacing="0">
	      <?php html_inputbox("tag", gettext("Tag number"), $pconfig['tag'], gettext("Numeric identifier of the group."), true, 10, (isset($uuid) && (FALSE !== $cnid)));?>
	      <?php html_inputbox("comment", gettext("Comment"), $pconfig['comment'], gettext("You may enter a description here for your reference."), false, 40);?>
	      <?php for ($i = 1; $i <= $MAX_AUTHUSERS; $i++): ?>
	      <?php $ldelete=sprintf("delete%d", $i); ?>
	      <?php $luser=sprintf("user%d", $i); ?>
	      <?php $lsecret=sprintf("secret%d", $i); ?>
	      <?php $lsecret2=sprintf("secret2%d", $i); ?>
	      <?php $lmuser=sprintf("muser%d", $i); ?>
	      <?php $lmsecret=sprintf("msecret%d", $i); ?>
	      <?php $lmsecret2=sprintf("msecret2%d", $i); ?>
	      <?php html_separator();?>
	      <?php html_titleline_checkbox("$ldelete", sprintf("%s%d", gettext("User"), $i), false, gettext("Delete"), false);?>
	      <?php html_inputbox("$luser", gettext("User"), $pconfig["$luser"], gettext("Target side user name. It is usually the initiator name by default."), false, 60);?>
	      <tr>
	        <td width="22%" valign="top" class="vncell"><?=gettext("Secret");?></td>
	        <td width="78%" class="vtable">
	          <input name="<?=$lsecret;?>" type="password" class="formfld" id="<?=$lsecret;?>" size="30" value="<?=htmlspecialchars($pconfig[$lsecret]);?>" /><br />
	          <input name="<?=$lsecret2;?>" type="password" class="formfld" id="<?=$lsecret2;?>" size="30" value="<?=htmlspecialchars($pconfig[$lsecret2]);?>" />&nbsp;(<?=gettext("Confirmation");?>)<br />
	          <span class="vexpl"><?=gettext("Target side secret.");?></span>
	        </td>
	      </tr>
	      <?php html_inputbox("$lmuser", gettext("Peer User"), $pconfig["$lmuser"], gettext("Initiator side user name. (for mutual CHAP authentication)"), false, 60);?>
	      <tr>
	        <td width="22%" valign="top" class="vncell"><?=gettext("Peer Secret");?></td>
	        <td width="78%" class="vtable">
	          <input name="<?=$lmsecret;?>" type="password" class="formfld" id="<?=$lmsecret;?>" size="30" value="<?=htmlspecialchars($pconfig[$lmsecret]);?>" /><br />
	          <input name="<?=$lmsecret2;?>" type="password" class="formfld" id="<?=$lmsecret2;?>" size="30" value="<?=htmlspecialchars($pconfig[$lmsecret2]);?>" />&nbsp;(<?=gettext("Confirmation");?>)<br />
	          <span class="vexpl"><?=gettext("Initiator side secret. (for mutual CHAP autentication)");?></span>
	        </td>
	      </tr>
	      <?php endfor;?>
	      </table>
	      <div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=(isset($uuid) && (FALSE !== $cnid)) ? gettext("Save") : gettext("Add")?>" />
					<input name="Cancel" type="submit" class="formbtn" value="<?=gettext("Cancel");?>" />
		      <input name="uuid" type="hidden" value="<?=$pconfig['uuid'];?>" />
	      </div>
	    </td>
	  </tr>
	</table>
	<?php include("formend.inc");?>
</form>
<?php include("fend.inc");?>
