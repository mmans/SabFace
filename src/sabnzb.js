var init = true;

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
  sab_host : '',
  sab_port : 80,
  sab_apikey : '',
  sab_username : '',
  sab_password : '',
  watch_vibrateOnNewDownload: true,
	watch_vibrateOnDownloadFinished: true,
	watch_intervalIdle: 900,
	watch_intervalActive: 10
};

var xhrRequest = function (url, type, callback, errorCallback) {
  var xhr = new XMLHttpRequest();
  xhr.timeout=10000;
  xhr.onreadystatechange = function () {
    //console.log("[xhrRequest] status="+xhr.status);
    if (xhr.readyState==4 && xhr.status==200) {
      callback(xhr.responseText);
    }
  };
  xhr.onerror = errorCallback;
  xhr.open(type, url);
  xhr.send();
};

function initSabConnection(){
  //console.log("[initSabConnection] Enter");
  sabMeta.valid = false;
  
  if (sabConfig.use_ssl){
    sabMeta.baseUrl = "https://";
  } else {
    sabMeta.baseUrl = "http://";
  }
  
  sabMeta.baseUrl += sabConfig.sab_host+":"+sabConfig.sab_port+"/sabnzbd/api?";
  //console.log("[initSabConnection] Base Url = " + sabMeta.baseUrl);
  //console.log("[initSabConnection] Valid = " + sabMeta.valid);
  try{
   // console.log("[initSabConnection] Fetching Auth Method");
    xhrRequest(sabMeta.baseUrl+"mode=auth", "GET", function(responseText){
      //console.log("[initSabConnection] [XHR Callback] Get Auth Method result: ", responseText);
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
      if (sabMeta.valid) {fetchSabData();}
      sendDataToPebble();
    }, function(){
      // ON ERROR
      sabMeta.valid=false;
      sendDataToPebble();
    });
  }catch(e){
    //console.log("Error while fetching authentication mode!", e);
  }
  //console.log("[initSabConnection] Exit");
}


function update(){
  //console.log('[update] Enter');
  if (!sabMeta.valid){
    initSabConnection();
  } else {
    fetchSabData();
  }
  //console.log('[update] Exit');
}

function fetchSabData(){
  //console.log('[fetchSabData] Enter');
  if (sabMeta.valid){
  xhrRequest(sabMeta.baseAUrl+"mode=qstatus&output=json", "GET", function(responseText){
    //console.log('[update] [XHR Callback] Enter');
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
  }, function(){
      // ON ERROR
      sabMeta.valid=false;
      sendDataToPebble();
    });
  }
  
  sendDataToPebble();
  //console.log('[fetchSabData] Exit');
}

function sendDataToPebble(){
  //console.log("[sendDataToPebble] Enter");
  //console.log("2Valid = " + sabMeta.valid);

  var dict = {
    "KEY_VALID_CONNECTION" : sabMeta.valid?1:0,
    "KEY_MB_TOTAL" : sabData.mbtotal,
    "KEY_MB_LEFT" : sabData.mbleft,
    "KEY_DOWNLOADS" : sabData.downloads,
    "KEY_TIME_LEFT" : sabData.timeleft,
    "KEY_SPEED" : sabData.speed,
    "KEY_PROC_LEFT" : sabData.procleft
  };
  
  Pebble.sendAppMessage(dict,
    function(e){
      //console.log("[sendDataToPebble] [App Message Callback] Data send successfull!");
    },
    function(e){
      //console.log("[sendDataToPebble] [App Message Callback] Error while sending data!");
    });

  //console.log("[sendDataToPebble] Exit");
}


/*
  Watchface is loaded successfully
*/
Pebble.addEventListener('ready', 
  function(e) {
    //console.log("PebbleKit JS ready!");
  }
);

/*
  User wants to open configuration
*/
Pebble.addEventListener("showConfiguration",
  function(e) {
    //console.log('Show Config');
    var configJson = encodeURIComponent(JSON.stringify(sabConfig));
    //console.log("Opening url: " + "http://pebble.telemans.de/sabface-config.html#/?config="+configJson);
    //Load the remote config page
    Pebble.openURL("http://pebble.telemans.de/sabface-config.html#/?config="+configJson);
  }
);

/* 
  New configuration received from webview
*/
Pebble.addEventListener('webviewclosed',
  function(e) {
    //console.log('Configuration window returned: ' + e.response);
    if ((e.response === '') || (e.response == 'cancel')){
      // User clicked cancel
      //console.log("User Clicked CANCEL");
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
    //console.log("AppMessage received --> " + cmd_type);
    
    switch (cmd_type){
      case 1:
        // Update data
        update();
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
        
        if (init){
          init = false;
          update();
        }
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
      //console.log("Configuration data send successfull!");
    },
    function(e){
      //console.log("Error while sending configuration data!");
    });
  
}
