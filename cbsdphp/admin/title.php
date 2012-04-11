<?php
//this code makesure you are registered and logged in before you can mess with teh system
session_start();

// is the one accessing this page logged in or not?
if (!isset($_SESSION['db_admin_is_logged_in'])
   || $_SESSION['db_admin_is_logged_in'] !== true) {

   // not logged in, move to login page
   header('Location: login.php');
   exit;
}
?>
<br/>

<?php
include('../includes/config.php');
include('modules/bandwidth/all_bw.php');
include('modules/nodectl/nodestatus.php');

echo "<br/><br/>";
//echo "**Note**<br/>";
//echo "Bandwidth usage updates once a minute<br/>";
?>

