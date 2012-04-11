<?php
session_start();

// is the one accessing this page logged in or not?
if (!isset($_SESSION['db_admin_is_logged_in'])
   || $_SESSION['db_admin_is_logged_in'] !== true) {

   // not logged in, move to login page
   header('Location: /admin/login.php');
   exit;
}
include('../../../includes/config.php');

//print_r($_POST);
//print_r($_GET);


if (isset($_GET['vm'])) {
$jname=$_GET['vm'];
switch ($_GET['status']) {
case "stop": echo "Popup windows here: STOPING $jname";
$fp = popen("/usr/local/bin/sudo /usr/local/bin/cbsd jstop ${jname}","r");
while (($buffer=fgets($fp,4096))!==false) {
echo "$buffer"."<br>";
flush();
}
pclose($fp);
break;

case "start": echo "Popup windows here: STARTING $jname";
$fp = popen("/usr/local/bin/sudo /usr/local/bin/cbsd jstart ${jname}","r");
while (($buffer=fgets($fp,4096))!==false) {
echo "$buffer"."<br>";
flush();
}
pclose($fp);
break;

}
}


printf("Check for repository content...");
$fp = popen('/usr/local/bin/sudo /usr/local/bin/cbsd repo check -q 2>&1', 'r');
if ($fp) {
$a=fscanf($fp,"%s");
list($status)=$a;
flush();
pclose($fp);
}

if (strcasecmp($status,"Offline")==0) return;
echo "Online<br>";

$fp=popen('uname -m','r');
$a=fscanf($fp,"%s");
list($arch)=$a;
pclose($fp);

$fp=popen('uname -r |cut -d "-" -f 1 |tr "\." "_"','r');
$a=fscanf($fp,"%s");
list($rel)=$a;
pclose($fp);
echo "<br>My arch: $arch<br>My ver: $rel<br><br>";
$fp = popen('/usr/local/bin/sudo /usr/local/bin/cbsd repo lssrc 2>&1', 'r');
if ($fp) {
while (($buffer=fgets($fp,4096))!==false) {
echo "$buffer"."<br>";
flush();
}
pclose($fp);
}
echo "<br>";
$fp = popen('/usr/local/bin/sudo /usr/local/bin/cbsd repo lsobj >&1', 'r');
if ($fp) {
while (($buffer=fgets($fp,4096))!==false) {
echo "$buffer"."<br>";
flush();
}
pclose($fp);
}
echo "<br>";
$fp = popen('/usr/local/bin/sudo /usr/local/bin/cbsd repo lsbase 2>&1', 'r');
if ($fp) {
while (($buffer=fgets($fp,4096))!==false) {
echo "$buffer"."<br>";
flush();
}
pclose($fp);
}
echo "<br>";
$fp = popen('/usr/local/bin/sudo /usr/local/bin/cbsd repo lsimg 2>&1', 'r');
if ($fp) {
while (($buffer=fgets($fp,4096))!==false) {
echo "$buffer"."<br>";
flush();
}
pclose($fp);
}









?>
