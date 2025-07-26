if ! [  -e "cert.txt" ] || ! [ -e "key.txt" ]; then
    openssl req -x509 -newkey rsa:1024 -sha256 -keyout key.txt -out cert.txt -days 4096 -nodes -addext subjectAltName=DNS:espsveglia.local   # This last part is to ensure it works on the default mDNS
fi

if [ -e "cert.txt" ]; then
    cert=$(cat "cert.txt")
else
    echo "Could not find certificate"
    exit 1
fi

if [ -e "key.txt" ]; then
    key=$(cat "key.txt")
else
    echo "Could not find key"
    exit 1
fi

echo "#include <Arduino.h>

static const char serverCert[] PROGMEM = R\"EOF(
$cert
)EOF\";

static const char serverKey[] PROGMEM = R\"EOF(
$key
)EOF\";" > include/sslcert.h;
