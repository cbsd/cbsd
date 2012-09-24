#!/usr/local/bin/php
<?php
/*
	services_status.php
	
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

$pgtitle = array(gettext("Status"), gettext("Services"));

$a_service[] = array("desc" => gettext("CIFS/SMB"), "link" => "services_samba.php", "config" => "samba", "scriptname" => "samba");
$a_service[] = array("desc" => gettext("FTP"), "link" => "services_ftp.php", "config" => "ftpd", "scriptname" => "proftpd");
$a_service[] = array("desc" => gettext("TFTP"), "link" => "services_tftp.php", "config" => "tftpd", "scriptname" => "tftpd");
$a_service[] = array("desc" => gettext("SSH"), "link" => "services_sshd.php", "config" => "sshd", "scriptname" => "sshd");
$a_service[] = array("desc" => gettext("NFS"), "link" => "services_nfs.php", "config" => "nfsd", "scriptname" => "nfsd");
$a_service[] = array("desc" => gettext("AFP"), "link" => "services_afp.php", "config" => "afp", "scriptname" => "afpd");
$a_service[] = array("desc" => gettext("RSYNC"), "link" => "services_rsyncd.php", "config" => "rsyncd", "scriptname" => "rsyncd");
$a_service[] = array("desc" => gettext("Unison"), "link" => "services_unison.php", "config" => "unison", "scriptname" => "unison");
$a_service[] = array("desc" => gettext("iSCSI Target"), "link" => "services_iscsitarget.php", "config" => "iscsitarget", "scriptname" => "iscsi_target");
$a_service[] = array("desc" => gettext("UPnP"), "link" => "services_upnp.php", "config" => "upnp", "scriptname" => "fuppes");
$a_service[] = array("desc" => gettext("iTunes/DAAP"), "link" => "services_daap.php", "config" => "daap", "scriptname" => "mt-daapd");
$a_service[] = array("desc" => gettext("Dynamic DNS"), "link" => "services_dynamicdns.php", "config" => "dynamicdns", "scriptname" => "inadyn");
$a_service[] = array("desc" => gettext("SNMP"), "link" => "services_snmp.php", "config" => "snmpd", "scriptname" => "bsnmpd");
$a_service[] = array("desc" => gettext("UPS"), "link" => "services_ups.php", "config" => "ups", "scriptname" => "nut");
$a_service[] = array("desc" => gettext("Webserver"), "link" => "services_websrv.php", "config" => "websrv", "scriptname" => "websrv");
$a_service[] = array("desc" => gettext("BitTorrent"), "link" => "services_bittorrent.php", "config" => "bittorrent", "scriptname" => "transmission");
$a_service[] = array("desc" => gettext("LCDproc"), "link" => "services_lcdproc.php", "config" => "lcdproc", "scriptname" => "LCDd");
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabcont">
			<form action="services_info.php" method="post">
				<table width="100%" border="0" cellpadding="0" cellspacing="0">
					<tr>
						<td width="90%" class="listhdrlr"><?=gettext("Service");?></td>
						<td width="5%" class="listhdrc"><?=gettext("Enabled");?></td>
						<td width="5%" class="listhdrc"><?=gettext("Status");?></td>
					</tr>
					<?php foreach ($a_service as $servicev):?>
					<tr>
						<?php $enable = isset($config[$servicev['config']]['enable']);?>
						<?php $status = rc_is_service_running($servicev['scriptname']);?>
						<td class="<?=$enable?"listlr":"listlrd";?>"><?=htmlspecialchars($servicev['desc']);?>&nbsp;</td>
						<td class="<?=$enable?"listrc":"listrcd";?>">
							<a href="<?=$servicev['link'];?>">
								<?php if ($enable):?>
								<img src="status_enabled.png" border="0" alt="" />
								<?php else:?>
								<img src="status_disabled.png" border="0" alt="" />
								<?php endif;?>
							</a>
						</td>
						<td class="<?=$enable?"listrc":"listrcd";?>">
							<?php if (0 === $status):?>
							<a title="<?=gettext("Running");?>"><img src="status_enabled.png" border="0" alt="" /></a>
							<?php else:?>
							<a title="<?=gettext("Stopped");?>"><img src="status_disabled.png" border="0" alt="" /></a>
							<?php endif;?>
						</td>
					</tr>
					<?php endforeach;?>
				</table>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
