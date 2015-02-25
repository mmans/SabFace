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
    console.log(responseText);
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
    console.log("Valid " + sabMeta.valid);
    console.log("Base AUrl = " + sabMeta.baseAUrl);
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

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
  }                     
);