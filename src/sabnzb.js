var host = "ed2oyecdfpgenbgy.myfritz.net";
var port = "20378";
var apikey = "848da14248a198fe4032209c3de39453";
var username = "admin";
var password = "test";
var sabMeta = {
  baseUrl  : '',
  baseAUrl : '',
  valid    : false,
}; 
var sabData = {
  downloads : 0,
  timeleft : '0:00:00',
  mbtotal : 0,
  mbleft : 0,
  paused : false,
  speed : 0,
  procleft : 0
};

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function getAuthenticationMethod(){
  xhrRequest(sabMeta.baseUrl+"mode=auth", "GET", function(responseText){
    switch(responseText.trim()){
      case "None":
        sabMeta.valid = true;
        sabMeta.baseAUrl = sabMeta.baseUrl;
        break;
      case "apikey":
        sabMeta.valid = true;
        sabMeta.baseAUrl = sabMeta.baseUrl+"apikey="+apikey+"&";
        break;
      case "login":
        sabMeta.valid = true;
        sabMeta.baseAUrl = sabMeta.baseUrl+"ma_username="+username+"&ma_password="+password+"&";
        break;
      default:
        sabMeta.valid = false;
        break;
    }       
    if (sabMeta.valid) updateSabData();
  });
}


function updateSabData(){
  console.log('Updating SabData...');
  xhrRequest(sabMeta.baseAUrl+"mode=qstatus&output=json", "GET", function(responseText){
    var json = JSON.parse(responseText);
    sabData.mbtotal = json.mb * 100;
    sabData.mbleft = json.mbleft * 100;
    sabData.downloads = json.jobs.length;
    sabData.timeleft = json.timeleft;
    sabData.paused = json.paused;
    sabData.speed = json.speed.trim();
    if (sabData.mbtotal>0){
      sabData.procleft = Math.round((sabData.mbleft / sabData.mbtotal)*100);
    }
    sendDataToPebble();
  });
  
  
}

function sendDataToPebble(){
  var dict = {
    "KEY_VALID_CONNECTION" : sabMeta.valid,
    "KEY_MB_TOTAL" : sabData.mbtotal,
    "KEY_MB_LEFT" : sabData.mbleft,
    "KEY_DOWNLOADS" : sabData.downloads,
    "KEY_TIME_LEFT" : sabData.timeleft,
    "KEY_SPEED" : sabData.speed,
    "KEY_PROC_LEFT" : sabData.procleft
  };
  
  Pebble.sendAppMessage(dict,
    function(e){
      console.log("Data send successfull!");
    },
    function(e){
      console.log("Error while sending data!");
    });
}




Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");
    sabMeta.baseUrl = "http://"+host+":"+port+"/sabnzbd/api?";
    console.log("Base Url = " + sabMeta.baseUrl);
    getAuthenticationMethod();
  }
);

Pebble.addEventListener("showConfiguration",
  function(e) {
    //Load the remote config page
    Pebble.openURL("http://pebble.telemans.de/sabface-config.html");
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    if (sabMeta.valid){
      updateSabData();
    } else {
      getAuthenticationMethod();
    }
  }                     
);
