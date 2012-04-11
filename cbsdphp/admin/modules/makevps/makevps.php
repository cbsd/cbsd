<?php
//this code makesure you are registered and logged in before you can mess with teh system
session_start();

// is the one accessing this page logged in or not?
if (!isset($_SESSION['db_admin_is_logged_in'])
   || $_SESSION['db_admin_is_logged_in'] !== true) {

   // not logged in, move to login page
   header('Location: ../../login.php');
   exit;
}
include('../../../includes/config.php');
//connect to your database ** EDIT REQUIRED HERE **
mysql_connect("$MySQL_Host","$MySQL_User","$MySQL_Passw"); //(host, username, password)

//specify database ** EDIT REQUIRED HERE **
mysql_select_db("$db") or die("Unable to select database"); //select which database we're using
// SQL query to change
$query = "INSERT INTO `Servers` (`user`, `vpsid`, `ip1`, `ip2`, `dns1`, `dns2`, `dedram`, `burstram`, `diskspace`, `hostname`, `rootpass`, `ostemplate`, `onboot`, 
`rebuild`) VALUES ('".@$_POST['user']."', '".@$_POST['vid']."', '".@$_POST['ip1']."', '".@$_POST['ip2']."', '".@$_POST['dns1']."', '".@$_POST['dns2']."', '".@$_POST['dram']."', '".@$_POST['bram']."', '".@$_POST['disk']."', '".@$_POST['host']."', '".@$_POST['pswd']."', '".@$_POST['os']."', 'yes', 'yes');";
$result = mysql_query($query) or die("Couldn't execute query");


  echo "The VPS $vpsid is scheduled to be built with the following info: ";
  echo "<br/>";
  echo "fosvm User: ";
  echo @$_POST['user'];
  echo "<br/>";
  echo "VPS-ID: ";
  echo @$_POST['vid'];
  echo "<br/>";
  echo "IP-1: ";
  echo @$_POST['ip1'];
  echo "<br/>";
  echo "IP-2: ";
  echo @$_POST['ip2'];
  echo "<br/>";
  echo "DNS-1: ";
  echo @$_POST['dns1'];
  echo "<br/>";
  echo "DNS-2: ";
  echo @$_POST['dns2'];
  echo "<br/>";
  echo "Dedicated RAM (MB): ";
  echo @$_POST['dram'];
  echo "<br/>";
  echo "Burstable RAM (MB): ";
  echo @$_POST['bram'];
  echo "<br/>";
  echo "DiskSpace (GB): ";
  echo @$_POST['disk'];
  echo "<br/>";
  echo "Hostname: ";
  echo @$_POST['host'];
  echo "<br/>";
  echo "root Password: ";
  echo @$_POST['pswd'];
  echo "<br/>";
  echo "OS Template: ";
  echo @$_POST['os'];
  echo "<br/>";
  echo "**Note**";
  echo "It may take upto 5 minutes for the VPS to be created";
  echo "<br/><br/><a href=\"index.php\">Back</a>"
?>

