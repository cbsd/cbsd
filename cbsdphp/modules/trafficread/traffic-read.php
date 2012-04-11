<?
include ('../../includes/config.php');
 mysql_connect("$MySQL_Host","$MySQL_User","$MySQL_Passw");
 
 $HN=trim(addslashes($_GET["HN"])); // Hardware Node
 
 $handle = fopen ("tmp/$HN-traffic","r");
 while (!feof($handle)) {
   $line = fgets($handle, 4096);
   list($date,$time,$ip,$traffic)=explode(" ",$line);
   if($traffic>0) {mysql($db,"insert into Traffic (ip,measuringtime,bytes) values('$ip','$date $time','$traffic')");}
 } 
 fclose($handle);
?>
