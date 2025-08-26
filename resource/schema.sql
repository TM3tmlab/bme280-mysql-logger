
use sensor_db;

CREATE TABLE IF NOT EXISTS bme280_log (
    id INT AUTO_INCREMENT PRIMARY KEY COMMENT 'PK',
    timestamp DATETIME NOT NULL COMMENT '記録年月日',
    temperature DOUBLE NOT NULL COMMENT '温度(℃)',
    humidity DOUBLE NOT NULL COMMENT '湿度',
    pressure DOUBLE NOT NULL COMMENT '気圧(Pa)',

    INDEX (timestamp)
) ENGINE 'InnoDB' CHARACTER SET 'utf8mb4' COLLATE 'utf8mb4_bin' COMMENT 'BME280データログテーブル';
