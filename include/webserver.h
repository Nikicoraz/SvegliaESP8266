#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266mDNS.h>

#include "sslcert.h"
#include "html_index.h"

BearSSL::ESP8266WebServerSecure server(443);
ESP8266WebServer serverHTTP(80);

void page(){
    server.sendHeader("Content-Encoding", "gzip");
    server.send(200, "text/html", html_index, html_index_L);
}

void secureRedirect(){
    serverHTTP.sendHeader("Location", String("https://espsveglia.local"), true);
    serverHTTP.send(301, "text/plain", "");
}

void handleSetWifi(const std::function<boolean(String, String)>& setWifiFunc){
    if(server.method() == HTTP_POST){
        String ssid = server.arg("ssid");
        String passwd = server.arg("passwd");
        if(ssid != "" && passwd != ""){
            if(setWifiFunc(ssid, passwd)){
                server.send(200, "text/plain", "OK");
            }else{
                server.send(500, "text/plain", "Could not connect to the network");
            }
        }else{
            server.send(400, "text/plain", "Missing parameters");
        }
    }else{
        server.send(405, "text/plain", "Method not allowed");
    }
}

void setupServer(const std::function<void()>& connectWifi, const std::function<boolean(String, String)>& setWifiFunc){
    // TODO: Modalità hotspot

    if(WiFi.status() != WL_CONNECTED){
        connectWifi();
    }

    serverHTTP.on("/", secureRedirect);
    serverHTTP.begin();

    server.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));
    server.on("/", page);
    server.on("/setWifi", HTTP_POST, [setWifiFunc](){handleSetWifi(setWifiFunc);});
    server.begin();
}

void loopServer(){
    serverHTTP.handleClient();
    server.handleClient();
    MDNS.update();
}