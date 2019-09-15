/*
 * Environment monitoring with The Things Network, InfluxDB, and Grafana
 * TTN Bridge
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

import { data } from "ttn";
const Influx = require("influx");

const appId = "htb-iot-monitoring-dht";
const accessKey = "your_access_key";

const influx = new Influx.InfluxDB({
  host: 'influxdb',
  port: 8086,
  database: 'iot-monitoring',
  username: 'root',
  password: 'root'
});

influx.getDatabaseNames()
  .then(names => {
    if (!names.includes('iot-monitoring')) {
      return influx.createDatabase('iot-monitoring');
    }
  });

let writeDataToInflux = (data) => {
  influx
      .writePoints(
          [
              {
                measurement: 'temperature',
                tags: {
                  deviceId: data.deviceId
                },
                fields: {
                  value: data.payload.dht.temperature
                }
              },
              {
                measurement: 'humidity',
                tags: {
                  deviceId: data.deviceId
                },
                fields: {
                  value: data.payload.dht.humidity
                }
              }
           ]
      )
      .catch(error => {
        console.error("Error writing data to Influx - " + error);
      });
};

const main = async function () {
  console.log("TTN Bridge started.");

  const ttnClient = await data(appId, accessKey, "eu.thethings.network:1883");

  ttnClient.on("uplink", function (devID, payload) {
    let data = {
      deviceId: payload.dev_id,
      payload: payload.payload_fields
    };
    console.log(data);
    writeDataToInflux(data);
  })
};

main().catch(function (error) {
  console.error("Error", error);
  process.exit(1);
});