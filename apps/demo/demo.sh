#!/bin/bash

set -i

exit_script ()
{
    echo "please provide 'pub' or 'sub' parameter only"
    exit
}

if [[ $# -ne 1 || ( "$1" != "pub"  &&  "$1" != "sub") ]] 
then
    exit_script
fi

source ./libs/evrythng/core/Config

CA_PATH='evrythng-c-library/client.pem'

build_debug/apps/demo/evrythng-demo --$1 -u ${MQTT_URL} -t ${THNG_1} -k ${DEVICE_API_KEY} -n ${PROPERTY_1} -c ${CA_PATH}
