/*     let scene, camera, rendered, cube;
    function parentWidth(elem) {
        return elem.parentElement.clientWidth;
    }

    function parentHeight(elem) {
        return elem.parentElement.clientHeight;
    }

    function init3D(){
        scene = new THREE.Scene();
        scene.background = new THREE.Color(0xffffff);

        camera = new THREE.PerspectiveCamera(75, parentWidth(document.getElementById("3Dcube")) / parentHeight(document.getElementById("3Dcube")), 0.1, 1000);

        rendered = new THREE.WebGLRenderer({ antialias: true });
        rendered.setSize(parentWidth(document.getElementById("3Dcube")), parentHeight(document.getElementById("3Dcube")));

        document.getElementById('3Dcube').appendChild(rendered.domElement);

        // Create a geometry
        const geometry = new THREE.BoxGeometry(5, 1, 4);

        // Materials of each face
        const cubeMaterials = [
            new THREE.MeshBasicMaterial({color: 0x03045e}),
            new THREE.MeshBasicMaterial({color: 0x023e8a}),
            new THREE.MeshBasicMaterial({color: 0x0077b6}),
            new THREE.MeshBasicMaterial({color: 0x03045e}),
            new THREE.MeshBasicMaterial({color: 0x023e8a}),
            new THREE.MeshBasicMaterial({color: 0x0077b6}),
        ];

        const material = new THREE.MeshFaceMaterial(cubeMaterials);

        cube = new THREE.Mesh(geometry, material);
        scene.add(cube);
        camera.position.z = 5;
        rendered.render(scene, camera);
    }

    // Resize the 3D object when the browser window changes size
    function onWindowResize(){
        camera.aspect = parentWidth(document.getElementById("3Dcube")) / parentHeight(document.getElementById("3Dcube"));
        //camera.aspect = window.innerWidth /  window.innerHeight;
        camera.updateProjectionMatrix();
        //rendered.setSize(window.innerWidth, window.innerHeight);
        rendered.setSize(parentWidth(document.getElementById("3Dcube")), parentHeight(document.getElementById("3Dcube")));

    }

    window.addEventListener('resize', onWindowResize, false);

    // Create the 3D representation
    init3D();  */

    // Create events for the sensor readings
    if (!!window.EventSource) {
        var source = new EventSource('/events');

        source.addEventListener('open',function(e){
          console.log("Events Connected");
        }, false);

        source.addEventListener('error', function(e){
          if (e.target.readyState !== EventSource.OPEN) {
            console.log("Events Disconnected");
          }
        }, false);

        source.addEventListener('gyro_readings', function(e){
          const obj = JSON.parse(e.data);
          document.getElementById("gyroX").innerHTML = obj.gyroX.toFixed(4);
          document.getElementById("gyroY").innerHTML = obj.gyroY.toFixed(4);
          document.getElementById("gyroZ").innerHTML = obj.gyroZ.toFixed(4);
        }, false);

        source.addEventListener('temperature_readings', function(e){
          const obj = JSON.parse(e.data);
          document.getElementById("temp").innerHTML = obj.temp.toFixed(2);
          document.getElementById("hum").innerHTML = obj.hum.toFixed(2);
        }, false);

        source.addEventListener('accelerometer_readings', function(e){
          const obj = JSON.parse(e.data);
          document.getElementById("accX").innerHTML = obj.accX.toFixed(4);
          document.getElementById("accY").innerHTML = obj.accY.toFixed(4);
          document.getElementById("accZ").innerHTML = obj.accZ.toFixed(4);
        }, false);
      }

    /* function resetPosition(element){
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/"+element.id, true);
        console.log(element.id);
        xhr.send();
    } */
