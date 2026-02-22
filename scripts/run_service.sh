#!/bin/bash
# Run GPU Service with proper vSOME/IP configuration
cd "$(dirname "$0")/.."

export COMMONAPI_CONFIG="$PWD/config/commonapi.ini"
export COMMONAPI4SOMEIP_CONFIG="$PWD/config/commonapi4someip.ini"
export COMMONAPI_DEFAULT_BINDING="someip"
export COMMONAPI_DEFAULT_FOLDER="/usr/local/lib"
export LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"
export VSOMEIP_CONFIGURATION="$PWD/config/vsomeip_service.json"
export VSOMEIP_APPLICATION_NAME="GpuService"

echo "Starting GPU Service..."
echo "COMMONAPI_CONFIG=$COMMONAPI_CONFIG"
echo "VSOMEIP_CONFIGURATION=$VSOMEIP_CONFIGURATION"

./build/GpuService
