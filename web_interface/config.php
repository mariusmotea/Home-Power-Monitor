<?php
$sensorIp = '192.168.10.99'; //put here your sensors ip. Set dhcp reservation, or static ip for long term usage
$connection = mysqli_connect('{database host ip}', '{username}', '{password}');

if (!$connection){
    die("Database Connection Failed" . mysqli_error($connection));
}
$select_db = mysqli_select_db($connection, 'powermeter');
if (!$select_db){
    die("Database Selection Failed" . mysqli_error($connection));
}
?>
