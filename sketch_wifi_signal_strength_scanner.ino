#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Set the network name and password for your scanner dashboard
const char* ssid = "ESP_Scanner_Hub";
const char* password = "password123";

ESP8266WebServer server(80);

// The main HTML page with embedded CSS and JavaScript
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Wi-Fi Spectrum Analyzer</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #0d1117; color: #c9d1d9; text-align: center; margin: 0; padding: 20px; }
    .container { max-width: 600px; margin: auto; background-color: #161b22; padding: 20px; border-radius: 10px; border: 1px solid #30363d; box-shadow: 0 4px 15px rgba(0,0,0,0.5); }
    h2 { color: #58a6ff; margin-bottom: 5px; }
    p { color: #8b949e; font-size: 0.9em; margin-bottom: 20px; }
    .btn { background-color: #238636; color: white; border: none; padding: 12px 24px; font-size: 16px; font-weight: bold; border-radius: 6px; cursor: pointer; transition: 0.2s; }
    .btn:hover { background-color: #2ea043; }
    .btn:disabled { background-color: #21262d; color: #8b949e; cursor: not-allowed; }
    table { width: 100%; border-collapse: collapse; margin-top: 20px; font-size: 0.9em; }
    th, td { padding: 12px; text-align: left; border-bottom: 1px solid #30363d; }
    th { color: #58a6ff; }
    .bar-bg { width: 100%; background-color: #21262d; border-radius: 4px; overflow: hidden; height: 10px; margin-top: 4px; }
    .bar-fg { height: 100%; background-color: #3fb950; }
    .bar-fg.fair { background-color: #d29922; }
    .bar-fg.weak { background-color: #f85149; }
    #loader { display: none; margin-top: 20px; color: #58a6ff; font-weight: bold; }
  </style>
</head>
<body>
  <div class="container">
    <h2>📡 Network Analyzer</h2>
    <p>Scanning 2.4GHz Spectrum</p>
    
    <button id="scanBtn" class="btn" onclick="startScan()">RUN RADAR SWEEP</button>
    <div id="loader">Scanning airwaves... (Takes ~3 seconds)</div>
    
    <table>
      <thead>
        <tr>
          <th>Network (SSID)</th>
          <th>CH</th>
          <th>Signal (RSSI)</th>
        </tr>
      </thead>
      <tbody id="results">
        <tr><td colspan="3" style="text-align: center;">Press button to start sweep</td></tr>
      </tbody>
    </table>
  </div>

  <script>
    function startScan() {
      document.getElementById('scanBtn').disabled = true;
      document.getElementById('loader').style.display = 'block';
      document.getElementById('results').innerHTML = '';

      // Ask the ESP8266 to perform the scan
      fetch('/run_scan')
        .then(response => response.text())
        .then(html => {
          document.getElementById('results').innerHTML = html;
          document.getElementById('loader').style.display = 'none';
          document.getElementById('scanBtn').disabled = false;
        });
    }
  </script>
</body>
</html>
)rawliteral";

// Serve the main UI
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// This runs when the Javascript clicks the "RUN RADAR SWEEP" button
void handleScan() {
  // Scan for networks (async = false, show_hidden = true)
  int n = WiFi.scanNetworks(false, true);
  String html = "";

  if (n == 0) {
    html = "<tr><td colspan='3' style='text-align: center;'>No networks found</td></tr>";
  } else {
    for (int i = 0; i < n; ++i) {
      // Get data
      String ssidName = WiFi.SSID(i);
      if (ssidName == "") ssidName = "<i>[Hidden Network]</i>";
      int channel = WiFi.channel(i);
      int rssi = WiFi.RSSI(i);
      
      // Calculate signal strength percentage for the bar graph
      int quality = 0;
      if(rssi <= -100) quality = 0;
      else if(rssi >= -50) quality = 100;
      else quality = 2 * (rssi + 100);

      // Determine bar color based on strength
      String barClass = "";
      if (quality < 40) barClass = "weak";
      else if (quality < 70) barClass = "fair";

      // Build the HTML row for this specific network
      html += "<tr>";
      html += "<td><strong>" + ssidName + "</strong></td>";
      html += "<td>" + String(channel) + "</td>";
      html += "<td>" + String(rssi) + " dBm";
      html += "<div class='bar-bg'><div class='bar-fg " + barClass + "' style='width:" + String(quality) + "%'></div></div></td>";
      html += "</tr>";
    }
  }
  
  // Send the HTML rows back to the Javascript to update the table
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  
  // Set ESP8266 to both Access Point AND Station mode
  WiFi.mode(WIFI_AP_STA);
  
  // Start the Access Point
  WiFi.softAP(ssid, password);
  
  Serial.println("\n[SYSTEM] Scanner AP Started");
  Serial.print("[SYSTEM] IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Setup server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/run_scan", HTTP_GET, handleScan);

  server.begin();
}

void loop() {
  server.handleClient();
}