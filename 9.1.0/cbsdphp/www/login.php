<?php
/*
	login.php
	
	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of FreeNAS (http://www.freenas.org)
	Copyright (C) 2005-2011 Olivier Cochard <olivier@freenas.org>.
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
require("guiconfig.inc");

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
	Session::start();

	if ($_POST['username'] === $config['system']['username'] &&
		$_POST['password'] === $config['system']['password']) {
		Session::initAdmin();
		header('Location: index.php');
		exit;
	} else {
		$users = system_get_user_list();
		foreach ($users as $userk => $userv) {
			$password = crypt($_POST['password'], $userv['password']);
			if (($_POST['username'] === $userv['name']) && ($password === $userv['password'])) {
				// Check if it is a local user
				if (FALSE === ($cnid = array_search_ex($userv['uid'], $config['access']['user'], "id")))
					break;
				// Is user allowed to access the user portal?
				if (!isset($config['access']['user'][$cnid]['userportal']))
					break;
				Session::initUser($userv['uid'], $userv['name']);
				header('Location: index.php');
				exit;
			}
		}
	}

	write_log(gettext("Authentication error for illegal user {$_POST['username']} from {$_SERVER['REMOTE_ADDR']}"));
}
?>
<?php header("Content-Type: text/html; charset=" . system_get_language_codeset());?>
<?php
function gentitle($title) {
	$navlevelsep = "|"; // Navigation level separator string.
	//return join($navlevelsep, $title);
}

function genhtmltitle($title) {
	return system_get_hostname() . " - " . gentitle($title);
}

// Menu items.
// Info and Manual
$menu['info']['desc'] = gettext("Information & Manuals");
$menu['info']['visible'] = TRUE;
$menu['info']['link'] = "http://wiki.nas4free.org/";
$menu['info']['menuitem']['visible'] = FALSE;
// Forum
$menu['forum']['desc'] = gettext("Forum");
$menu['forum']['link'] = "http://forums.nas4free.org";
$menu['forum']['visible'] = TRUE;
$menu['forum']['menuitem']['visible'] = FALSE;
// IRC
$menu['irc']['desc'] = gettext("IRC Live Support");
$menu['irc']['visible'] = TRUE;
$menu['irc']['link'] = "http://webchat.freenode.net/?channels=#nas4free";
$menu['irc']['menuitem']['visible'] = FALSE;
// Donate
$menu['donate']['desc'] = gettext("Donate");
$menu['donate']['visible'] = TRUE;
$menu['donate']['link'] = "https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=info%40nas4free%2eorg&lc=US&item_name=NAS4Free%20Project&no_note=0&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHostedGuest";
$menu['donate']['menuitem']['visible'] = FALSE;


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
<body onload='document.iform.username.focus();'>
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
		<?=display_menu("forum");?>
		<?=display_menu("info");?>
		<?=display_menu("irc");?>
		<?=display_menu("donate");?>
	</ul>
	<div style="clear:both"></div>
</div>
        <br /><br /><br /><br /><br /><br /><br /><br />
        <br /><br /><br /><br /><br /><br /><br /><br /><br />
        <div id="loginpage">
            <table height="100%" width="100%" cellspacing="0" cellpadding="0" border="0">
				<tbody>
					<tr>
						<td align="center">
							<form name="iform" id="iform" action="login.php" method="post">
								<table>
									<tbody>
										<tr>
											<td align="center">
												<div class="shadow">
													<div id="loginboxheader"><b><?=gettext("NAS4Free WebGUI Login");?></b></div>
													<div id="loginbox">
														<table background="vncell_bg.png">
															<tbody>
																<tr>
																	<td><b><?=gettext("Username");?>:</b></td>
																	<td><input class="formfld" type="text" name="username" value="" /></td>
																</tr>
																<tr>
																	<td><b><?=gettext("Password");?>:</b></td>
																	<td><input class="formfld" type="password" name="password" value="" /></td>
																</tr>
																<tr>
																	<td align="right" colspan="2"><input class="formbtn" type="submit" value="<?=gettext("Login");?>" /></td>
																</tr>
															</tbody>
														</table>
													</div>
												</duv>
											</td>
										</tr>
									</tbody>
								</table>
							</form>
						</td>
					</tr>
				</tbody>
			</table>
        </div>
        <div id="pagefooter">
			<span><p><a title="www.<?=get_product_url();?>" href="http://<?=get_product_url();?>" target="_blank"></a> <?=str_replace("Copyright (C)","&copy;",get_product_copyright());?></a></p></span>
		</div>
    </body>
</html>
