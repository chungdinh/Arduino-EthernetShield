* "Smart Garden"
Copyright 2017 Chung Dinh*/
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h> 
#include <Wire.h> 
#define SD_CARD 4

/* Địa chỉ của DS1307 */
const byte DS1307 = 0x68;
/* Số byte dữ liệu sẽ đọc từ DS1307 */
const byte NumberOfFields = 7;
 
/* khai báo các biến thời gian */
int second, minute, hour, day, wday, month, year;
byte ip[4]      = { 192,168,1,9 };//Địa chỉ IP của Ethernet Shield                  
byte gateway[4] = { 192,168,0,254 };
byte subnet[4]  = { 255,255,255,0 };
byte mac[6]     = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };//Địa chỉ MAC của Ethernet Shield (có thể tự đặt theo hệ số HEX)
    
EthernetServer server(80);//Cổng kết nối HTTP

boolean LED_state[7] = {0};//Khởi tạo mảng có 7 phần tử để lưu trữ các trạng thái của LED 

char linebuf[80]; 
int charCount = 0;
int k;

void setup() {
    //Khai báo chân kết nối LED
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(7, OUTPUT);
    pinMode(8, OUTPUT);
    pinMode(9, OUTPUT);
    
    Serial.begin(9600);
Wire.begin();
  /* cài đặt thời gian cho module */
  setTime(21, 59, 57, 1, 6, 5, 17);
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);//chân nháy đèn
    //Khởi tạo thẻ SD
    Serial.println("Initializing SD card...");
    if (!SD.begin(SD_CARD)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;
    }
    Serial.println("SUCCESS - SD card initialized.");
    //Tìm kiếm file index.htm trên thẻ SD
    if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return; 
    }
    Serial.println("SUCCESS - Found index.htm file.");   
    
    Ethernet.begin(mac, ip, gateway, subnet);     
    server.begin();
    Serial.print("server is at ");
    Serial.println(Ethernet.localIP());
}


void loop() {
      
    EthernetClient client = server.available();//Kiểm tra khởi tạo kết nối client
    if (client) { 
        //memset(linebuf, 0, sizeof(linebuf));

        boolean authenticated      = false;
        boolean currentLineIsBlank = true;
        boolean logoff             = false;
        boolean xml                = false;
         
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                
                linebuf[charCount] = c;                 
                if (charCount<sizeof(linebuf)-1) { charCount++; }
                Serial.write(c);//Đọc dữ liệu từ client
                //Kiểm tra yêu cầu từ client và gởi trang web đến client
                if (c == '\n' && currentLineIsBlank) {
                    if (authenticated && !logoff ) {
                        if (xml) { html_xml(client);
                        } else { html_authenticated(client); } 
                    } else {
                        logoff ? html_logoff(client) : html_authenticator(client);
                    }
                    //SetLEDs();
                    break;
                    
                }
                
                if (c == '\n') { 
                    currentLineIsBlank = true;               
                     
                    if (strstr(linebuf, "GET /logoff"         ) > 0 ) 
                    { 
                      logoff = true; 
                    }
                    if (strstr(linebuf, "Authorization: Basic") > 0 ) 
                    {
                      if(strstr(linebuf, "YWRtaW46ZGluaHZhbmNodW5n") != NULL) //Base 64
                      //if (validate_user(linebuf)) 
                      { 
                        authenticated = true;
                      }
                    }  
                    if (strstr(linebuf, "ajax_inputs") > 0 ) 
                    { 
                      xml = true;
                      SetLEDs(); 
                    }
                     
                    memset(linebuf, 0, sizeof(linebuf));
                    charCount = 0;
                    
                } else if (c != '\r') {
                    currentLineIsBlank = false;  
                }
            }
        }
        delay(1);           
        client.stop(); 
    }
    /* Đọc dữ liệu của DS1307 */
  readDS1307();
  /* Hiển thị thời gian ra Serial monitor */
  digitalClockDisplay();
  delay(1000);
}


 void readDS1307()
{
        Wire.beginTransmission(DS1307);
        Wire.write((byte)0x00);
        Wire.endTransmission();
        Wire.requestFrom(DS1307, NumberOfFields);
        
        second = bcd2dec(Wire.read() & 0x7f);
        minute = bcd2dec(Wire.read() );
        hour   = bcd2dec(Wire.read() & 0x3f); // chế độ 24h.
        wday   = bcd2dec(Wire.read() );
        day    = bcd2dec(Wire.read() );
        month  = bcd2dec(Wire.read() );
        year   = bcd2dec(Wire.read() );
        year += 2000;    
}
/* Chuyển từ format BCD (Binary-Coded Decimal) sang Decimal */
int bcd2dec(byte num)
{
        return ((num/16 * 10) + (num % 16));
}
/* Chuyển từ Decimal sang BCD */
int dec2bcd(byte num)
{
        return ((num/10 * 16) + (num % 10));
}
 
void digitalClockDisplay(){
    // digital clock display of the time
    Serial.print(hour);
    printDigits(minute);
    printDigits(second);
    Serial.print(" ");
    Serial.print(day);
    Serial.print(" ");
    Serial.print(month);
    Serial.print(" ");
    Serial.print(year); 
    Serial.println(); 
}
 
void printDigits(int digits){
    // các thành phần thời gian được ngăn cách bằng dấu :
    Serial.print(":");
        
    if(digits < 10)
        Serial.print('0');
    Serial.print(digits);
}
 
/* cài đặt thời gian cho DS1307 */
void setTime(byte hr, byte min, byte sec, byte wd, byte d, byte mth, byte yr)
{
        Wire.beginTransmission(DS1307);
        Wire.write(byte(0x00)); // đặt lại pointer
        Wire.write(dec2bcd(sec));
        Wire.write(dec2bcd(min));
        Wire.write(dec2bcd(hr));
        Wire.write(dec2bcd(wd)); // day of week: Sunday = 1, Saturday = 7
        Wire.write(dec2bcd(d)); 
        Wire.write(dec2bcd(mth));
        Wire.write(dec2bcd(yr));
        Wire.endTransmission();
}
void html_logoff(EthernetClient &client){
    client.print(F(
                 "HTTP/1.1 401 Authorization Required\n"  
                 "Content-Type: text/html\n"  
                 "Connnection: close\n\n"  
                 "<!DOCTYPE HTML>\n"  
                 "<html><head><title>Logoff</title>\n"  
                 "<script>document.execCommand('ClearAuthenticationCache', 'false');</script>"
                 "<script>try {"                                                              
                 "   var agt=navigator.userAgent.toLowerCase();"
                 "   if (agt.indexOf(\"msie\") != -1) { document.execCommand(\"ClearAuthenticationCache\"); }"
                 "   else {"
                 "     var xmlhttp = createXMLObject();"
                 "      xmlhttp.open(\"GET\",\"URL\",true,\"logout\",\"logout\");"
                 "     xmlhttp.send(\"\");"
                 "     xmlhttp.abort();"
                 "   }"
                 " } catch(e) {"
                 "   alert(\"erro ao fazer logoff\");"
                 " }"
                 "function createXMLObject() {"
                 "  try {if (window.XMLHttpRequest) {xmlhttp = new XMLHttpRequest();} else if (window.ActiveXObject) {xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");}} catch (e) {xmlhttp=false;}"
                 "  return xmlhttp;"
                 "}</script>"
                 "</head><body><h1>401 Authentication</h1><hr /><h4>You had exit the application, reload the page and sign in to continue the program.</h4></body></html>\n"));
}
 
void html_authenticator(EthernetClient &client) {
    client.print(F("HTTP/1.1 401 Authorization Required\n"  
                 "WWW-Authenticate: Basic realm=\"Area Restrita\"\n"
                 "Content-Type: text/html\n"  
                 "Connnection: close\n\n"  
                 "<!DOCTYPE HTML>\n"  
                 "<html><head><title>Error</title>\n"
                 "</head><body><h1>401 Unauthorized Access</h1><hr /><h4>Enter a valid username and password to be verified</h4></body></html>\n"));
}

void html_xml(EthernetClient &client) {
    client.println(F("HTTP/1.1 200 OK\n"
                     "Content-Type: text/xml\n"
                     "Connection: keep-alive\n"));
    XML_response(client);
}

void html_authenticated(EthernetClient &client){
    client.println(F("HTTP/1.1 200 OK\n"
                     "Content-Type: text/html\n"
                     "Connection: keep-alive\n"));
   
    File webFile = SD.open("index.htm");
    if (webFile) {
        while(webFile.available()) {
            client.write(webFile.read()); 
        }
        webFile.close();
    }
} 

void SetLEDs()
{
    // LED 1 (pin 2)
    if ((strstr(linebuf, "LED1=1"))||(hour==22&&minute==0)) {
        LED_state[0] = 1;
        digitalWrite(2, HIGH);
    } else if ((strstr(linebuf, "LED1=0"))||(hour==22&&minute==1)) {
        LED_state[0] = 0;
        digitalWrite(2, LOW);
    }
    
    // LED 2 (pin 3)
    if (strstr(linebuf, "LED2=1")) {
        LED_state[1] = 1;
        digitalWrite(3, HIGH);
    } else if (strstr(linebuf, "LED2=0")) {
        LED_state[1] = 0;
        digitalWrite(3, LOW);
    }
    
    // LED 3 (pin 5)
    if (strstr(linebuf, "LED3=1")) {
        LED_state[2] = 1;
        digitalWrite(5, HIGH);
    } else if (strstr(linebuf, "LED3=0")) {
        LED_state[2] = 0;
        digitalWrite(5, LOW);
    }
    
    // LED 4 (pin 6)
    if (strstr(linebuf, "LED4=1")) {
        LED_state[3] = 1;
        digitalWrite(6, HIGH);
    } else if (strstr(linebuf, "LED4=0")) {
        LED_state[3] = 0;
        digitalWrite(6, LOW);
    }
    
    // LED 5 (pin 7)
    if (strstr(linebuf, "LED5=1")) {
        LED_state[4] = 1;
        digitalWrite(7, HIGH);
    } else if (strstr(linebuf, "LED5=0")) {
        LED_state[4] = 0;
        digitalWrite(7, LOW);
    }
    
    // LED 6 (pin 8)
    if (strstr(linebuf, "LED6=1")) {
        LED_state[5] = 1;
        digitalWrite(8, HIGH);
    } else if (strstr(linebuf, "LED6=0")) {
        LED_state[5] = 0;
        digitalWrite(8, LOW);
    }
    
    // LED 7 (pin 9)
    if (strstr(linebuf, "LED7=1")) {
        LED_state[6] = 1;
        digitalWrite(9, HIGH);
    } else if (strstr(linebuf, "LED7=0")) {
        LED_state[6] = 0;
        digitalWrite(9, LOW);
    }
}

//Gởi file XML chứa thông tin trạng thái của LED và đầu vào analog
void XML_response(EthernetClient &client)
{
    int analog_val;            //Lưu trữ giá trị của đầu vào analog
    int count;                 //Sử dụng trong vòng lặp for
    
    client.print(F("<?xml version = \"1.0\" ?>"));
    client.print(F("<inputs>"));
    
    //Đọc đầu vào analog
    for (count = 2; count <= 5; count++)        // A2 to A5
    { 
        analog_val = analogRead(count);
        k=map(analog_val,0,1023,100,0);
        client.print(F("<analog>"));
        client.print(k);
        client.println(F("</analog>"));
    }
 
    //Đọc trang thái của LED
    // LED1
    client.print(F("<LED>"));
    if (LED_state[0]) { client.print(F("on"));
    } else { client.print(F("off")); }
    client.println(F("</LED>"));

    // LED2
    client.print(F("<LED>"));
    if (LED_state[1]) { client.print(F("on"));
    } else { client.print(F("off")); }
    client.println(F("</LED>"));
    
    // LED3
    client.print(F("<LED>"));
    if ((LED_state[2])||(k<=5)) { client.print(F("on"));
    } else { client.print(F("off")); }
    client.println(F("</LED>"));
    
    // LED4
    client.print(F("<LED>"));
    if (LED_state[3]) { client.print(F("on"));
    } else { client.print(F("off")); }
    client.println(F("</LED>"));

    // LED5
    client.print(F("<LED>"));
    if (LED_state[4]) { client.print(F("on"));
    } else { client.print(F("off")); }
    client.println(F("</LED>"));

    // LED6
    client.print(F("<LED>"));
    if (LED_state[5]) { client.print(F("on"));
    } else { client.print(F("off")); }
    client.println(F("</LED>"));

    // LED7
    client.print(F("<LED>"));
    if (LED_state[6]) { client.print(F("on"));
    } else { client.print(F("off")); }
    client.println(F("</LED>"));
    
    client.print(F("</inputs>"));
}

