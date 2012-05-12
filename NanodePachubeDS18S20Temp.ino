// Simple demo for feeding some random data to Pachube.
// 2011-07-08 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
//Modified by Jon 25-Feb-12 to add DS18S20+ temperature update to My Feed

#include <EtherCard.h>
#include "NanodeMAC.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/pgmspace.h>

// change these settings to match your own setup
#define FEED    "Yourkey"
#define APIKEY  "yjR4WMzFzWGWu4bf_tFiioiua89UiKxuUDN1YkltcDlpaz0g"
#define SERIAL_BAUD_RATE 115200
#define RED_LED 6

// ethernet interface mac address, must be unique on the LAN
static uint8_t mymac[6] = { 0,0,0,0,0,0 };
NanodeMAC mac( mymac );        //Use Nanodes MAC address chip

char website[] PROGMEM = "api.pachube.com";

byte Ethernet::buffer[700];
unsigned long timer;
Stash stash;

//Temperature stuff
// Data wire from DS18S20 Temperature chip is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 5

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

void setup () {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println("\n[webClient details]");
  
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  

  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");
    
  ether.printIp("SRV: ", ether.hisip);
  
  //Now setup the temperature
  // locate Temperature devices on the (one-wire) bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");   
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 9);
 
  // show the addresses we found on the bus
  Serial.print("Device 0 Resolution: ");
  Serial.println(sensors.getResolution(insideThermometer), DEC); 

}

void loop () {
    
  ether.packetLoop(ether.packetReceive());

  if (millis() > timer) {
     //Use the brutal method from PVsolar to see if the ethernet is still working.  Reboot if not.
     //Doesn't work - so took out!
//     if (ether.dhcpExpired() && !ether.dhcpSetup())
//    {
//       Serial.println(F("\r\n*Err*"));
//       int loop=150;
//       while (loop>0) {
//            digitalWrite( RED_LED, HIGH);
//            delay(50);
//            digitalWrite( RED_LED, LOW);
//            delay(150);
//           loop--;
//        } 
//        asm volatile ("  jmp 0");
//    }

	if(Stash::freeCount()<5){
		Stash::initMap(56);
                Serial.println("Stash reset");
	}

    //Read temperature first
    //Read temp now, so we get a delay for the query 
    Serial.println(F("Requesting temperatures..."));
//    sensors.requestTemperatures(); // Send the command to get temperatures
    Serial.print(F("Timer:"));
    Serial.print(timer);
    Serial.print(F("  Millis:"));
    Serial.println( millis());
//    delay(700);

    
    // Payload is ID, data pairs....
    // we can determine the size of the generated message ahead of time
    byte sd = stash.create();
    stash.print("temperature,");
    stash.println(sensors.getTempC(insideThermometer));
    //stash.print("1,");
    //stash.println((word) micros() / 456);
    stash.save();
    
    // generate the header with payload - note that the stash size is used,
    // and that a "stash descriptor" is passed in as argument using "$H"
    Stash::prepare(PSTR("PUT http://$F/v2/feeds/$F.csv HTTP/1.0" "\r\n"
                        "Host: $F" "\r\n"
                        "X-PachubeApiKey: $F" "\r\n"
                        "Content-Length: $D" "\r\n"
                        "\r\n"
                        "$H"),
            website, PSTR(FEED), website, PSTR(APIKEY), stash.size(), sd);

    // send the packet - this also releases all stash buffers once done
    ether.tcpSend();
    timer= millis() + 9300;
//    Serial.print(F("Stash:freeCount :"));
//    Serial.println(Stash::freeCount());
    sensors.requestTemperatures(); // Send the command to get temperatures

  }
}

 // function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

