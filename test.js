mod = require(".");

var radarTypes = Object.fromEntries(mod.getRadarTypes().map((a, idx) => [a.type, idx]));

ri = new mod.RadarInfo(
  radarTypes.RT_HaloA,
  (angle, bearing, data, range_meters, time) => console.log("SPOKE", angle, bearing, data, range_meters, time),
  (item) => {
    console.log("CONFIG", item);
    if (item.type.id == "CT_STATE" && item.value.value == 'RADAR_STANDBY') {
      console.log("Start transmitting");
      ri.setProperty('CT_RANGE', 1000);
      ri.setTransmit(true);
    }
  }
);
//console.log("Radar type:", ri.getType());

// ri.setProperty("CT_TRAILS_MOTION", "Relative")

//console.log("CT_TRAILS_MOTION:", ri.getProperty("CT_TRAILS_MOTION"));
//console.log("CT_TRANSPARENCY:", ri.getProperty("CT_TRANSPARENCY"));

//console.log("Properties:", ri.getProperties());


//console.log("Properties:", ri.getProperties());
