<?php
// CORS headers
header("Access-Control-Allow-Origin: *"); // Allow requests from any origin
header("Access-Control-Allow-Methods: POST"); // Allow only POST requests
header("Access-Control-Allow-Headers: Content-Type"); // Allow Content-Type header

// Database connection parameters
$servername = "localhost";
$username = "sacredh1_waterlevel";
$password = "6363081958@johnson";
$dbname = "sacredh1_waterlevel";
$port = 3306;

// Function to sanitize input data to avoid SQL injection and other security risks
function test_input($data) {
    $data = trim($data);  // Removes extra spaces
    $data = stripslashes($data);  // Removes slashes from input
    $data = htmlspecialchars($data);  // Converts special characters to HTML entities
    return $data;
}

// Check if the request is a POST request
if ($_SERVER["REQUEST_METHOD"] == "POST") {
    // Check if the API key is present and valid
    if (!isset($_POST["api_key"]) || $_POST["api_key"] !== "1234") {
        die("Invalid API key.");
    }

    // Extract and sanitize the Ackno parameter
    $id = (int)test_input($_POST["id"]);
    $Ackno = (int)test_input($_POST["ackno"]);

    // Create a connection to the MySQL database
    $conn = new mysqli($servername, $username, $password, $dbname, $port);

    // Check if the connection was successful
    if ($conn->connect_error) {
        die("Connection failed: " . $conn->connect_error);
    }

    // Check if the record with the given id already exists
    $checkSql = "SELECT * FROM TankData WHERE id = $id";
    $result = $conn->query($checkSql);

    if ($result->num_rows > 0) {
        // If record exists, update only the Ackno field
        $sql = "UPDATE TankData SET Ackno='$Ackno' WHERE id=$id";
    } else {
        // If record does not exist, insert it without last_update
        $sql = "INSERT INTO TankData (id, Ackno) VALUES ($id, $Ackno)";
    }

    // Execute the SQL query and check if it was successful
    if ($conn->query($sql) === TRUE) {
        echo "Ackno processed successfully";
    } else {
        echo "Error: " . $sql . "<br>" . $conn->error;
    }

    // Close the connection to the database
    $conn->close();
} else {
    // Respond with an error if no POST data was received
    echo "No data sent via HTTP POST.";
}
?>