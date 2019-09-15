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