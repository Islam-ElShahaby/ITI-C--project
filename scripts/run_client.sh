#!/bin/bash
# Run Logger App (Client) with proper vSOME/IP configuration
cd "$(dirname "$0")/.."

export COMMONAPI_CONFIG="$PWD/config/commonapi.ini"
export COMMONAPI4SOMEIP_CONFIG="$PWD/config/commonapi4someip.ini"
export COMMONAPI_DEFAULT_BINDING="someip"
export COMMONAPI_DEFAULT_FOLDER="/usr/local/lib"
export LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"
export VSOMEIP_CONFIGURATION="$PWD/config/vsomeip_client.json"
export VSOMEIP_APPLICATION_NAME="GpuClient"

echo "Starting Logger App (GPU Client)..."
echo "COMMONAPI_CONFIG=$COMMONAPI_CONFIG"
echo "VSOMEIP_CONFIGURATION=$VSOMEIP_CONFIGURATION"

./build/LoggerApp
