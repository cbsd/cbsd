<?php

/*
** ZFSguru main include
** always included in every request
*/

// start guru global array
$guru = array();

// project data
$guru['product_name']           = 'CBSD';
$guru['product_majorversion']   = 9;
$guru['product_minorversion']   = 2;
$guru['product_revision']       = 0;
$guru['product_suffix']         = '';
$guru['product_version_string'] = $guru['product_majorversion'] . '.' .
                                  $guru['product_minorversion'] . '.' .
                                  $guru['product_revision'] .
                                  $guru['product_suffix'];
$guru['product_url']            = 'http://bsdstore.ru/';

// path locations
$guru['docroot']                = realpath($_SERVER['DOCUMENT_ROOT']) . '/';
$guru['path_services']		= '/services';
$guru['media_systemimage']      = '/cdrom/system.ufs.uzip';
$guru['script_install']         = $guru['docroot'].
				  'scripts/zfsguru_install.php';

// torrent data
$guru['torrent'] = array(
 'path_downloading'	=> $guru['path_services']
			   .'/rtorrent/chroot/rtorrent/files',
 'path_finished'	=> $guru['path_services']
			   .'/rtorrent/chroot/rtorrent/finished',
 'path_torrents'	=> $guru['path_services']
			   .'/rtorrent/chroot/rtorrent/torrents'
);

// remote files
$guru['master_servers']		= array(
 'US' => 'alpha.bsdstore.ru/',
 'EU' => 'bravo.bsdstore.ru/'
);
$guru['remote_version_url']	= 'cbsd_version.txt';
$guru['remote_update_version']	= 0;
$guru['remote_system_url']	= 'cbsd_system.txt';
$guru['remote_system_ident']	= 'systemimage';
$guru['remote_system_version']	= 0;
$guru['remote_services_url']	= 'cbsd_services.txt';
$guru['remote_services_version'] = 0;
$guru['url_changelog']		= '/zfsguru_changelog.html';

// other
$guru['iso_date_format']        = 'Y-M-d @ H:i';
$guru['default_bootfs']         = 'zfsguru';
$guru['benchmark_magic_string'] = 'XX00XXBENCHMARKXX00XX';
$guru['benchmark_poolname']     = 'gurubenchmarkpool';
$guru['benchmark_zvolname']     = 'guruzvoltest';
$guru['recommended_zfsversion'] = array('zpl' => 4, 'spa' => 15);

/*
** Configuration File
** If needed, change this variable to a www-group writable file path
*/
$guru['configuration_file']     = $guru['docroot'] . '/config/config.bin';

/*
** Temporary Directory
** If needed, change this variable to a www-group writable directory
*/
$guru['tempdir']                = '/tmp/';

/*
** File Locations
** If needed, change these accordingly
*/
$guru['required_binaries'] = array(
 'Tar' => '/usr/bin/tar',
 'Sudo' => '/usr/local/bin/sudo',
 'sh' => '/bin/sh'
);
$guru['path'] = array(
 'Samba' => '/usr/local/etc/smb.conf',
 'OpenSSH' => '/etc/ssh/sshd_config'
);
$guru['rc.d'] = array(
 'Lighttpd' => '/usr/local/etc/rc.d/lighttpd',
 'OpenSSH' => '/etc/rc.d/sshd',
 'Samba' => '/usr/local/etc/rc.d/samba',
 'NFS' => '/etc/rc.d/nfsserver',
 'iSCSI' => '/usr/local/etc/rc.d/istgt',
 'powerd' => '/etc/rc.d/powerd'
);
$guru['runcontrol'] = array(
 'Apache' => 'apache22',
 'OpenSSH' => 'sshd',
 'Lighttpd' => 'lighttpd',
 'Samba' => 'samba',
 'NFS' => 'nfs_server',
 'iSCSI' => 'istgt'
);

/* path for Embedded/LiveCD media */
$guru['path_media_mp']                  = '/cdrom';
$guru['path_media_systemfile']          = '/system.ufs.uzip';
$guru['path_livecd']                    = '/dev/iso9660/ZFSGURU-LIVECD';
$guru['path_embedded']                  = '/dev/gpt/ZFSGURU-EMBEDDED';

/*
** Default preferences
** Do NOT change these! Your actual preferences are stored in a file
*/
$guru['default_preferences'] = array(
  'uuid'		=> '',
  'preferred_server'	=> '',
  'connect_timeout'	=> 1,
  'timezone'		=> 'UTC',
  'download_method'	=> 'http',
  'access_control'	=> 2,
  'access_whitelist'	=> '',
  'authentication'	=> '',
  'theme'		=> 'default',
  'advanced_mode'	=> true,
  'offline_mode'	=> false,
  'command_confirm'	=> true,
  'destroy_pools'	=> true
);

/* Include required files */
require('common.php');
require('page.php');
require('procedure.php');

?>
