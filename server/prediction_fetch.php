<?php
$host = "your_host_name";
$db = "sensor_data";
$user = "your_user_name";
$pass = "your_password";

header("Content-Type: application/json");

$conn = new mysqli($host, $user, $pass, $db);
if ($conn->connect_error) {
    http_response_code(500);
    echo json_encode(["error" => "Database connection error: " . $conn->connect_error]);
    exit();
}

$sql = "SELECT predicted_label, prediction_time
        FROM predictions
        ORDER BY id DESC
        LIMIT 1";

$result = $conn->query($sql);

if ($result && $result->num_rows > 0) {
    $row = $result->fetch_assoc();
    echo json_encode([
        "predicted_label" => $row['predicted_label'],
        "prediction_time" => $row['prediction_time']
    ]);
} else {
    // Returns empty data if there is no prediction at all.
    echo json_encode([
        "predicted_label" => "veri_yok",
        "prediction_time" => null
    ]);
}

$conn->close();
?>
