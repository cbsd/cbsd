#!/usr/local/bin/php
<?php
/*
	license.php
	
	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 NAS4Free team <info@nas4free.org>.
	All rights reserved.

	Portions from freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 Olivier Cochard-Labbe <olivier@freenas.org>.
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.

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
// Configure page permission
$pgperm['allowuser'] = TRUE;

require("auth.inc");
require("guiconfig.inc");

$pgtitle = array(gettext("Help"), gettext("License & Credits"));
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabcont">
			<table width="100%" border="0" cellspacing="0" cellpadding="0">
				<?php html_titleline(gettext("License"));?>
				<tr>
					<td class="listt">
            <p><strong>NAS4Free is Copyright &copy; 2012 by NAS4Free Team
              (<a href="mailto:info@nas4free.org">info@nas4free.org</a>).<br />
              All rights reserved.</strong></p>

		<p>NAS4Free use portions of freenas which is Copyright &copy; 2005-2011 by Olivier Cochard (olivier@freenas.org).</p>
		<p>NAS4Free use portions of m0n0wall which is Copyright &copy; 2002-2006 by Manuel Kasper (mk@neon1.net).</p>
		<p>NAS4Free code and documentation are released under the Simplified BSD license, under terms as follows.</p>
            <p> Redistribution and use in source and binary forms, with or without<br />
              modification, are permitted provided that the following conditions
              are met:<br />
              <br />
              1. Redistributions of source code must retain the above copyright
              notice,<br />
              this list of conditions and the following disclaimer.<br />
              <br />
              2. Redistributions in binary form must reproduce the above copyright<br />
              notice, this list of conditions and the following disclaimer in
              the<br />
              documentation and/or other materials provided with the distribution.<br />
              <br />
              <strong>THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND<br />
              ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,<br />
              THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR<br />
              PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE PARTICULAR COPYRIGHT OWNER OR<br />
              CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR<br /> 
              CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE<br />
              GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER<br />
              CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR<br />
              TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS<br />
	      SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.</strong><br />
            <p>The views and conclusions contained in the software and documentation are those of the authors and should<br /> 
               not be interpreted as representing official policies, either expressed or implied, of the NAS4Free Project.</p>
							</td>
							
						</tr>
            <?php html_separator();?>
            <?php html_titleline(gettext("Credits"));?>
            <tr>
            	<td class="listt">
            <p>The following persons have contributed to NAS4Free code:</p>                         
              <div>Daisuke Aoyama (<a href="mailto:aoyama@nas4free.org">aoyama@nas4free.org</a>)<br />              
              &nbsp;&nbsp;&nbsp;&nbsp;<em><font color="#666666">Developer & Project leader</font></em></div><br />
             <div>Michael Zoon (<a href="mailto:zoon01@nas4free.org">zoon01@nas4free.org</a>)<br />             
              &nbsp;&nbsp;&nbsp;&nbsp;<em><font color="#666666">Developer & Project leader</font></em></div><br />
            <hr size="1" />
            <p>The following persons have contributed to NAS4Free support:</p>
              <div>Christian Degen alias bubulein (<a href="mailto:degen@nas4free.org">degen@nas4free.org</a>)<br />
              &nbsp;&nbsp;&nbsp;&nbsp;<em><font color="#666666">User guide and Live support on irc #nas4free|IRC Freenode <a href="http://webchat.freenode.net/?channels=#nas4free">http://webchat.freenode.net</a></font></em></div><br />
              <div>Samuel Tunis alias killermist (<a href="mailto:killermist@nas4free.org">killermist@nas4free.org</a>)<br />
              &nbsp;&nbsp;&nbsp;&nbsp;<em><font color="#666666">User guide and Live support on irc #nas4free|IRC Freenode <a href="http://webchat.freenode.net/?channels=#nas4free">http://webchat.freenode.net</a></font></em></div><br />
            <hr size="1" />
            <p>The following persons have contributed to NAS4Free documentation project:</p>
            <div>Carsten Vinkler (<a href="mailto:carsten@indysign.dk">carsten@indysign.dk</a>)<br />
              &nbsp;&nbsp;&nbsp;&nbsp;<em><font color="#666666">Danish translator of the WebGUI</font></em></div><br />            
            <div>Kris Verhoeven (<a href="mailto:kris@esiv.be">kris@esiv.be</a>)<br />
              &nbsp;&nbsp;&nbsp;&nbsp;<em><font color="#666666">Dutch translator of the WebGUI</font></em></div><br />
            <div>Christophe Lherieau (<a href="skimpax+freenas@gmail.com">skimpax+freenas@gmail.com</a>)<br />
              &nbsp;&nbsp;&nbsp;&nbsp;<em><font color="#666666">French translator of the WebGUI</font></em></div><br />             
            <div>Dominik Plaszewski (<a href="mailto:domme555@gmx.net">domme555@gmx.net</a>)<br />
              &nbsp;&nbsp;&nbsp;&nbsp;<em><font color="#666666">German translator of the WebGUI</font></em></div><br />       
            <div>Petros Kyladitis (<a href="mailto:petros.kyladitis@gmail.com">petros.kyladitis@gmail.com</a>)<br />
              &nbsp;&nbsp;&nbsp;&nbsp;<em><font color="#666666">Greek translator of the WebGUI</font></em></div><br />             
            <div>Christian Sulmoni (<a href="mailto:csulmoni@gmail.com">csulmoni@gmail.com</a>)<br />
              &nbsp;&nbsp;&nbsp;&nbsp;<em><font color="#666666">Italian translator of the WebGUI and QuiXplorer</font></em></div><br />              
            <div>Frederico Tavares (<a href="mailto:frederico-tavares@sapo.pt">frederico-tavares@sapo.pt</a>)<br />
              &nbsp;&nbsp;&nbsp;&nbsp;<em><font color="#666666">Portuguese translator of the WebGUI</font></em></div><br />
            <div>Raul Fernandez Garcia (<a href="mailto:raulfg3@gmail.com">raulfg3@gmail.com</a>)<br />
	      &nbsp;&nbsp;&nbsp;&nbsp;<em><font color="#666666">Spanish translator of the WebGUI</font></em></div><br />
	    <div>Kevin Fowler (<a href="mailto:kfowler@nas4free.org">kfowler@nas4free.org</a>)<br />
              &nbsp;&nbsp;&nbsp;&nbsp;<em><font color="#666666">Webmaster, Documentation & Coder</font></em></div><br />	                                           
						</td></tr>						
            <?php html_separator();?>
            <?php html_titleline(gettext("Software used"));?>
            <tr>
            	<td class="listt">
      <p>NAS4Free is based upon/includes various free software packages, listed
        below.<br />
        The authors of NAS4Free would like to thank the authors of these software
        packages for their efforts.</p>
      <p> FreeBSD (<a href="http://www.freebsd.org" target="_blank">http://www.freebsd.org</a>)<br />
        Copyright &copy; 1995-2012 The FreeBSD Project. All rights reserved.</p>

      <p> geom raid5 <a href="http://wgboome.homepage.t-online.de/geom_raid5.tbz">FreeBSD GEOM RAID5 module</a><br />
        Copyright &copy; 2006-2009 Arne Woerner (<a href="mailto:arne_woerner@yahoo.com">arne_woerner@yahoo.com</a>).</p>

      <p> PHP (<a href="http://www.php.net" target="_blank">http://www.php.net</a>).<br />
        Copyright &copy; 1999-2012 The PHP Group. All rights reserved.</p>

      <p> Lighttpd (<a href="http://www.lighttpd.net" target="_blank">http://www.lighttpd.net</a>)<br />
        Copyright &copy; 2004 by Jan Kneschke (jan@kneschke.de). All rights reserved.</p>

      <p> OpenSSH (<a href="http://www.openssh.com" target="_blank">http://www.openssh.com</a>)<br />
        Copyright &copy; 1999-2009 OpenBSD</p>

      <p> Samba (<a href="http://www.samba.org" target="_blank">http://www.samba.org</a>)<br />
        Copyright &copy; 2007 Free Software Foundation</p>

      <p> Rsync (<a href="http://www.samba.org/rsync" target="_blank">http://www.samba.org/rsync</a>)<br />
        Copyright &copy; 2007 Free Software Foundation</p>

      <p> ProFTPD - Highly configurable FTP server (<a href="http://www.proftpd.org" target="_blank">http://www.proftpd.org</a>)<br />
        Copyright &copy; 1999, 2000-2012 The ProFTPD Project</p>

      <p>tftp-hpa (<a href="http://www.kernel.org/pub/software/network/tftp" target="_blank">http://www.kernel.org/pub/software/network/tftp</a>)<br />
       Copyright &copy; 1999, 2000-2009 The tftp-hpa series is maintained by H. Peter Anvin <hpa@zytor.com>.</p>

      <p> Netatalk (<a href="http://netatalk.sourceforge.net" target="_blank">http://netatalk.sourceforge.net</a>)<br />
        Copyright &copy; 1990,1996 Regents of The University of Michigan</p>

      <p> Apple Bonjour (<a href="http://developer.apple.com/networking/bonjour" target="_blank">http://developer.apple.com/networking/bonjour</a>)<br />
        Apple Public Source License.</p>

      <p> Circular log support for FreeBSD syslogd (<a href="http://software.wwwi.com/syslogd" target="_blank">http://software.wwwi.com/syslogd</a>)<br />
        Copyright &copy; 2001 Jeff Wheelhouse (jdw@wheelhouse.org)</p>

      <p>ataidle (<a href="http://www.cran.org.uk/bruce/software/ataidle.php" target="_blank">http://www.cran.org.uk/bruce/software/ataidle.php</a>)<br />
        Copyright &copy; 2004-2005 Bruce Cran (bruce@cran.org.uk). All rights reserved.</p>

      <p>smartmontools (<a href="http://sourceforge.net/projects/smartmontools" target="_blank">http://sourceforge.net/projects/smartmontools</a>)<br />
        Copyright &copy; 2002-2012 Bruce Allen.</p>

      <p>iSCSI initiator (<a href="ftp://ftp.cs.huji.ac.il/users/danny/freebsd" target="_blank">ftp://ftp.cs.huji.ac.il/users/danny/freebsd</a>)<br />
        Copyright &copy; 2005-2011 Daniel Braniss (danny@cs.huji.ac.il).</p>

      <p>istgt - iSCSI target for FreeBSD (<a href="http://shell.peach.ne.jp/aoyama" target="_blank">http://shell.peach.ne.jp/aoyama</a>)<br />
        Copyright &copy; 2008-2012 Daisuke Aoyama (aoyama@peach.ne.jp). All rights reserved.</p>

      <p>FUPPES - Free UPnP Entertainment Service (<a href="http://fuppes.ulrich-voelkel.de" target="_blank">http://fuppes.ulrich-voelkel.de</a>)<br />
        Copyright &copy; 2005 - 2011 Ulrich V&ouml;lkel (u-voelkel@users.sourceforge.net).</p>

      <p>mt-daapd - Multithread daapd Apple iTunes server (<a href="http://www.fireflymediaserver.org" target="_blank">http://www.fireflymediaserver.org</a>)<br />
        Copyright &copy; 2003 Ron Pedde (ron@pedde.com).</p>

      <p>NTFS-3G driver (<a href="http://www.ntfs-3g.org" target="_blank">http://www.ntfs-3g.org</a>)<br />
        from Szabolcs Szakacsits.</p>

      <p>Fuse - Filesystem in Userspace (<a href="http://fuse.sourceforge.net" target="_blank">http://fuse.sourceforge.net</a>)<br />
       </p>

      <p>e2fsprogs (<a href="http://e2fsprogs.sourceforge.net" target="_blank">http://e2fsprogs.sourceforge.net</a>)<br />
        Copyright &copy; 1994-2006 Theodore Ts'o. All rights reserved.</p>

      <p>inadyn-mt - Simple Dynamic DNS client (<a href="http://sourceforge.net/projects/inadyn-mt" target="_blank">http://sourceforge.net/projects/inadyn-mt</a>)<br />
        Inadyn Copyright &copy; 2003-2004 Narcis Ilisei. All rights reserved.<br />
        Inadyn-mt Copyright &copy; 2007 Bryan Hoover (bhoover@wecs.com).</p>

      <p>XMLStarlet Command Line XML Toolkit (<a href="http://xmlstar.sourceforge.net" target="_blank">http://xmlstar.sourceforge.net</a>)<br />
        Copyright &copy; 2002 Mikhail Grushinskiy. All rights reserved.</p>

      <p>sipcalc (<a href="http://www.routemeister.net/projects/sipcalc" target="_blank">http://www.routemeister.net/projects/sipcalc</a>)<br />
        Copyright &copy; 2003 Simon Ekstrand. All rights reserved.</p>

      <p>msmtp - An SMTP client with a sendmail compatible interface (<a href="http://msmtp.sourceforge.net" target="_blank">http://msmtp.sourceforge.net</a>)<br />
        Copyright &copy; 2008 Martin Lambers and others.</p>

      <p>cdialog - Display simple dialog boxes from shell scripts (<a href="http://invisible-island.net/dialog" target="_blank">http://invisible-island.net/dialog</a>)<br />
        Copyright &copy; 2000-2006, 2007 Thomas E. Dickey.</p>

      <p>host - An utility to query DNS servers<br />
        Rewritten by Eric Wassenaar, Nikhef-H, (e07@nikhef.nl).</p>

      <p>Transmission - Transmission is a fast, easy, and free multi-platform BitTorrent client (<a href="http://www.transmissionbt.com" target="_blank">http://www.transmissionbt.com</a>)<br />
        Copyright &copy; 2008-2012 Transmission Project. All rights reserved.</p>

      <p>QuiXplorer - Web-based file-management (<a href="http://quixplorer.sourceforge.net" target="_blank">http://quixplorer.sourceforge.net</a>)<br />
        Copyright &copy; Felix C. Stegerman. All rights reserved.</p>

      <p>pfSense: NAS4Free use some pfSense code too (<a href="http://www.pfsense.com" target="_blank">http://www.pfsense.com</a>)<br />
        Copyright &copy; 2004, 2005, 2006 Scott Ullrich. All rights reserved.</p>

      <p>Open Virtual Machine Tools (<a href="http://sourceforge.net/projects/open-vm-tools/" target="_blank">http://sourceforge.net/projects/open-vm-tools/</a>)</p>

      <p>VirtualBox Open Source Edition (OSE) (guest additions) (<a href="http://www.virtualbox.org/" target="_blank">http://www.virtualbox.org/</a>)</p>

      <p>LCDproc: A client/server suite for LCD devices (<a href="http://lcdproc.org/" target="_blank">http://lcdproc.org</a>)<br />
	 Copyright &copy; 1998-2006 William Ferrell, Selene Scriven and many other contributors. All rights reserved.</p>

      <p>Some of the software used for NAS4Free are under the <a href="gpl-license.txt">GNU General Public License</a> (<a href="gpl-license.txt">GPLv2</a>, <a href="gpl3-license.txt">GPLv3</a>), <a href="lgpl-license.txt">GNU Lesser General Public License (LGPL)</a>, <a href="apple-license.txt">Apple Public Source License</a> and <a href="php-license.txt">PHP License</a>.</p>

					</td>
				</tr>
			</table>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
