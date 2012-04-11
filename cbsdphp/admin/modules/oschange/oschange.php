<?php
include('../../session.php');
  $var = @$_POST['os'] ;
  $var2 = @$_POST['vid'] ;
  $trimmed = trim($var); //trim whitespace from the stored variable
  $cid = trim($var2);
include('../../../includes/config.php');
//connect to your database ** EDIT REQUIRED HERE **
mysql_connect("$MySQL_Host","$MySQL_User","$MySQL_Passw"); //(host, username, password)

//specify database ** EDIT REQUIRED HERE **
mysql_select_db("$db") or die("Unable to select database"); //select which database we're using
// SQL query to change
$query2 = "UPDATE `fosvm`.`Servers` SET `rebuild` = 'yes', `ostemplate` = '".$trimmed."' WHERE `vpsid` = '".$cid."';";
$result2 = mysql_query($query2) or die("Couldn't execute query");


  echo "The VPS $vpsid is scheduled to be reformatted with the following template: ";
  echo "<i>";
  echo $trimmed;
  echo "</i><br/>";
  echo "**Note**";
  echo "It may take upto 5 minutes for the VPS to be updated";
  echo "<br/><br/><a href=\"index.php\">Back</a>"
?>
