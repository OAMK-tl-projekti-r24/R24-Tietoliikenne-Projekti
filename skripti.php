<?php

$servername = "172.20.241.9";
$username = "dbaccess_rw"; // katso discordin pinned-viesteistä
$password = "fasdjkf2389vw2c3k234vk2f3"; // katso discordin pinned-viesteistä
$dbname = "measurements";
$groupid = 24; // oma groupid

date_default_timezone_set('Europe/Helsinki');
echo date(DATE_RFC2822) . "<br>";

$conn = new mysqli($servername, $username, $password, $dbname);

if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
        }

$sql = "SELECT timestamp, groupid, from_mac, to_mac, sensorvalue_a, sensorvalue_b, sensorvalue_c, sensorvalue_d, sensorvalue_e, sensorvalue_f FROM rawdata WHERE groupid = $groupid ORDER BY id DESC LIMIT 20";
$result = $conn->query($sql);

if ($result->num_rows > 0) {
   while($row = $result->fetch_assoc()) {
     echo "Time: " . $row["timestamp"] . " - Sensor A: " . $row["sensorvalue_a"] . " - Sensor B: " . $row["sensorvalue_b"]. " - Sensor C: " . $row["sensorvalue_c"]. " - Sensor D: " . $row["sensorvalue_d"]. "<br>";
   }
}
else
{
echo"No results";
}
$conn->close();

?>
