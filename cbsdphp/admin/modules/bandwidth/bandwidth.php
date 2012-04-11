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
?>

<?php

function fileSizeInfo($fs)
{
 $bytes = array('kb', 'kb', 'mb', 'GB', 'TB');
 // values are always displayed in at least 1 kilobyte:
 if ($fs <= 999) {
  $fs = 1;
 }
 for ($i = 0; $fs > 999; $i++) {
  $fs /= 1024;
 }
 return array(round($fs, 2), $bytes[$i]);
}


include('../../../includes/config.php');
  // Get the search variable from URL

  $var = @$_POST['q'] ;
  $trimmed = trim($var); //trim whitespace from the stored variable

// rows to return
$limit=10; 

// check for an empty string and display a message.
if ($trimmed == "")
  {
echo "<br/><br/>";
echo "**Note**<br/>";
echo "Bandwidth usage updates once a minute<br/>";

  exit;
  }

// check for a search parameter
if (!isset($var))
  {
  echo "<p>We dont seem to have a search parameter!</p>";
  exit;
  }


//connect to your database ** EDIT REQUIRED HERE **
mysql_connect("$MySQL_Host","$MySQL_User","$MySQL_Passw"); //(host, username, password)

//specify database ** EDIT REQUIRED HERE **
mysql_select_db("$db") or die("Unable to select database"); //select which database we're using

// Build SQL Query  
$query = "SELECT ip,sum(bytes) FROM Traffic WHERE ip = \"$trimmed\" and month(measuringtime) = month(current_date)";
 // EDIT HERE and specify your table and field names for the SQL query

 $numresults=mysql_query($query);
 $numrows=mysql_num_rows($numresults);

// If we have no results, offer a google search as an alternative

if ($numrows == 0)
  {
  echo "<h4>Results</h4>";
  echo "<p>Sorry, your search: &quot;" . $trimmed . "&quot; returned zero results</p>";
  }

// next determine if s has been passed to script, if not use 0
  if (empty($s)) {
  $s=0;
  }

// get results
  $query .= " limit $s,$limit";
  $result = mysql_query($query) or die("Couldn't execute query");

// display what the person searched for

// begin to show results set
//$count = 1 + $s ;

// now you can display the results returned
//  while ($row= mysql_fetch_array($result)) {
//  $bwbc = $row["sum(bytes)"];
//  $ipadd = $row['ip'];

//$bwconarr = fileSizeInfo($bwbc);
//$bwcons = $bwconarr["0"];
//$bwconi = $bwconarr["1"];

//  echo "<i>Current Monthly Bandwidth Usage</i><br/>";
//  echo "$ipadd&nbsp;-&nbsp;$bwcons$bwconi" ;
//  $count++ ;
//  }


  
?>

<br/>
<a href="../../index.php">Home</a>
