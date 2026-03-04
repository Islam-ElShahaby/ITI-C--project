#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

export LD_LIBRARY_PATH="${SCRIPT_DIR}/libs:${LD_LIBRARY_PATH}"
export COMMONAPI_CONFIG="${SCRIPT_DIR}/config/commonapi.ini"
export COMMONAPI4SOMEIP_CONFIG="${SCRIPT_DIR}/config/commonapi4someip.ini"
export COMMONAPI_DEFAULT_BINDING="someip"
export VSOMEIP_CONFIGURATION="${SCRIPT_DIR}/config/vsomeip_service.json"
export VSOMEIP_APPLICATION_NAME="TelemetryService"

echo "==========================================="
echo " Telemetry Service (RPi 3B+)"
echo " CPU + Memory + GPU + CPU Temp"
echo "==========================================="
echo "VSOMEIP_CONFIGURATION=$VSOMEIP_CONFIGURATION"
echo "COMMONAPI_CONFIG=$COMMONAPI_CONFIG"
echo ""

exec "${SCRIPT_DIR}/TelemetryService"
