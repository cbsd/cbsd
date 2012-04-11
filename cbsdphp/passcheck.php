<?
if ( isset($_GET[pass]) )
{
if ( $_GET[pass] )
{
#$token = "ed2e5fc6822f6c9356237c0a5379ffa1";
$pswd = md5(sha1(md5(base64_encode($pass))));
#if ( $token == $pswd )
#{
#echo "Passwords Matched successfully!";
#echo "<br />";
#echo "Logging in now";
echo $pswd;
} else {
echo "error occured";
}
#}
}
?>

