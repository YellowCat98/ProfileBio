<?php
$dbHost = "sql4.5v.pl";
$dbUsername = "sus69420_yellowcat98";
$dbPassword = "5df2aa9c1f";
$dbName = "sus69420_yellowcat98";

// Create connection
$conn = new mysqli($dbHost, $dbUsername, $dbPassword, $dbName);

// Check connection
if ($conn->connect_error) {
    echo(-1);
}
?>