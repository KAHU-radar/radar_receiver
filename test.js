mod = require(".");
const fs = require('fs/promises');
const path = require('path');

const outputFile = path.join(__dirname, 'radar_data.bin');

var radarTypes = Object.fromEntries(mod.getRadarTypes().map((a, idx) => [a.type, idx]));

state = {
  transmitting: false
}


const ROW_SIZE = 1024;
const TOTAL_ROWS = 2048;
const FILE_SIZE = ROW_SIZE * TOTAL_ROWS;

// Preallocate file if it doesn't exist
async function ensureFileInitialized() {
  try {
    await fs.access(outputFile);
    const stats = await fs.stat(outputFile);
    if (stats.size !== FILE_SIZE) {
      console.log("Reinitializing radar data file...");
      const zeroBuffer = Buffer.alloc(FILE_SIZE);
      await fs.writeFile(outputFile, zeroBuffer);
    }
  } catch {
    console.log("Initializing radar data file...");
    const zeroBuffer = Buffer.alloc(FILE_SIZE);
    await fs.writeFile(outputFile, zeroBuffer);
  }
}

(async () => {
  await ensureFileInitialized();

  ri = new mod.RadarInfo({
    settings: {
      Plugins: {
        Radar: {
          Radar0Type: 'Navico BR24',
          ThresholdBlue: '33',
          ThresholdGreen: '100',
          ThresholdRed: '200',
          Radar0Zone0StartBearing: '0',
          Radar0Zone0EndBearing: '360',
          Radar0Zone0OuterRange: '100000',
          Radar0Zone0InnerRange: '0',
          Radar0Zone0Type: '1',
          Radar0Zone0AlarmOn: '0',
          Radar0Zone0ArpaOn: '1',
          VerboseLog: '77'
        }
      }
    },
    spoke_cb: async (angle, bearing, data, range_meters, time) => {
     if (state.transmitting) {
  //     console.log("SPOKE", angle, bearing, data, range_meters, time);
        if (angle < 0 || angle >= TOTAL_ROWS) {
          console.warn(`Invalid angle: ${angle}`);
          return;
        }

        try {
          const filehandle = await fs.open(outputFile, 'r+');
          await filehandle.write(Buffer.from(data), 0, ROW_SIZE, angle * ROW_SIZE);
          await filehandle.close();
        } catch (err) {
          console.error("Failed to write radar data:", err);
        }
      }
    },
    property_cb: (item) => {
      if (item.type.id == "CT_STATE" && item.value.value == 'RADAR_STANDBY') {
        console.log("Start transmitting");
        ri.setProperty('CT_RANGE', 10000);
        ri.setTransmit(true);
        state.transmitting = true;
      } else if (item.type.id == "CT_STATE" && item.value.value == 'RADAR_TRANSMIT') {
        console.log("Radar is now transmitting");
        state.transmitting = true;
      }
    },
    arpa_cb: (target) => {
      console.log("ARPA", target);
    }
  });
  ri.setPosition({lat: 0.0, lon: 0.0});
  //console.log("SETTINGS", ri.getSettings());
//console.log("Radar type:", ri.getType());

// ri.setProperty("CT_TRAILS_MOTION", "Relative")

//console.log("CT_TRAILS_MOTION:", ri.getProperty("CT_TRAILS_MOTION"));
//console.log("CT_TRANSPARENCY:", ri.getProperty("CT_TRANSPARENCY"));

//console.log("Properties:", ri.getProperties());


//console.log("Properties:", Object.keys(ri.getProperties()));
})();



