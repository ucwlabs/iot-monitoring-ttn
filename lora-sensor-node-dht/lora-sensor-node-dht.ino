/* 
 * Environment monitoring with The Things Network, InfluxDB, and Grafana
 * LoRa Sensor Node - DHT
 * Copyright 2019 HackTheBase - UCW Labs Ltd. All rights reserved.
 *
 * The example was created for the HackTheBase IoT Hub Lab.
 *
 * The HackTheBase IoT Hub Lab is a creative space dedicated to prototyping and inventing, 
 * all sorts of microcontrollers (Adafruit Feather, Particle Boron, ESP8266, ESP32 and more) 
 * with various types of connectivity (WiFi, LoRaWAN, GSM, LTE-M), and tools such as screwdriver,
 * voltmeter, wirings, as well as an excellent program filled with meetups, workshops, 
 * and hackathons to the community members. 
 *
 * https://hackthebase.com/iot-hub-lab
 * 
 * The HackTheBase IoT Hub will give you access to the infrastructure dedicated to your project,
 * and you will get access to our Slack community.
 *
 * https://hackthebase.com/iot-hub
 *
 * As a member, you will get access to the hardware and software resources 
 * that can help you to work on your IoT project.
 *
 * https://hackthebase.com/register
 *
 */

#include <lmic.h>
#include <hal/hal.h>
#include "SPI.h"
#include "DHT.h"

#define DHTPIN  9
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// LoRaWAN NwkSKey - Network session key
static const PROGMEM u1_t NWKSKEY[16] = { 0x51, 0x9E, 0x03, 0x6A, 0x4D, 0xA9, 0x8E, 0xBB, 0x9A, 0x5C, 0x20, 0xEA, 0x62, 0x2C, 0xEF, 0xEC };

// LoRaWAN AppSKey - Application session key
static const u1_t PROGMEM APPSKEY[16] = { 0x51, 0x9E, 0x03, 0x6A, 0x4D, 0xA9, 0x8E, 0xBB, 0x9A, 0x5C, 0x20, 0xEA, 0x62, 0x2C, 0xEF, 0xEC };

// LoRaWAN end-device address (DevAddr)
static const u4_t DEVADDR = 0x260217CE; // <-- Change this address for every node!

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty cycle limitations).
const unsigned TX_INTERVAL = 16;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 8,  
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 4,
  .dio = {3, 6, 11},
};

void onEvent(ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  
  switch(ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
      
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
      
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
      
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
      
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
      
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      break;
      
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
      
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
      
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
      
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      
      if (LMIC.txrxFlags & TXRX_ACK) Serial.println(F("Received ack"));
      
      if (LMIC.dataLen) {
        Serial.println(F("Received "));
        Serial.println(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));

        for (int i = 0; i < LMIC.dataLen; i++) {
          if (LMIC.frame[LMIC.dataBeg + i] < 0x10) {
            Serial.print(F("0"));
          }
          //store received data
          Serial.println(LMIC.frame[LMIC.dataBeg + i], HEX);
        }
      }

      do_sleep();
      do_send(&sendjob);
      
      // Schedule next transmission
      //os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
      break;
    
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;

    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;

    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;

    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    byte data[6];

    Serial.println();
    Serial.println("Reading data from sensors [BEGIN]");

    readDHT(data);

    Serial.println("Reading data from sensors [END]");
    Serial.println();

    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, data, sizeof(data), 0);
    Serial.println(F("Packet queued"));
  }

  // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
  setupSerialPorts();

  Serial.println("LoRa Sensor Node - DHT");
  Serial.println();

  // Initialise DHT sensor
  dht.begin();

  #ifdef VCC_ENABLE
  // For Pinoccio Scout boards
  pinMode(VCC_ENABLE, OUTPUT);
  digitalWrite(VCC_ENABLE, HIGH);
  delay(1000);
  #endif

  // LMIC init
  os_init();
  
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

  // Set static session parameters. Instead of dynamically establishing a session
  // by joining the network, precomputed session parameters are be provided.

  #ifdef PROGMEM
  // On AVR, these values are stored in flash and only copied to RAM
  // once. Copy them to a temporary buffer here, LMIC_setSession will
  // copy them into a buffer of its own again.
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
  #else
  // If not running an AVR with PROGMEM, just use the arrays directly
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
  #endif

  #if defined(CFG_eu868)
  // Set up the channels used by the Things Network, which corresponds
  // to the defaults of most gateways. Without this, only three base
  // channels from the LoRaWAN specification are used, which certainly
  // works, so it is good for debugging, but can overload those
  // frequencies, so be sure to configure the full frequency range of
  // your network here (unless your network autoconfigures them).
  // Setting up channels should happen after LMIC_setSession, as that
  // configures the minimal channel set.
  // NA-US channels 0-71 are configured automatically
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
  // TTN defines an additional channel at 869.525Mhz using SF9 for class B
  // devices' ping slots. LMIC does not have an easy way to define set this
  // frequency and support for class B is spotty and untested, so this
  // frequency is not configured here.
  
  // Comment out this line for single-channel gateway
  //for (int i = 1; i <= 8; i++) LMIC_disableChannel(i);
  
  #elif defined(CFG_us915)
  // NA-US channels 0-71 are configured automatically
  // but only one group of 8 should (a subband) should be active
  // TTN recommends the second sub band, 1 in a zero based count.
  // https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json
  LMIC_selectSubBand(1);  
  // Comment out this line for single-channel gateway
  // for (int i = 1; i <= 71; i++) LMIC_disableChannel(i);
  #endif

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7, 14);

  // Start job
  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}

void setupSerialPorts() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  
  //while (!Serial) {
  //  ; // wait for serial port to connect. Needed for native USB port only
  //}
}

void do_sleep() {
  for (int i = 0; i < (TX_INTERVAL / 8); i++) {
    delay(8000);
  }
}

void readDHT(byte data[]) {
  float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float temperature = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float temperatureInFahrenheit = dht.readTemperature(true);

  int16_t t = temperature * 100;
  int16_t h = humidity * 100;
  int16_t f = temperatureInFahrenheit * 100;

  data[0] = highByte(t);
  data[1] = lowByte(t);
  data[2] = highByte(f);
  data[3] = lowByte(f);
  data[4] = highByte(h);
  data[5] = lowByte(h);

  Serial.print("DHT - ");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" oC");
  Serial.print("\t");
  Serial.print("Temperature: ");
  Serial.print(temperatureInFahrenheit);
  Serial.print(" oF");
  Serial.print("\t");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");  
}
