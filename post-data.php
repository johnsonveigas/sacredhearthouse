<?php


date_default_timezone_set('Asia/Kolkata');

// Database connection parameters
$servername = "localhost";
$username = "sacredh1_waterlevel";
$password = "6363081958@johnson";
$dbname = "sacredh1_waterlevel";
$port = 3306;

// Function to sanitize input data
function test_input($data) {
    $data = trim($data);
    $data = stripslashes($data);
    $data = htmlspecialchars($data);
    return $data;
}

// Check if the request is a POST request
if ($_SERVER["REQUEST_METHOD"] == "POST") {
    // Check API key
    if (!isset($_POST["api_key"]) || $_POST["api_key"] !== "1234") {
        die("Invalid API key.");
    }

    // Extract and sanitize the input fields
    $id = isset($_POST["id"]) ? (int)test_input($_POST["id"]) : null;
    $ackno = isset($_POST["ackno"]) ? (int)test_input($_POST["ackno"]) : null;
    $buzzer = isset($_POST["buzzer"]) ? (int)test_input($_POST["buzzer"]) : null;
    $level1 = isset($_POST["level1"]) ? (int)test_input($_POST["level1"]) : null;
    $level2 = isset($_POST["level2"]) ? (int)test_input($_POST["level2"]) : null;
    $sen_status = isset($_POST["sen_status"]) ? (int)test_input($_POST["sen_status"]) : null;
    $error_code = isset($_POST["error_code"]) ? test_input($_POST["error_code"]) : null;
    $error_description = isset($_POST["error_description"]) ? test_input($_POST["error_description"]) : null;

    // Database connection
    $conn = new mysqli($servername, $username, $password, $dbname, $port);
    if ($conn->connect_error) {
        die("Connection failed: " . $conn->connect_error);
    }

    $conn->query("SET time_zone = '+05:30';");
    // Check if the record with the given id already exists
    $checkSql = "SELECT * FROM TankData WHERE id = $id";
    $result = $conn->query($checkSql);

    // Prepare SQL query for insert or update
    if ($result->num_rows > 0) {
        $sql = "UPDATE TankData SET last_update = NOW()";
        if (!is_null($ackno)) $sql .= ", Ackno='$ackno'";
        if (!is_null($buzzer)) $sql .= ", Buzzer='$buzzer'";
        if (!is_null($level1)) $sql .= ", level1='$level1'";
        if (!is_null($level2)) $sql .= ", level2='$level2'";
        if (!is_null($sen_status)) $sql .= ", sen_status='$sen_status'";
        if (!is_null($error_code)) $sql .= ", error_code='$error_code'";
        if (!is_null($error_description)) $sql .= ", error_description='$error_description'";
        $sql .= " WHERE id=$id";
    } else {
        $columns = "id, last_update";
        $values = "$id, NOW()";
        if (!is_null($ackno)) { $columns .= ", Ackno"; $values .= ", '$ackno'"; }
        if (!is_null($buzzer)) { $columns .= ", Buzzer"; $values .= ", '$buzzer'"; }
        if (!is_null($level1)) { $columns .= ", level1"; $values .= ", '$level1'"; }
        if (!is_null($level2)) { $columns .= ", level2"; $values .= ", '$level2'"; }
        if (!is_null($sen_status)) { $columns .= ", sen_status"; $values .= ", '$sen_status'"; }
        if (!is_null($error_code)) { $columns .= ", error_code"; $values .= ", '$error_code'"; }
        if (!is_null($error_description)) { $columns .= ", error_description"; $values .= ", '$error_description'"; }
        $sql = "INSERT INTO TankData ($columns) VALUES ($values)";
    }

    // Execute SQL query
    if ($conn->query($sql) === TRUE) {
        echo "Record processed successfully";
    } else {
        echo "Error: " . $sql . "<br>" . $conn->error;
    }

    // Log error in error_history if error_code and error_description are provided
    if (!empty($error_code) && !empty($error_description)) {
        $errorSql = "INSERT INTO error_history (error_code, error_description) VALUES ('$error_code', '$error_description')";
        if (!$conn->query($errorSql)) {
            echo "Error logging to error_history: " . $conn->error;
        }
    }

    // Add data to the historical_val table with value capping at 100
    $historical_level1 = ($level1 > 100) ? 100 : $level1;
    $historical_level2 = ($level2 > 100) ? 100 : $level2;

    $historicalSql = "INSERT INTO historical_val (level1, level2, last_update) VALUES ('$historical_level1', '$historical_level2', NOW())";
    if ($conn->query($historicalSql) === TRUE) {
        echo "Historical data added successfully.";
    } else {
        echo "Error inserting historical data: " . $conn->error;
    }

    // Close the connection
    $conn->close();
} else {
    echo "No data sent via HTTP POST.";
}
?>