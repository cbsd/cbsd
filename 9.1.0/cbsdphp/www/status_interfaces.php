#!/usr/local/bin/php
<?php
/*
	status_interfaces.php
	
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

$pgtitle = array(gettext("Status"), gettext("Interfaces"));
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabcont">
		  <table width="100%" border="0" cellspacing="0" cellpadding="0">
		    <?php $i = 0; $ifdescrs = array('lan' => 'LAN');
		    for ($j = 1; isset($config['interfaces']['opt' . $j]); $j++) {
		      $ifdescrs['opt' . $j] = $config['interfaces']['opt' . $j]['descr'];
		    }
		    foreach ($ifdescrs as $ifdescr => $ifname):
		      $ifinfo = get_interface_info_ex($ifdescr);
		    ?>
		      <?php if ($i): ?>
		      <tr>
		        <td colspan="8" class="list" height="12"></td>
		      </tr>
		      <?php endif; ?>
		      <tr>
		        <td colspan="2" class="listtopic">
		          <?=sprintf(gettext("%s interface"), htmlspecialchars($ifname));?>
		        </td>
		      </tr>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("Name");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['hwif']);?>
		        </td>
		      </tr>
		      <?php if ($ifinfo['dhcplink']): ?>
		  	  <tr>
		  		  <td width="22%" class="vncellt"><?=gettext("DHCP");?></td>
		  			<td width="78%" class="listr">
		  			  <?=htmlspecialchars($ifinfo['dhcplink']);?>&nbsp;&nbsp;
		  			  <?php if ($ifinfo['dhcplink'] == "up"): ?>
		  			  <input type="submit" name="submit" value="Release" class="formbtns" />
		  			  <?php else: ?>
		  			  <input type="submit" name="submit" value="Renew" class="formbtns" />
		  			  <?php endif; ?>
		  			</td>
		  	  </tr>
		      <?php endif; if ($ifinfo['pppoelink']): ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("PPPoE");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['pppoelink']);?>&nbsp;&nbsp;
		  			  <?php if ($ifinfo['pppoelink'] == "up"): ?>
		  			  <input type="submit" name="submit" value="Disconnect" class="formbtns" />
		  			  <?php else: ?>
		  			  <input type="submit" name="submit" value="Connect" class="formbtns" />
		  			  <?php endif; ?>
		        </td>
		      </tr>
		      <?php  endif; if ($ifinfo['pptplink']): ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("PPTP");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['pptplink']);?>&nbsp;&nbsp;
		  			  <?php if ($ifinfo['pptplink'] == "up"): ?>
		  			  <input type="submit" name="submit" value="Disconnect" class="formbtns" />
		  			  <?php else: ?>
		  			  <input type="submit" name="submit" value="Connect" class="formbtns" />
		  			  <?php endif; ?>
		        </td>
		      </tr>
		      <?php  endif; if ($ifinfo['macaddr']): ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("MAC address");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['macaddr']);?>
		        </td>
		      </tr>
		      <?php endif; if ($ifinfo['status'] != "down"): ?>
		  		<?php if ($ifinfo['dhcplink'] != "down" && $ifinfo['pppoelink'] != "down" && $ifinfo['pptplink'] != "down"): ?>
		  		<?php if ($ifinfo['ipaddr']): ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("IP address");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['ipaddr']);?>&nbsp;
		        </td>
		      </tr>
		      <?php endif; ?><?php if ($ifinfo['subnet']): ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("Subnet mask");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['subnet']);?>
		        </td>
		      </tr>
		      <?php endif; ?><?php if ($ifinfo['gateway']): ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("Gateway");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['gateway']);?>
		        </td>
		      </tr>
			  <?php endif; ?><?php if ($ifinfo['ipv6addr']): ?>
			  <tr>
		        <td width="22%" class="vncellt"><?=gettext("IPv6 address");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['ipv6addr']);?>&nbsp;
		        </td>
		      </tr>
		      <?php endif; ?><?php if ($ifinfo['ipv6subnet']): ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("IPv6 Prefix");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['ipv6subnet']);?>
		        </td>
		      </tr>
		      <?php endif; ?><?php if ($ifinfo['ipv6gateway']): ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("IPv6 Gateway");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['ipv6gateway']);?>
		        </td>
		      </tr>
		      <?php endif; if ($ifdescr == "wan" && file_exists("{$g['varetc_path']}/nameservers.conf")): ?>
		      <td width="22%" class="vncellt"><?=gettext("ISP DNS servers");?></td>
		      <td width="78%" class="listr"><?php echo nl2br(file_get_contents("{$g['varetc_path']}/nameservers.conf")); ?></td>
		      <?php endif; endif; if ($ifinfo['media']): ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("Media");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['media']);?>
		        </td>
		      </tr>
		      <?php endif; ?><?php if ($ifinfo['channel']): ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("Channel");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['channel']);?>
		        </td>
		      </tr>
		      <?php endif; ?><?php if ($ifinfo['ssid']): ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("SSID");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['ssid']);?>
		        </td>
		      </tr>
		      <?php endif; ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("MTU");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['mtu']);?>
		        </td>
		      </tr>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("I/O packets");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['inpkts'] . "/" . $ifinfo['outpkts'] . " (" . format_bytes($ifinfo['inbytes']) . "/" . format_bytes($ifinfo['outbytes']) . ")");?>
		        </td>
		      </tr>
		      <?php if (isset($ifinfo['inerrs'])): ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("I/O errors");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['inerrs'] . "/" . $ifinfo['outerrs']);?>
		        </td>
		      </tr>
		      <?php endif; ?><?php if (isset($ifinfo['collisions'])): ?>
		      <tr>
		        <td width="22%" class="vncellt"><?=gettext("Collisions");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['collisions']);?>
		        </td>
		      </tr>
		      <?php endif; ?>
		  	  <?php endif; ?>
		  	  <tr>
		        <td width="22%" class="vncellt"><?=gettext("Status");?></td>
		        <td width="78%" class="listr">
		          <?=htmlspecialchars($ifinfo['status']);?>
		        </td>
		      </tr>
		    <?php $i++; endforeach; ?>
		  </table>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
