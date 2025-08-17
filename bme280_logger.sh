#!/bin/bash
# BME280 Logger launcher with .env loading

ENV_FILE="/etc/bme280_logger/.env"

if [ ! -f "$ENV_FILE" ]; then
    echo "ERROR: $ENV_FILE not found"
    echo "Do copy and rewrite .env.example to $ENV_FILE"
    exit 1
fi

# load environment variables
set -o allexport
source "$ENV_FILE"
set +o allexport

# run the logger
exec /usr/local/bin/bme280_logger "$@"
