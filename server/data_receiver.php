<?php
$host = "your_host_name";
$db = "sensor_data";
$user = "your_user_name";
$pass = "your_password";

$conn = new mysqli($host, $user, $pass, $db);
if ($conn->connect_error) {
    die("Connection error: " . $conn->connect_error);
}

// Gerekli veriler geldi mi kontrol et
if (isset($_POST['temperature'], $_POST['humidity'], $_POST['mq135'], $_POST['ldr'])) {
    $temperature = $_POST['temperature'];
    $humidity    = $_POST['humidity'];
    $mq135       = $_POST['mq135'];
    $ldr         = $_POST['ldr'];

    // Prepared statement kullan
    $stmt = $conn->prepare("INSERT INTO sensor_data (temperature, humidity, mq135, ldr, time_col) VALUES (?, ?, ?, ?, CURTIME())");
    $stmt->bind_param("ddii", $temperature, $humidity, $mq135, $ldr);

    if ($stmt->execute()) {
        echo "Data saved!";
    } else {
        echo "Error: " . $stmt->error;
    }

    $stmt->close();
} else {
    echo "Required data missing.";
}

$conn->close();
?>
