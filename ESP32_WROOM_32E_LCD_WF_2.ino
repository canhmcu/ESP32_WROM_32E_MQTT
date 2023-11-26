//ESP32_WROOM_32E_LCD_WF
//DESIGNER: V3_MTA
#include <EEPROM.h>
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>// mqtt
#include <WiFiClient.h>// mqtt
#include <ezButton.h>// debounce 
#include <U8g2lib.h> // lcd12864
#include <SPI.h>// de truyen du lieu cho lcd

#define FLASH_MEMORY_SIZE 512
#define EEPROM_SSID_ADDR 0
#define EEPROM_PASSWORD_ADDR 64
#define EEPROM_SERVERORIP_ADDR 128
#define EEPROM_PORT_ADDR 192
#define EEPROM_ID_ADDR 256
#define EEPROM_USER_ADDR 320
#define EEPROM_SERVER_PASS_ADDR 384
#define DEBOUNCE_TIME  50

// decorate sth
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 25  , /* data=*/ 23, /* CS=*/ 22, /* reset=*/26); // LCD set pin
ezButton button(0); // BUTTON PIN
uint8_t statussy = 0; // statu system
uint8_t LED1pin = 2;
uint8_t statusreceive = 0;
bool    LED1status = LOW;
const int maxMessages = 1;  // Số lượng tin nhắn tối đa bạn muốn lưu
char messages[maxMessages][128];  // Mảng 2D để lưu tin nhắn, mỗi tin nhắn có độ dài tối đa 128 ký tự
int messageCount = 0;  // Số lượng tin nhắn hiện tại trong mảng
bool buttonPressed = false;
unsigned long buttonPressStartTime = 0;
const int buttonHoldDuration = 5000;
// start web
WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
// setup
void setup()
{
  //Start
  Serial.begin(115200);
  EEPROM.begin(FLASH_MEMORY_SIZE);
  //pin mode
  pinMode(LED1pin, OUTPUT);
  // Chờ 5 giây
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    u8g2.begin();
    u8g2.setContrast(50);
    LCDhello();
    button.setDebounceTime(200);
    button.loop();
    Serial.println("WATTING 5S");
    // Kiểm tra nút trong thời gian chờ
    if (button.isPressed()) {
      // Nút được nhấn trong 5 giây, thực hiện xóa dữ liệu EEPROM và chuyển đến hàm firstconnect
      Serial.println("The button is pressed");
      cleEEP(0, 512);
      //accesspoint();
      break;
    }
    //buttonhandle
    button.setDebounceTime(DEBOUNCE_TIME);
  }
  Serial.println("WATTING 5S DONE");
  // lcd setup
  u8g2.begin();
  u8g2.setContrast(50);
  // call first connect
  firstconnect();
}
//loop
void loop()
{
  // button loop
  button.loop();
  // web control
  server.handleClient();
  //  // led control
  //  if (LED1status) digitalWrite(LED1pin, HIGH);
  //  else digitalWrite(LED1pin, LOW);
  // button control
  if (button.isPressed())
  {
    Serial.println("The button is pressed");
  }
  if (button.isReleased())
  {
    Serial.println("The button is released");
    client.publish("outTopic", "Received");
    statusreceive = 0;
  }
  // led receive
  if (statusreceive == 1)
  {
    digitalWrite(LED1pin, HIGH);
    delay(100);
    digitalWrite(LED1pin, LOW);
    delay(100);
  }
  else
  {
    digitalWrite(LED1pin, LOW);
  }
  // LCD control
  switch (statussy) {
    case 0:
      LCDnoWF();
      break;

    case 1:
      LCDconWF();
      break;

    case 2:
      LCDmqttReceive();
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("WiFi connection lost, switching to Station Mode...");
        stationmode();
      }
      break;
      break;

    case 3:
      LCDerrorWF();
      break;
  }
  // MQTT loop
  client.loop();
}
//First connect
void firstconnect()
{
  String ssid = readEEPROMString(EEPROM_SSID_ADDR);
  String password = readEEPROMString(EEPROM_PASSWORD_ADDR);
  Serial.println(ssid);
  Serial.println(password);
  if (isValidWiFi(ssid, password))
  {
    stationmode();
  }
  else
  {
    accesspoint();
  }
}
//StationMode
void stationmode()
{
  String STssid = readEEPROMString(EEPROM_SSID_ADDR);
  String STpassword = readEEPROMString(EEPROM_PASSWORD_ADDR);
  String STServerOrIP = readEEPROMString(EEPROM_SERVERORIP_ADDR);
  String STPort = readEEPROMString(EEPROM_PORT_ADDR);
  String STID = readEEPROMString(EEPROM_ID_ADDR);
  String STUser = readEEPROMString(EEPROM_USER_ADDR);
  String STServerPass = readEEPROMString(EEPROM_SERVER_PASS_ADDR);

  Serial.println(STssid);
  Serial.println(STpassword);
  Serial.println(STServerOrIP);
  Serial.println(STPort);
  Serial.println(STID);
  Serial.println(STUser);
  Serial.println(STServerPass);

  WiFi.begin(STssid.c_str(), STpassword.c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    LCDconWF();
    delay(1000);
    Serial.println("samodeConnecting to WiFi...");
  }
  // Set up MQTT client
  client.setServer(STServerOrIP.c_str(), STPort.toInt());
  client.setCallback(callback);
  reconnectMqtt();
}
//Accesspoint
void accesspoint()
{
  statussy = 0;
  const char* APssid = "CATT_V3";  // Tên mạng Wi-Fi Access Point
  const char* APpassword = "88888888";  // Mật khẩu cho AP
  String username = "John";
  // Configure IP addresses of the local access point
  IPAddress local_IP(192, 168, 1, 22);
  IPAddress gateway(192, 168, 1, 5);
  IPAddress subnet(255, 255, 255, 0);

  // Bắt đầu trong chế độ Access Point
  Serial.print("Setting up Access Point ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

  Serial.print("Starting Access Point ... ");
  Serial.println(WiFi.softAP(APssid, APpassword) ? "Ready" : "Failed!");

  Serial.print("IP address = ");
  Serial.println(WiFi.softAPIP());

  delay(100);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/connect", HTTP_POST, handleConnect);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}
// handle for web
void handleRoot()
{
  String ssid = "";
  String password = "";
  String ID = "";
  String ServerOrIP = "";
  String Port = "";
  String User = "";
  String ServerPass = "";
  // Trang web cho phép nhập tên và mật khẩu Wi-Fi
  String html = "<!DOCTYPE html><html><head><style>body{text-align:center;font-family:Arial,sans-serif;}form{display:block;text-align:left;padding:10px;border:1px solid #ccc;border-radius:10px;margin:10px;}input[type='text'],input[type='password']{width:100%;padding:10px;margin:5px 0;}button{display:block;width:100%;padding:10px;background-color:#0074c0;color:white;border:none;border-radius:10px;cursor:pointer;}</style></head><body><h1>ESP32_CATT_V3</h1><form method='post' action='/connect'><p><b>WiFi_NAME:</b> <input type='text' name='ssid'></p><p><b>PASSWORD:</b> <input type='password' name='password'></p><p><b>IP server:</b> <input type='text' name='ip'></p><p><b>Port:</b> <input type='text' name='port'></p><p><b>ID:</b> <input type='text' name='id'></p><p><b>User:</b> <input type='text' name='user'></p><p><b>Password server:</b> <input type='password' name='serverpass'></p><button type='submit'>Submit</button></form></body></html>";
  server.send(200, "text/html", html);
}
void handleConnect()
{
  String newSsid = server.arg("ssid");
  String newPassword = server.arg("password");
  String newIP = server.arg("ip");
  String newPort = server.arg("port");
  String newID = server.arg("id");
  String newUser = server.arg("user");
  String newServerPass = server.arg("serverpass");

  if (isValidWiFi(newSsid, newPassword))
  {
    // Save in EEPROM
    writeEEPROMString(EEPROM_SSID_ADDR, newSsid);
    writeEEPROMString(EEPROM_PASSWORD_ADDR, newPassword);
    writeEEPROMString(EEPROM_SERVERORIP_ADDR, newIP);
    writeEEPROMString(EEPROM_PORT_ADDR, newPort);
    writeEEPROMString(EEPROM_ID_ADDR, newID);
    writeEEPROMString(EEPROM_USER_ADDR, newUser);
    writeEEPROMString(EEPROM_SERVER_PASS_ADDR, newServerPass);

    // Connectwf, if use stationmode u will see error

    String ssid = readEEPROMString(EEPROM_SSID_ADDR);
    String password = readEEPROMString(EEPROM_PASSWORD_ADDR);
    String ServerOrIP = readEEPROMString(EEPROM_SERVERORIP_ADDR);
    String Port = readEEPROMString(EEPROM_PORT_ADDR);
    String ID = readEEPROMString(EEPROM_ID_ADDR);
    String User = readEEPROMString(EEPROM_USER_ADDR);
    String ServerPass = readEEPROMString(EEPROM_SERVER_PASS_ADDR);

    Serial.println(ssid);
    Serial.println(password);
    Serial.println(ServerOrIP);
    Serial.println(Port);
    Serial.println(ID);
    Serial.println(User);
    Serial.println(ServerPass);

    WiFi.begin(ssid.c_str(), password.c_str());
    while (WiFi.status() != WL_CONNECTED)
    {
      LCDconWF();
      delay(1000);
      Serial.println("Connecting to WiFi...");
    }
    // Answer
    String successHtml = "<html><body>";
    successHtml += "<h1>Connect success!</h1>";
    successHtml += "</body></html>";
    server.send(200, "text/html", successHtml);
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    // Set up MQTT client
    client.setServer(ServerOrIP.c_str(), Port.toInt());
    client.setCallback(callback);
    reconnectMqtt();
  }
  else
  {
    server.send(200, "text/plain", "Invalid Wi-Fi credentials");
    statussy = 3;
  }
}
// write eeprom
void writeEEPROMString(int addr, String data)
{
  for (int i = 0; i < data.length(); i++)
  {
    EEPROM.write(addr + i, data[i]);
  }
  // Đảm bảo kết thúc chuỗi trong EEPROM
  EEPROM.write(addr + data.length(), 0);
  EEPROM.commit(); // Lưu thay đổi vào EEPROM
}
//readEEPROM
String readEEPROMString(int addr)
{
  String result = "";
  for (int i = 0; i < 64; i++) {
    char c = EEPROM.read(addr + i);
    if (c == 0) {
      // Ký tự null (0) được sử dụng để đánh dấu kết thúc của chuỗi trong EEPROM
      break;
    }
    result += c;
  }
  return result;
}

// delete EEPROM
void cleEEP(int startAddr, int endAddr)
{
  for (int i = startAddr; i < endAddr; i++)
  {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

//not found
void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
  Serial.println("Not found");
}
//wf check
bool isValidWiFi(const String & ssid, const String & password)
{
  // Đây là nơi bạn có thể thêm các kiểm tra để xác minh tính hợp lệ của tên và mật khẩu Wi-Fi
  // Ví dụ: kiểm tra độ dài, ký tự đặc biệt, hoặc quy tắc riêng
  return (ssid.length() > 0 && password.length() >= 8);
}
//LCD print
void LCDnoWF()
{
  //  String ipAddress = WiFi.localIP().toString();
  //  char bufferip[50];
  //  snprintf(bufferip, sizeof(bufferip), "IP address: %s", ipAddress.c_str());
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); // Đặt font
  u8g2.drawStr(0, 10, "Please connect Wifi!"); // Vẽ văn bản
  u8g2.drawStr(0, 20, "WF:CATT,PW: 88888888");
  u8g2.drawStr(0, 30, "Web: 192.168.1.22");
  u8g2.drawStr(25, 60, "Designer:V3_MTA");
  u8g2.sendBuffer();
}
void LCDconWF()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); // Đặt font
  u8g2.drawStr(0, 10, "Connecting to WiFi..."); // Vẽ văn bản
  u8g2.drawStr(25, 60, "Designer:V3_MTA");
  u8g2.sendBuffer();
}
void LCDerrorWF()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); // Đặt font
  u8g2.drawStr(0, 10, "Invalid Wi-Fi credentials"); // Vẽ văn bản
  u8g2.drawStr(25, 60, "Designer:V3_MTA");
  u8g2.sendBuffer();
}
void LCDconMQTT()
{
  String ServerOrIP = readEEPROMString(EEPROM_SERVERORIP_ADDR);
  String Port = readEEPROMString(EEPROM_PORT_ADDR);
  String ID = readEEPROMString(EEPROM_ID_ADDR);
  String User = readEEPROMString(EEPROM_USER_ADDR);
  String ServerPass = readEEPROMString(EEPROM_SERVER_PASS_ADDR);
  char cServerOrIP[50];
  snprintf(cServerOrIP, sizeof(cServerOrIP), "IP: %s", ServerOrIP.c_str());
  char cPort[50];
  snprintf(cPort, sizeof(cPort), "Port: %s", Port.c_str());
  char cID[50];
  snprintf(cID, sizeof(cID), "ID: %s", ID.c_str());
  char cUser[50];
  snprintf(cUser, sizeof(cUser), "User: %s", User.c_str());
  char cServerPass[50];
  snprintf(cServerPass, sizeof(cServerPass), "ServerPass: %s", ServerPass.c_str());

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); // Đặt font
  u8g2.drawStr(0, 10, cServerOrIP);
  u8g2.drawStr(0, 20, cPort);
  u8g2.drawStr(0, 30, cID);
  u8g2.drawStr(0, 40, cUser);
  u8g2.drawStr(0, 50, cServerPass);
  u8g2.drawStr(25, 60, "Designer:V3_MTA");
  u8g2.sendBuffer();
}
void LCDfailedMQTT()
{
  int clientState = client.state();
  String clientStateStr = String(clientState);
  char bufferer[50];
  snprintf(bufferer, sizeof(bufferer), "rc=: %s", clientStateStr.c_str());
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); // Đặt font
  u8g2.drawStr(0, 10, "Connect Wifi Success"); // Vẽ văn bản
  u8g2.drawStr(0, 20, "MQTT connection...failed");
  u8g2.drawStr(0, 30, bufferer);
  u8g2.drawStr(0, 40, "SYS is trying again!");
  u8g2.drawStr(25, 60, "Designer:V3_MTA");
  u8g2.sendBuffer();
}
void LCDmqttReceive()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); // Đặt font
  u8g2.drawStr(0, 10, "V3_MTA");
  u8g2.drawStr(0, 20, "MQTT_Receive:");
  u8g2.drawStr(0, 30, messages[0]);// Vẽ văn bản
  if (statusreceive == 1)
  {
    u8g2.drawStr(0, 40, "Messenger_Arrive");
  }
  else
  {
    u8g2.drawStr(0, 40, "Device_Received");
  }
  u8g2.drawStr(25, 60, "Designer:V3_MTA");
  u8g2.sendBuffer();
}
void LCDhello()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); // Đặt font
  u8g2.drawStr(15, 20, "Welcome!!!");
  u8g2.drawStr(25, 60, "Designer:V3_MTA");
  u8g2.sendBuffer();
}
// Set up MQTT callback
void callback(char* topic, byte * payload, unsigned int length)
{
  Serial.print("Received message on topic: ");
  Serial.println(topic);
  Serial.print("Noi dung ");
  // Chuyển dữ liệu từ payload (byte array) thành một chuỗi (string)
  String message = "";
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
    Serial.print((char)payload[i]);
  }
  Serial.println();
  // Lưu tin nhắn vào mảng
  if (messageCount < maxMessages)
  {
    strcpy(messages[messageCount], message.c_str());
    messageCount++;
  }
  else
  {
    // Nếu mảng đầy, bạn có thể thực hiện các xử lý khác, ví dụ: ghi đè tin nhắn cũ nhất
    strcpy(messages[0], message.c_str());
  }
  statusreceive = 1;
}
// setup con MQTT
void reconnectMqtt() {
  // Loop until we're reconnected
  while (!client.connected())
  {
    //  { button.loop();
    //    int    error = 0;
    //    if (button.isPressed())
    //    {
    //      Serial.println("The button is pressed");
    //    }
    //    if (button.isReleased())
    //    {
    //      Serial.println("The button is released");
    //      error = 1;
    //    }
    //    if (error == 0)
    //    {
    //      LCDfailedMQTT();
    //    }
    //    else
    //    {
    //      LCDconMQTT();
    //    }
    LCDfailedMQTT();
    Serial.print("Attempting MQTT connection...");
    String reID = readEEPROMString(EEPROM_ID_ADDR);
    String reUser = readEEPROMString(EEPROM_USER_ADDR);
    String reServerPass = readEEPROMString(EEPROM_SERVER_PASS_ADDR);
    Serial.println(reID);
    // Attempt to connect
    if (client.connect(reID.c_str(), reUser.c_str(), reServerPass.c_str()))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
      statussy = 2;
    } else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 7 seconds");
      // Wait 5 seconds before retrying
      LCDconMQTT();
      delay(7000);
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("WiFi connection lost, switching to Station Mode...");
        stationmode();
      }
    }
  }
}
