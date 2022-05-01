#include <WiFi.h>
#include <FS.h>
#include <LITTLEFS.h>
#include <EEPROM.h>
#include "WebServer.h"
#include "SimplePortal.h"
// getting access to the nice mime-type-table and getContentType()
#include "RequestHandlersImpl.h"
#include "ESPxWebFlMgr.h"

const word filemanagerport = 80;

String hlssid = "";
String hlpwd = "";

ESPxWebFlMgr filemgr(filemanagerport);

void setup() {
#ifdef DEBUG_ON  
  // the usual Serial stuff....
  Serial.begin(115200);
  while (! Serial) {
    delay(1);
  }
#endif

  if (!LITTLEFS.begin(true)) { // Format if failed.
    DEBUG("LITTLEFS Mount Failed");
    return;
  } else {
    DEBUG("LITTLEFS Started OK");
  }
EEPROM.begin(512);
  EEPROM.get(0, portalCfg); // try to get our stored values
  DEB("***** cfg SSID:");
  DEBUG(portalCfg.SSID);
  DEB("***** cfg succeed:");
  DEBUG(portalCfg.succeed);
  DEB("***** cfg tried:");
  DEBUG(portalCfg.tried);
  if ((!portalCfg.tried && String(portalCfg.SSID) != "") || (portalCfg.tried && portalCfg.succeed ) || (!portalCfg.tried && !portalCfg.succeed ) ){
    DEBUG("trying to connect to " + String(portalCfg.SSID));
    WiFi.begin(portalCfg.SSID, portalCfg.pass);
    
    while(WiFi.status() != WL_CONNECTED && WiFi.status() != WL_CONNECT_FAILED){
      delay(1);    
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      portalCfg.succeed = true;
      DEBUG("WIFI CONNECTED");
    } else {
      portalCfg.succeed = false;
      DEBUG("WIFI CONNECTION FAILED");
    }

    portalCfg.tried = true;
    EEPROM.put(0, portalCfg);
  }
  
  if (portalCfg.tried && !portalCfg.succeed) {
    listNetworks();
    portalRun(120000);  // run a blocking captive portal with (ms) timeout
    
    DEBUG(portalStatus());
    // status: 0 error, 1 connect, 2 AP, 3 local, 4 exit, 5 timeout
    
    switch (portalStatus() ) {
      case SP_SUBMIT:
        //save and reboot
        DEBUG("Got WiFi credentials");
        
        portalCfg.tried = false;
        portalCfg.succeed = false;
        EEPROM.put(0, portalCfg);
        EEPROM.end();
        esp_restart();
        break;
      case SP_ERROR:
      case SP_SWITCH_AP:
      case SP_SWITCH_LOCAL:
      case SP_EXIT:
      case SP_TIMEOUT:
      default:
        break;
    }

  }
  DEBUG("\n\nESPxFileManager");

  if (WiFi.status() == WL_CONNECTED) {
    DEB("Open Filemanager with http://");
    DEB(WiFi.localIP());
    DEB(":");
    DEB(filemanagerport);
    DEB("/");
    DEBUG();
  } else {
    DEBUG("No connection, restarting");
    EEPROM.end();
    esp_restart();
  }
  
  filemgr.begin();
    
  EEPROM.end();
}

void loop() {
  filemgr.handleClient();
}
