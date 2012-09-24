#!/usr/local/bin/php
<?php
/*
	system_firmware.php

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
$d_isfwfile = 1;

require("auth.inc");
require("guiconfig.inc");

$pgtitle = array(gettext("System"), gettext("Firmware"));

/* checks with /etc/firm.url to see if a newer firmware version online is available;
   returns any HTML message it gets from the server */
$locale = $config['system']['language'];

function check_firmware_version($locale) {
	global $g;

	$post = "product=".rawurlencode(get_product_name())
	      . "&platform=".rawurlencode($g['fullplatform'])
	      . "&version=".rawurlencode(get_product_version())
	      . "&revision=".rawurlencode(get_product_revision());
	$url = trim(get_firm_url());
	if (preg_match('/^([^\/]+)(\/.*)/', $url, $m)) {
		$host = $m[1];
		$path = $m[2];
	} else {
		$host = $url;
		$path = "";
	}

	$rfd = @fsockopen($host, 80, $errno, $errstr, 3);
	if ($rfd) {
		$hdr = "POST $path/checkversion.php?locale=$locale HTTP/1.0\r\n";
		$hdr .= "Content-Type: application/x-www-form-urlencoded\r\n";
		$hdr .= "User-Agent: ".get_product_name()."-webGUI/1.0\r\n";
		$hdr .= "Host: ".$host."\r\n";
		$hdr .= "Content-Length: ".strlen($post)."\r\n\r\n";

		fwrite($rfd, $hdr);
		fwrite($rfd, $post);

		$inhdr = true;
		$resp = "";
		while (!feof($rfd)) {
			$line = fgets($rfd);
			if ($inhdr) {
				if (trim($line) === "")
					$inhdr = false;
			} else {
				$resp .= $line;
			}
		}

		fclose($rfd);

		return $resp;
	}

	return null;
}

function get_path_version($rss) {
	$version = get_product_version();

	$resp = "$version";
	// e.g. version = 9.1.0.1 -> 9001, 0001
	if (preg_match("/^.*(\d+)\.(\d+)\.(\d)\.(\d).*$/", $version, $m)) {
		$os_ver = $m[1] * 1000 + $m[2];
		$pd_ver = $m[3] * 1000 + $m[4];
	} else {
		return $resp;
	}

	$xml = @simplexml_load_file($rss);
	if (empty($xml)) return $resp;
	foreach ($xml->channel->item as $item) {
		$title = $item->title;
		$parts = pathinfo($title);
		if ($parts['dirname'] === "/") {
			if (preg_match("/^.*(\d+)\.(\d+)\.(\d)\.(\d).*$/",
			    $parts['basename'], $m)) {
			    	$os_ver2 = $m[1] * 1000 + $m[2];
				$pd_ver2 = $m[3] * 1000 + $m[4];
				$rss_version = sprintf("%d.%d.%d.%d",
				    $m[1], $m[2], $m[3], $m[4]);
				// Compare with rss version, equal or greater?
				if ($os_ver2 > $os_ver
				    || ($os_ver2 == $os_ver && $pd_ver2 >= $pd_ver)) {
				    $resp = $rss_version;
				    break;
				}
			}
		}
	}
	return $resp;
}

function get_latest_file($rss) {
	global $g;
	$product = get_product_name();
	$platform = $g['fullplatform'];
	$version = get_product_version();
	$revision = get_product_revision();
	if (preg_match("/^(.*?)(\d+)$/", $revision, $m)) {
		$revision = $m[2];
	}
	$ext = "img";

	$resp = "";
	$xml = @simplexml_load_file($rss);
	if (empty($xml)) return $resp;
	foreach ($xml->channel->item as $item) {
		$link = $item->link;
		$title = $item->title;
		$date = $item->pubDate;
		$parts = pathinfo($title);
		if (empty($parts['extension']) || strcasecmp($parts['extension'], $ext) != 0)
			continue;
		$filename = $parts['filename'];
		$fullname = $parts['filename'].".".$parts['extension'];

		if (preg_match("/^{$product}-{$platform}-(.*?)\.(\d+)$/", $filename, $m)) {
			$filever = $m[1];
			$filerev = $m[2];
			if ($version < $filever
			    || ($version == $filever && $revision < $filerev)) {
				$resp .= sprintf("<a href=\"%s\" title=\"%s\" target=\"_blank\">%s</a> (%s)",
					htmlspecialchars($link), htmlspecialchars($title),
					htmlspecialchars($fullname), htmlspecialchars($date));
			} else {
				$resp .= sprintf("%s (%s)", htmlspecialchars($fullname),
					htmlspecialchars($date));
			}
			break;
		}
	}
	return $resp;
}

function check_firmware_version_rss($locale) {
	$rss_path = "http://sourceforge.net/api/file/index/project-id/722987/mtime/desc/limit/20/rss";
	$rss_release = "http://sourceforge.net/api/file/index/project-id/722987/path/NAS4Free-@@VERSION@@/mtime/desc/limit/20/rss";
	$rss_beta = "http://sourceforge.net/api/file/index/project-id/722987/path/NAS4Free-Beta/mtime/desc/limit/20/rss";

	// replace with existing version
	$path_version = get_path_version($rss_path);
	if (empty($path_version)) {
		return "";
	}
	$rss_release = str_replace('@@VERSION@@', $path_version, $rss_release);

	$release = get_latest_file($rss_release);
	$beta = get_latest_file($rss_beta);
	$resp = "";
	if (!empty($release)) {
		$resp .= sprintf(gettext("Latest Release: %s"), $release);
		$resp .= "<br />\n";
	}
	if (!empty($beta)) {
		$resp .= sprintf(gettext("Latest Beta Build: %s"), $beta);
		$resp .= "<br />\n";
	}
	return $resp;
}

if ($_POST && !file_exists($d_firmwarelock_path)) {
	unset($input_errors);
	unset($sig_warning);

	if (stristr($_POST['Submit'], gettext("Enable firmware upload")))
		$mode = "enable";
	else if (stristr($_POST['Submit'], gettext("Disable firmware upload")))
		$mode = "disable";
	else if (stristr($_POST['Submit'], gettext("Upgrade firmware")) || $_POST['sig_override'])
		$mode = "upgrade";
	else if ($_POST['sig_no'])
		unlink("{$g['ftmp_path']}/firmware.img");

	if ($mode) {
		if ($mode === "enable") {
			$retval = rc_exec_script("/etc/rc.firmware enable");
			if (0 == $retval) {
				touch($d_fwupenabled_path);
			} else {
				$input_errors[] = gettext("Failed to create in-memory file system.");
			}
		} else if ($mode === "disable") {
			rc_exec_script("/etc/rc.firmware disable");
			if (file_exists($d_fwupenabled_path))
				unlink($d_fwupenabled_path);
		} else if ($mode === "upgrade") {
			if (is_uploaded_file($_FILES['ulfile']['tmp_name'])) {
				/* verify firmware image(s) */
				if (!stristr($_FILES['ulfile']['name'], $g['fullplatform']) && !$_POST['sig_override'])
					$input_errors[] = gettext("The uploaded image file is not for this platform")." ({$g['fullplatform']}).";
				else if (!file_exists($_FILES['ulfile']['tmp_name'])) {
					/* probably out of memory for the MFS */
					$input_errors[] = gettext("Image upload failed (out of memory?)");
				} else {
					/* move the image so PHP won't delete it */
					move_uploaded_file($_FILES['ulfile']['tmp_name'], "{$g['ftmp_path']}/firmware.img");

					if (!verify_gzip_file("{$g['ftmp_path']}/firmware.img")) {
						$input_errors[] = gettext("The image file is corrupt");
						unlink("{$g['ftmp_path']}/firmware.img");
					}
				}
			}

			// Cleanup if there were errors.
			if ($input_errors) {
				rc_exec_script("/etc/rc.firmware disable");
				unlink_if_exists($d_fwupenabled_path);
			}

			// Upgrade firmware if there were no errors.
			if (!$input_errors && !file_exists($d_firmwarelock_path) && (!$sig_warning || $_POST['sig_override'])) {
				touch($d_firmwarelock_path);

				switch($g['platform']) {
					case "embedded":
						rc_exec_script_async("/etc/rc.firmware upgrade {$g['ftmp_path']}/firmware.img");
						break;

					case "full":
						rc_exec_script_async("/etc/rc.firmware fullupgrade {$g['ftmp_path']}/firmware.img");
						break;
				}

				$savemsg = sprintf(gettext("The firmware is now being installed. %s will reboot automatically."), get_product_name());

				// Clean firmwarelock: Permit to force all pages to be redirect on the firmware page.
				if (file_exists($d_firmwarelock_path))
					unlink($d_firmwarelock_path);

				// Clean fwupenabled: Permit to know if the ram drive /ftmp is created.
				if (file_exists($d_fwupenabled_path))
					unlink($d_fwupenabled_path);
			}
		}
	}
} else {
	$mode = "default";
}
if ($mode === "default" || $mode === "enable" || $mode === "disable") {
	if (!isset($config['system']['disablefirmwarecheck'])) {
		$fwinfo = check_firmware_version_rss($locale);
		if (empty($fwinfo)) {
			//$fwinfo = check_firmware_version($locale);
		}
	}
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabcont">
			<?php if ($input_errors) print_input_errors($input_errors); ?>
			<?php if ($savemsg) print_info_box($savemsg); ?>
			<table width="100%" border="0" cellpadding="6" cellspacing="0">
			<?php html_titleline(gettext("Firmware"));?>
			<?php html_text("Current version", gettext("Current Version:"), sprintf("%s %s (%s)", get_product_name(), get_product_version(), get_product_revision()));?>
			<?php html_separator();?>
			<?php if ($fwinfo) {
				html_titleline(gettext("Online version check"));
				echo "<tr id='fwinfo'><td class='vtable' colspan='2'>";
				echo "{$fwinfo}";
				echo "</td></tr>\n";
				html_separator();
			      }
			?>
			</table>
			<?php if (!in_array($g['platform'], $fwupplatforms)): ?>
			<?php print_error_box(gettext("Firmware uploading is not supported on this platform."));?>
			<?php elseif ($sig_warning && !$input_errors): ?>
			<form action="system_firmware.php" method="post">
			<?php
			$sig_warning = "<strong>" . $sig_warning . "</strong><br />".gettext("This means that the image you uploaded is not an official/supported image and may lead to unexpected behavior or security compromises. Only install images that come from sources that you trust, and make sure that the image has not been tampered with.<br /><br />Do you want to install this image anyway (on your own risk)?");
			print_info_box($sig_warning);
			?>
			<input name="sig_override" type="submit" class="formbtn" id="sig_override" value=" Yes ">
			<input name="sig_no" type="submit" class="formbtn" id="sig_no" value=" No ">
			<?php include("formend.inc");?>
			</form>
			<?php else:?>
			<?php if (!file_exists($d_firmwarelock_path)):?>
			<?=gettext("Click &quot;Enable firmware upload&quot; below, then choose the image file to be uploaded.<br />Click &quot;Upgrade firmware&quot; to start the upgrade process.");?>
			<form action="system_firmware.php" method="post" enctype="multipart/form-data">
				<?php if (!file_exists($d_sysrebootreqd_path)):?>
					<?php if (!file_exists($d_fwupenabled_path)):?>
					<div id="submit">
					<input name="Submit" id="Enable" type="submit" class="formbtn" value="<?=gettext("Enable firmware upload");?>" />
					</div>
					<?php else:?>
					<div id="submit">
					<input name="Submit" id="Disable" type="submit" class="formbtn" value="<?=gettext("Disable firmware upload");?>" />
					</div>
					<div id="submit">
					<strong><?=gettext("Firmware image file");?> </strong>&nbsp;<input name="ulfile" type="file" class="formfld" size="40" />
					</div>
					<div id="submit">
					<input name="Submit" id="Upgrade" type="submit" class="formbtn" value="<?=gettext("Upgrade firmware");?>" />
					</div>
					<?php endif;?>
				<?php else:?>
				<strong><?=gettext("You must reboot the system before you can upgrade the firmware.");?></strong>
				<?php endif;?>
				<div id="remarks">
					<?php html_remark("warning", gettext("Warning"), sprintf(gettext("DO NOT abort the firmware upgrade once it has started. %s will reboot automatically after storing the new firmware. The configuration will be maintained.<br />You need a minimum of %d Mb RAM to perform the firmware update.<br />It is strongly recommended that you <a href='%s'>Backup</a> the System configuration before doing a Firmware upgrade."), get_product_name(), 512, "system_backup.php"));?>
				</div>
				<?php include("formend.inc");?>
			</form>
			<?php endif; endif;?>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
