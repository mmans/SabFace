var sabMeta = {
  baseUrl : '',
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

var sabConfig = {
  sab_use_ssl : false,
  sab_host : 'ed2oyecdfpgenbgy.myfritz.net',
  sab_port : 20378,
  sab_apikey : '848da14248a198fe4032209c3de39453',
  sab_username : 'admin',
  sab_password : 'ild!98',
  watch_vibrateOnNewDownload: true,
	watch_vibrateOnDownloadFinished: true,
	watch_intervalIdle: 900,
	watch_intervalActive: 10
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
  if (sabConfig.use_ssl){
    sabMeta.baseUrl = "https://";
  } else {
    sabMeta.baseUrl = "http://";
  }
  
  sabMeta.baseUrl += sabConfig.sab_host+":"+sabConfig.sab_port+"/sabnzbd/api?";
  console.log("Base Url = " + sabMeta.baseUrl);

  xhrRequest(sabMeta.baseUrl+"mode=auth", "GET", function(responseText){
    switch(responseText.trim()){
      case "None":
        sabMeta.valid = true;
        sabMeta.baseAUrl = sabMeta.baseUrl;
        break;
      case "apikey":
        sabMeta.valid = true;
        sabMeta.baseAUrl = sabMeta.baseUrl+"apikey="+sabConfig.sab_apikey+"&";
        break;
      case "login":
        sabMeta.valid = true;
        sabMeta.baseAUrl = sabMeta.baseUrl+"ma_username="+sabConfig.sab_username+"&ma_password="+sabConfig.sab_password+"&";
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


/*
  Watchface is loaded successfully
*/
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");
    getAuthenticationMethod();
  }
);

/*
  User wants to open configuration
*/
Pebble.addEventListener("showConfiguration",
  function(e) {
    console.log('Show Config');
    var configJson = encodeURIComponent(JSON.stringify(sabConfig));
    console.log("Opening url: " + "http://pebble.telemans.de/sabface-config.html#/?config="+configJson);
    //Load the remote config page
    Pebble.openURL("http://pebble.telemans.de/sabface-config.html#/?config="+configJson);
  }
);

/* 
  New configuration received from webview
*/
Pebble.addEventListener('webviewclosed',
  function(e) {
    console.log('Configuration window returned: ' + e.response);
    if (e.response == 'cancel'){
      // User clicked cancel
      console.log("User Clicked CANCEL");
    } else {
      sabConfig = JSON.parse(decodeURIComponent(e.response));
      sabMeta.valid = false;
      sendConfigToPebble();
    }
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    var cmd_type=0;
    cmd_type = e.payload.CMD_TYPE;
    console.log("AppMessage received --> " + cmd_type);
    
    switch (cmd_type){
      case 1:
        // Update data
        if (sabMeta.valid){
          updateSabData();
        } else {
          getAuthenticationMethod();
        }
      break;
    case 2:
        // Receiving configuration
        sabConfig.sab_use_ssl = e.payload.CONFIG_SAB_USE_SSL;
        sabConfig.sab_host = e.payload.CONFIG_SAB_HOST;
        sabConfig.sab_port = e.payload.CONFIG_SAB_PORT;
        sabConfig.sab_apikey = e.payload.CONFIG_SAB_APIKEY;
        sabConfig.sab_username = e.payload.CONFIG_SAB_USERNAME;
        sabConfig.sab_password = e.payload.CONFIG_SAB_PASSWORD;
        sabConfig.watch_vibrateOnNewDownload = e.payload.CONFIG_WATCH_VIB_NEW;
        sabConfig.watch_vibrateOnDownloadFinished = e.payload.CONFIG_WATCH_VIB_FIN;
        sabConfig.watch_intervalIdle = e.payload.CONFIG_WATCH_INT_IDLE;
        sabConfig.watch_intervalActive = e.payload.CONFIG_WATCH_INT_ACTIVE;
        break;
  }
  }                     
);

function sendConfigToPebble(){
  var dict = {
    "CONFIG_SAB_USE_SSL" : sabConfig.sab_use_ssl,
    "CONFIG_SAB_HOST" : sabConfig.sab_host,
    "CONFIG_SAB_PORT" : sabConfig.sab_port,
    "CONFIG_SAB_APIKEY" : sabConfig.sab_apikey,
    "CONFIG_SAB_USERNAME" : sabConfig.sab_username,
    "CONFIG_SAB_PASSWORD" : sabConfig.sab_password,
    "CONFIG_WATCH_VIB_NEW" : sabConfig.watch_vibrateOnNewDownload,
    "CONFIG_WATCH_VIB_FIN" : sabConfig.watch_vibrateOnDownloadFinished,
    "CONFIG_WATCH_INT_IDLE" : sabConfig.watch_intervalIdle,
    "CONFIG_WATCH_INT_ACTIVE" : sabConfig.watch_intervalActive
  };
  
  Pebble.sendAppMessage(dict,
    function(e){
      console.log("Data send successfull!");
    },
    function(e){
      console.log("Error while sending data!");
    });
  
}
