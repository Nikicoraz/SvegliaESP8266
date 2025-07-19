#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266mDNS.h>

#include "sslcert.h"

BearSSL::ESP8266WebServerSecure server(443);
ESP8266WebServer serverHTTP(80);


void page(){
    server.send(200, "text/html", "Hello world!");
}

void secureRedirect(){
    serverHTTP.sendHeader("Location", String("https://espsveglia.local"), true);
    serverHTTP.send(301, "text/plain", "");
}

void setupServer(const std::function<void()>& connectWifi){
    // TODO: Modalità hotspot

    if(WiFi.status() != WL_CONNECTED){
        connectWifi();
    }


    // TODO: Gestisci quando la roba viene mandata via POST
    serverHTTP.on("/", secureRedirect);
    serverHTTP.begin();

    server.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));
    server.on("/", page);
    server.begin();
}

void loopServer(){
    serverHTTP.handleClient();
    server.handleClient();
    MDNS.update();
}