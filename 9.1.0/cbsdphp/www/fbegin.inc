<?php
/*
	fbegin.inc
	
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

function gentitle($title) {
	$navlevelsep = "|"; // Navigation level separator string.
	return join($navlevelsep, $title);
}

function genhtmltitle($title) {
	return system_get_hostname() . " - " . gentitle($title);
}

// Menu items.
// System
$menu['system']['desc'] = gettext("System");
$menu['system']['visible'] = TRUE;
$menu['system']['link'] = "";
$menu['system']['menuitem'] = array();
$menu['system']['menuitem'][] = array("desc" => gettext("General"), "link" => "system.php", "visible" => Session::isAdmin());
$menu['system']['menuitem'][] = array("desc" => gettext("Advanced"), "link" => "system_advanced.php", "visible" => Session::isAdmin());
$menu['system']['menuitem'][] = array("desc" => gettext("Password"), "link" => "userportal_system_password.php", "visible" => !Session::isAdmin());
$menu['system']['menuitem'][] = array("type" => "separator", "visible" => Session::isAdmin());
if ("full" === $g['platform']) {
	$menu['system']['menuitem'][] = array("desc" => gettext("Packages"), "link" => "system_packages.php", "visible" => Session::isAdmin());
} else {
	$menu['system']['menuitem'][] = array("desc" => gettext("Firmware"), "link" => "system_firmware.php", "visible" => Session::isAdmin());
}
$menu['system']['menuitem'][] = array("desc" => gettext("Backup/Restore"), "link" => "system_backup.php", "visible" => Session::isAdmin());
$menu['system']['menuitem'][] = array("desc" => gettext("Factory defaults"), "link" => "system_defaults.php", "visible" => Session::isAdmin());
$menu['system']['menuitem'][] = array("type" => "separator", "visible" => Session::isAdmin());
$menu['system']['menuitem'][] = array("desc" => gettext("Reboot"), "link" => "reboot.php", "visible" => Session::isAdmin());
$menu['system']['menuitem'][] = array("desc" => gettext("Shutdown"), "link" => "shutdown.php", "visible" => Session::isAdmin());
$menu['system']['menuitem'][] = array("type" => "separator", "visible" => TRUE);
$menu['system']['menuitem'][] = array("desc" => gettext("Logout"), "link" => "logout.php", "visible" => TRUE);

// Network
$menu['network']['desc'] = gettext("Network");
$menu['network']['visible'] = Session::isAdmin();
$menu['network']['link'] = "";
$menu['network']['menuitem'] = array();
$menu['network']['menuitem'][] = array("desc" => gettext("Interface Management"), "link" => "interfaces_assign.php", "visible" => TRUE);
$menu['network']['menuitem'][] = array("desc" => gettext("LAN Management"), "link" => "interfaces_lan.php", "visible" => TRUE);
for ($i = 1; isset($config['interfaces']['opt' . $i]); $i++) {
	$desc = $config['interfaces']['opt'.$i]['descr'];
	$menu['network']['menuitem'][] = array("desc" => "{$desc}", "link" => "interfaces_opt.php?index={$i}", "visible" => TRUE);
}
$menu['network']['menuitem'][] = array("type" => "separator", "visible" => TRUE);
$menu['network']['menuitem'][] = array("desc" => gettext("Hosts"), "link" => "system_hosts.php", "visible" => TRUE);
$menu['network']['menuitem'][] = array("desc" => gettext("Static Routes"), "link" => "system_routes.php", "visible" => TRUE);
$menu['network']['menuitem'][] = array("desc" => gettext("Firewall"), "link" => "system_firewall.php", "visible" => TRUE);

// Disks
$menu['disks']['desc'] = gettext("Disks");
$menu['disks']['visible'] = Session::isAdmin();
$menu['disks']['link'] = "";
$menu['disks']['menuitem'] = array();
$menu['disks']['menuitem'][] = array("desc" => gettext("Management"), "link" => "disks_manage.php", "visible" => TRUE);
$menu['disks']['menuitem'][] = array("desc" => gettext("Software RAID"), "link" => "disks_raid_gmirror.php", "visible" => TRUE);
$menu['disks']['menuitem'][] = array("desc" => gettext("Encryption"), "link" => "disks_crypt.php", "visible" => TRUE);
$menu['disks']['menuitem'][] = array("desc" => gettext("ZFS"), "link" => "disks_zfs_zpool.php", "visible" => TRUE);
$menu['disks']['menuitem'][] = array("desc" => gettext("Format"), "link" => "disks_init.php", "visible" => TRUE);
$menu['disks']['menuitem'][] = array("desc" => gettext("Mount Point"), "link" => "disks_mount.php", "visible" => TRUE);

// Services
$menu['services']['desc'] = gettext("Services");
$menu['services']['visible'] = Session::isAdmin();
$menu['services']['link'] = "status_services.php";
$menu['services']['menuitem'] = array();
$menu['services']['menuitem'][] = array("desc" => gettext("CIFS/SMB"), "link" => "services_samba.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("FTP"), "link" => "services_ftp.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("TFTP"), "link" => "services_tftp.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("SSH"), "link" => "services_sshd.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("NFS"), "link" => "services_nfs.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("AFP"), "link" => "services_afp.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("Rsync"), "link" => "services_rsyncd.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("Unison"), "link" => "services_unison.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("iSCSI Target"), "link" => "services_iscsitarget.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("UPnP"), "link" => "services_upnp.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("iTunes/DAAP"), "link" => "services_daap.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("Dynamic DNS"), "link" => "services_dynamicdns.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("SNMP"), "link" => "services_snmp.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("UPS"), "link" => "services_ups.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("Webserver"), "link" => "services_websrv.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("BitTorrent"), "link" => "services_bittorrent.php", "visible" => TRUE);
$menu['services']['menuitem'][] = array("desc" => gettext("LCDproc"), "link" => "services_lcdproc.php", "visible" => TRUE);

// Access
$menu['access']['desc'] = gettext("Access");
$menu['access']['visible'] = Session::isAdmin();
$menu['access']['link'] = "";
$menu['access']['menuitem'] = array();
$menu['access']['menuitem'][] = array("desc" => gettext("Users and Groups"), "link" => "access_users.php", "visible" => TRUE);
$menu['access']['menuitem'][] = array("desc" => gettext("Active Directory"), "link" => "access_ad.php", "visible" => TRUE);
$menu['access']['menuitem'][] = array("desc" => gettext("LDAP"), "link" => "access_ldap.php", "visible" => TRUE);
$menu['access']['menuitem'][] = array("desc" => gettext("NIS"), "link" => "notavailable.php", "visible" => false);

// Status
$menu['status']['desc'] = gettext("Status");
$menu['status']['visible'] = Session::isAdmin();
$menu['status']['link'] = "index.php";
$menu['status']['menuitem'] = array();
$menu['status']['menuitem'][] = array("desc" => gettext("System"), "link" => "index.php", "visible" => TRUE);
$menu['status']['menuitem'][] = array("desc" => gettext("Process"), "link" => "status_process.php", "visible" => TRUE);
$menu['status']['menuitem'][] = array("desc" => gettext("Services"), "link" => "status_services.php", "visible" => TRUE);
$menu['status']['menuitem'][] = array("desc" => gettext("Interfaces"), "link" => "status_interfaces.php", "visible" => TRUE);
$menu['status']['menuitem'][] = array("desc" => gettext("Disks"), "link" => "status_disks.php", "visible" => TRUE);
$menu['status']['menuitem'][] = array("desc" => gettext("Graph"), "link" => "status_graph.php", "visible" => TRUE);
$menu['status']['menuitem'][] = array("desc" => gettext("Email Report"), "link" => "status_report.php", "visible" => TRUE);

// Advanced
$menu['advanced']['desc'] = gettext("Advanced");
$menu['advanced']['visible'] = TRUE;
$menu['advanced']['link'] = "";
$menu['advanced']['menuitem'] = array();
$menu['advanced']['menuitem'][] = array("desc" => gettext("File Editor"), "link" => "system_edit.php", "visible" => Session::isAdmin());
if (!isset($config['system']['disablefm'])) {
	$menu['advanced']['menuitem'][] = array("desc" => gettext("File Manager"), "link" => "/quixplorer/system_filemanager.php", "visible" => TRUE);
}
$menu['advanced']['menuitem'][] = array("type" => "separator", "visible" => Session::isAdmin());
$menu['advanced']['menuitem'][] = array("desc" => gettext("Command"), "link" => "exec.php", "visible" => Session::isAdmin());

// Diagnostics
$menu['diagnostics']['desc'] = gettext("Diagnostics");
$menu['diagnostics']['visible'] = Session::isAdmin();
$menu['diagnostics']['link'] = "";
$menu['diagnostics']['menuitem'] = array();
$menu['diagnostics']['menuitem'][] = array("desc" => gettext("Log"), "link" => "diag_log.php", "visible" => TRUE);
$menu['diagnostics']['menuitem'][] = array("desc" => gettext("Information"), "link" => "diag_infos.php", "visible" => TRUE);
$menu['diagnostics']['menuitem'][] = array("type" => "separator", "visible" => TRUE);
$menu['diagnostics']['menuitem'][] = array("desc" => gettext("Ping/Traceroute"), "link" => "diag_ping.php", "visible" => TRUE);
$menu['diagnostics']['menuitem'][] = array("desc" => gettext("ARP Tables"), "link" => "diag_arp.php", "visible" => TRUE);
$menu['diagnostics']['menuitem'][] = array("desc" => gettext("Routes"), "link" => "diag_routes.php", "visible" => TRUE);

// Help
$menu['help']['desc'] = gettext("Help");
$menu['help']['visible'] = TRUE;
$menu['help']['link'] = "";
$menu['help']['menuitem'] = array();
$menu['help']['menuitem'][] = array("desc" => gettext("Report Generator"), "link" => "report_generator.php", "visible" => TRUE);
$menu['help']['menuitem'][] = array("type" => "separator", "visible" => TRUE);
$menu['help']['menuitem'][] = array("desc" => gettext("Forum"), "link" => "http://forums.nas4free.org", "visible" => TRUE, "target" => "_blank");
$menu['help']['menuitem'][] = array("desc" => gettext("Information & Manual"), "link" => "http://wiki.nas4free.org/", "visible" => TRUE, "target" => "_blank");
$menu['help']['menuitem'][] = array("desc" => gettext("IRC Live Support"), "link" => "http://webchat.freenode.net/?channels=#nas4free", "visible" => TRUE, "target" => "_blank");
$menu['help']['menuitem'][] = array("type" => "separator", "visible" => TRUE);
$menu['help']['menuitem'][] = array("desc" => gettext("Release Notes"), "link" => "changes.php", "visible" => TRUE);
$menu['help']['menuitem'][] = array("desc" => gettext("License & Credits"), "link" => "license.php", "visible" => TRUE);
$menu['help']['menuitem'][] = array("desc" => gettext("Donate"), "link" => "https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=info%40nas4free%2eorg&lc=US&item_name=NAS4Free%20Project&no_note=0&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHostedGuest", "visible" => TRUE, "target" => "_blank");
function display_menu($menuid) {
	global $menu;

	// Is menu visible?
	if (!$menu[$menuid]['visible'])
		return;

	$link = $menu[$menuid]['link'];
	if ($link == '') $link = 'index.php';
	echo "<li>\n";
	echo "	<a href=\"{$link}\" onmouseover=\"mopen('{$menuid}')\" onmouseout=\"mclosetime()\">".htmlspecialchars($menu[$menuid]['desc'])."</a>\n";
	echo "	<div id=\"{$menuid}\" onmouseover=\"mcancelclosetime()\" onmouseout=\"mclosetime()\">\n";

	# Display menu items.
	foreach ($menu[$menuid]['menuitem'] as $menuk => $menuv) {
		# Is menu item visible?
		if (!$menuv['visible']) {
			continue;
		}
		if ("separator" !== $menuv['type']) {
			# Display menuitem.
			$link = $menuv['link'];
			if ($link == '') $link = 'index.php';
			echo "<a href=\"{$link}\" target=\"" . (empty($menuv['target']) ? "_self" : $menuv['target']) . "\" title=\"".htmlspecialchars($menuv['desc'])."\">".htmlspecialchars($menuv['desc'])."</a>\n";
		} else {
			# Display separator.
			echo "<span class=\"tabseparator\">&nbsp;</span>";
		}
	}

	echo "	</div>\n";
	echo "</li>\n";
}
?>
<?php header("Content-Type: text/html; charset=" . system_get_language_codeset());?>
<?php
  // XML declarations
/*
  some browser might be broken.
  echo '<?xml version="1.0" encoding="'.system_get_language_codeset().'"?>';
  echo "\n";
*/
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="<?=system_get_language_code();?>" lang="<?=system_get_language_code();?>">
<head>
	<title><?=htmlspecialchars(genhtmltitle($pgtitle));?></title>
	<meta http-equiv="Content-Type" content="text/html; charset=<?=system_get_language_codeset();?>" />
	<meta http-equiv="Content-Script-Type" content="text/javascript" />
	<meta http-equiv="Content-Style-Type" content="text/css" />
	<?php if ($pgrefresh):?>
	<meta http-equiv="refresh" content="<?=$pgrefresh;?>" />
	<?php endif;?>
    <link href="gui.css" rel="stylesheet" type="text/css" />
	<link href="navbar.css" rel="stylesheet" type="text/css" />
	<link href="tabs.css" rel="stylesheet" type="text/css" />	
	<script type="text/javascript" src="javascript/gui.js"></script>
	<script type="text/javascript" src="javascript/navbar.js"></script>
<?php
	if (isset($pglocalheader) && !empty($pglocalheader)) {
		if (is_array($pglocalheader)) {
			foreach ($pglocalheader as $pglocalheaderv) {
		 		echo $pglocalheaderv;
				echo "\n";
			}
		} else {
			echo $pglocalheader;
			echo "\n";
		}
	}
?>
</head>
<body>
<div id="header">
	<div id="headerlogo">
		<a title="www.<?=get_product_url();?>" href="http://<?=get_product_url();?>" target="_blank"><img src="header_logo.png" alt="logo" /></a>
	</div>
	<div id="headerrlogo">
		<div class="hostname">
			<span><?=system_get_hostname();?>&nbsp;</span>
		</div>
	</div>
</div>
<div id="headernavbar">
	<ul id="navbarmenu">
		<?=display_menu("system");?>
		<?=display_menu("network");?>
		<?=display_menu("disks");?>
		<?=display_menu("services");?>
		<!-- Begin extension section -->
		<?php if (Session::isAdmin() && is_dir("{$g['www_path']}/ext")):?>
		<li>
			<a href="index.php" onmouseover="mopen('extensions')" onmouseout="mclosetime()"><?=gettext("Extensions");?></a>
			<div id="extensions" onmouseover="mcancelclosetime()" onmouseout="mclosetime()">
				<?php
				$dh = @opendir("{$g['www_path']}/ext");
				if ($dh) {
					while (($extd = readdir($dh)) !== false) {
						if (($extd === ".") || ($extd === ".."))
							continue;
						@include("{$g['www_path']}/ext/" . $extd . "/menu.inc");
					}
					closedir($dh);
				}?>
			</div>
		</li>
		<?php endif;?>
		<!-- End extension section -->
		<?=display_menu("access");?>
		<?=display_menu("status");?>
		<?=display_menu("diagnostics");?>
		<?=display_menu("advanced");?>
		<?=display_menu("help");?>
	</ul>
	<div style="clear:both"></div>
</div>
<div id="pagecontent">
	<table width="100%" border="0" cellspacing="0" cellpadding="2">
	  <tr valign="top">
	    <td>
				<table width="100%" border="0" cellpadding="10" cellspacing="0">
	        <tr>
						<td>
							<?php if (!$pgtitle_omit): ?>
	      			<p class="pgtitle"><?=htmlspecialchars(gentitle($pgtitle));?></p>
							<?php endif;?>
