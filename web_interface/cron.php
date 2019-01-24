<?php
require_once('config.php');
$json = file_get_contents('http://' . $sensorIp);
$obj = json_decode($json, true);
$sql = "INSERT INTO data (time, senz1, senz2, senz3, senz4, senz5, senz6, senz7, senz8) VALUES (NOW(), " . $obj["in1"] . ", " .  $obj["in2"] . ", " . $obj["in3"] . ", " . $obj["in4"] . ", " . $obj["in5"] . ", " . $obj["in6"] . ", " . $obj["in7"] . ", " . $obj["in8"] . ");";
mysqli_query($connection, $sql);
 ?>
