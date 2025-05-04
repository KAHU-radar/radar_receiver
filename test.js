mod = require(".");

ri = new mod.RadarInfo(
  0,
  (angle, bearing, data, range_meters, time) => console.log("SPOKE", angle, bearing, data, range_meters, time),
  (item) => {
    console.log("CONFIG", item);
    if (item.type.id == "CT_STATE" && item.value.value == 1) {
      console.log("Start transmitting");
      ri.setProperty('CT_RANGE', 1000);
      ri.setTransmit(true);
    }
  }
);
//console.log("Radar type:", ri.getType());

//console.log("Properties:", ri.getProperties());


//console.log("Properties:", ri.getProperties());
