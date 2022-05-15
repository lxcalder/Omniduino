/*
  LoRa Duplex communication

  Sends a message every half second, and polls continually
  for new incoming messages. Implements a one-byte addressing scheme,
  with 0xFF as the broadcast address.

  Uses readString() from Stream class to read payload. The Stream class'
  timeout may affect other functuons, like the radio's callback. For an

  created 28 April 2017
  by Tom Igoe
*/
#include <SPI.h>              // include libraries
#include <LoRa.h>
#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)  // Required for Serial port in Omniduino boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif

const int csFLASH = 38; //Chip select for External Flash memory in Omniduino M0
const int csLora = 26; //Chip select for Lora module in Omniduino M0(10, library default)
const int rstLora = 31; //Chip select for External Flash memory in Omniduino M0 (9, library default)
const int dio0Lora = 30; //Interrupt pin from RFM95 module in Omniduino M0(2, library default)

const long frequency = 915E6;  //LoRa Frequency for North America usage, need to change according to country usage

String outgoing;              // outgoing message

byte msgCount = 0;        // count of outgoing messages
byte localAddress = 0xAA; //**Device's Address, different for every device on the network or "This message is not for me" will be shown
byte destination = 0x55;  //**Address for destination device
long lastSendTime = 0;    // last send time
int interval = 2000;      // interval between sends

void setup() {
  pinMode(csFLASH, OUTPUT);//Disable External Flash memory in Omniduino M0
  digitalWrite(csFLASH, HIGH);
  LoRa.setPins(csLora, rstLora, dio0Lora); //override default CS, reset, and IRQ pins for Omniduino M0 interfacing 
  
  Serial.begin(9600);                   // initialize serial
  delay(1000); //Wait for the serial port to initialize
 //while (!Serial); //Un-Comment this line to wait for printed data in Arduino serial port, Comment for stand-alone Omniduino MO application
  Serial.println("LoRa Duplex");

  if (!LoRa.begin(915E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");
}

void loop() {
  if (millis() - lastSendTime > interval) {
    String message = "HeLoRa from Node_1!";   // send a message
    sendMessage(message);
    Serial.println("Sending " + message);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;    // 2-3 seconds
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}

void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
}
