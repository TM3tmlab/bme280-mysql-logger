
CREATE DATABASE IF NOT EXISTS sensor_db
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_bin
;

CREATE USER IF NOT EXISTS 'logger'@'localhost' IDENTIFIED BY 'secretpass';
GRANT ALL PRIVILEGES ON sensor_db.* TO 'logger'@'localhost';

CREATE USER IF NOT EXISTS 'reader'@'%' IDENTIFIED BY 'secretpass';
GRANT SELECT ON sensor_db.* TO 'reader'@'%';

FLUSH PRIVILEGES;
