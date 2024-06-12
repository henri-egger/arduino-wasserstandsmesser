#include <SoftwareSerial.h>

// 10 goes to TX, is arduino RX
// 11 goes to RX, is arduino TX
SoftwareSerial sim(10, 11);

// AT+HTTPCLIENT=<opt>,<content-type>,<"url">,[<"host">],[<"path">],<transport_type>[,<"data">][,<"http_req_header">][,<"http_req_header">][...]
// AT+HTTPCLIENT=GET,text/plain,http://httpbin.org/ip,,,,,HTTP_TRANSPORT_OVER_TCP

// --- Basic commands ---
// CREG? - network registration status -> want 0,1 or 0,5
// COPS? - connected operator status
// CMEE - error info -> set to 2

// --- GPRS ---
// Enable GPRS: AT+SAPBR=3,1,Contype,GPRS

// TIM APN parameter: ibox.tim.it
// Enter APN parameter: AT+SAPBR=3,1,APN,ibox.tim.it

// Open GPRS connection: AT+SAPBR=1,1
// Check GPRS status: AT+SAPBR=2,1
// Close GPRS connection: AT+SAPBR=0,1

// --- HTTP ---
/*
AT+HTTPINIT - init
AT+HTTPPARA=CID,1 - carrier profile (usually 1)
AT+HTTPPARA=URL,"URL" - url
AT+HTTPPARA=CONTENT,application/x-www-form-urlencoded - content type
AT+HTTPDATA=192,10000 - data size and max response time
Postname=VALUE - data
AT+HTTPACTION=1 - transfer method (0=GET 1=POST 2=HEAD)
AT+HTTPREAD - execute and read response
AT+HTTPTERM - terminate

AT+HTTPINIT
AT+HTTPPARA=CID,1
AT+HTTPPARA=URL,https://httpbin.org/post
AT+HTTPPARA=CONTENT,text/plain
AT+HTTPDATA=192,5000
Postname=helloworld
AT+HTTPACTION=1
AT+HTTPREAD
AT+HTTPTERM
*/

/*
AT+HTTPINIT

OK
AT+HTTPPARA=CID,1

OK
AT+HTTPPARA=URL,http://wasserstand-eisack.it/post

OK
AT+HTTPPARA="CONTENT","application/json"

OK
AT+HTTPDATA=14,5000

DOWNLOAD

{"height":14}

OK
AT+HTTPACTION=1

OK

+HTTPACTION: 1,200,24
AT+HTTPREAD

+HTTPREAD: 24
Data stored successfully
OK
AT+HTTPTERM

OK
*/

void setup() {
  sim.begin(9600);
  Serial.begin(115200);
}

void loop() {
  if (Serial.available() > 0) {
    String str = Serial.readString();
    sim.println(str);
  }

  if (sim.available() > 0) {
    Serial.write(sim.read());
  }
}
