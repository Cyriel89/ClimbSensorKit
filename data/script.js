// Create events for the sensor readings
if (!!window.EventSource) {
    var source = new EventSource('/events');
  
    source.addEventListener('open', function(e) {
      console.log("Events Connected");
    }, false);
  
    source.addEventListener('error', function(e) {
      if (e.target.readyState != EventSource.OPEN) {
        console.log("Events Disconnected");
      }
    }, false);
  
    source.addEventListener('gyro_readings', function(e) {
      //console.log("gyro_readings", e.data);
      var obj = JSON.parse(e.data);
      document.getElementById("gyroX").innerHTML = obj.gyroX;
      document.getElementById("gyroY").innerHTML = obj.gyroY;
      document.getElementById("gyroZ").innerHTML = obj.gyroZ;
    }, false);
  
    source.addEventListener('temperature_readings', function(e) {
      console.log("temperature_reading", e.data);
      document.getElementById("temp").innerHTML = e.data;
      document.getElementById("hum").innerHTML = e.data;
    }, false);
  
    source.addEventListener('accelerometer_readings', function(e) {
      console.log("accelerometer_readings", e.data);
      var obj = JSON.parse(e.data);
      document.getElementById("accX").innerHTML = obj.accX;
      document.getElementById("accY").innerHTML = obj.accY;
      document.getElementById("accZ").innerHTML = obj.accZ;
    }, false);
  }