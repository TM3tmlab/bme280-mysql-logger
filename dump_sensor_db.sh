#!/usr/bin/env bash
# Dumps the sensor database to a file
# Usage: ./dump_sensor_db.sh
# Description: This script dumps the schema and data of the sensor_db database into two separate SQL
# Output: Two files - one for schema and one for data
# Requires: MySQL client installed and access to the sensor_db database

date=$(date +%Y%m%d)

mysqldump -u root -p -B sensor_db --no-data > ${date}_sensor_db_schema_dump.sql
mysqldump -u root -p -B sensor_db --no-create-db --no-create-info > ${date}_sensor_db_data_dump.sql
