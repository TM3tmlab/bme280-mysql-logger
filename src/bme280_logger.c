#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <mysql/mysql.h>
#include "bme280.h"

// I2C デバイスのパス
#define I2C_DEV "/dev/i2c-1"
// BME280 I2C アドレス
#define BME280_I2C_ADDR BME280_I2C_ADDR_PRIM

// I2C 書き込み関数
static int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len, void *intf_ptr) {
    int fd = *(int *)intf_ptr;
    uint8_t buf[len + 1];
    buf[0] = reg_addr;
    memcpy(buf + 1, data, len);
    if (write(fd, buf, len + 1) != (len + 1)) {
        return BME280_E_COMM_FAIL;
    }
    return BME280_OK;
}

// I2C 読み込み関数
static int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len, void *intf_ptr) {
    int fd = *(int *)intf_ptr;
    if (write(fd, &reg_addr, 1) != 1) {
        return BME280_E_COMM_FAIL;
    }
    if (read(fd, data, len) != len) {
        return BME280_E_COMM_FAIL;
    }
    return BME280_OK;
}

// スリープ関数 (us単位)
static void user_delay_us(uint32_t period, void *intf_ptr) {
    usleep(period);
}

int main(void) {
    struct bme280_dev dev;
    struct bme280_data comp_data;
    int fd;

    // I2C オープン
    if ((fd = open(I2C_DEV, O_RDWR)) < 0) {
        fprintf(stderr, "I2Cデバイスオープン失敗: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (ioctl(fd, 0x0703 /* I2C_SLAVE */, BME280_I2C_ADDR) < 0) {
        fprintf(stderr, "I2Cアドレス設定失敗: %s\n", strerror(errno));
        close(fd);
        return EXIT_FAILURE;
    }

    // BME280 初期化
    dev.intf = BME280_I2C_INTF;
    dev.read = user_i2c_read;
    dev.write = user_i2c_write;
    dev.delay_us = user_delay_us;
    dev.intf_ptr = &fd;

    if (bme280_init(&dev) != BME280_OK) {
        fprintf(stderr, "BME280初期化失敗\n");
        close(fd);
        return EXIT_FAILURE;
    }

    // 設定
    uint8_t settings_sel;
    dev.settings.osr_h = BME280_OVERSAMPLING_1X;
    dev.settings.osr_p = BME280_OVERSAMPLING_16X;
    dev.settings.osr_t = BME280_OVERSAMPLING_2X;
    dev.settings.filter = BME280_FILTER_COEFF_16;
    settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL |
                   BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

    if (bme280_set_sensor_settings(settings_sel, &dev) != BME280_OK) {
        fprintf(stderr, "BME280設定失敗\n");
        close(fd);
        return EXIT_FAILURE;
    }

    // 強制モードで測定
    if (bme280_set_sensor_mode(BME280_FORCED_MODE, &dev) != BME280_OK) {
        fprintf(stderr, "BME280測定開始失敗\n");
        close(fd);
        return EXIT_FAILURE;
    }

    // 測定完了待ち（必要時間はデータシート参照）
    dev.delay_us(40000, dev.intf_ptr); // 40ms程度

    // データ取得
    if (bme280_get_sensor_data(BME280_ALL, &comp_data, &dev) != BME280_OK) {
        fprintf(stderr, "BME280データ取得失敗\n");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);

    // MySQL接続情報取得（環境変数から）
    const char *mysql_host = getenv("MYSQL_HOST");
    const char *mysql_user = getenv("MYSQL_USER");
    const char *mysql_pass = getenv("MYSQL_PASSWORD");
    const char *mysql_db   = getenv("MYSQL_DATABASE");

    if (!mysql_host || !mysql_user || !mysql_pass || !mysql_db) {
        fprintf(stderr, "MySQL接続情報が環境変数に設定されていません\n");
        return EXIT_FAILURE;
    }

    // MySQL 接続
    MYSQL *conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, mysql_host, mysql_user, mysql_pass, mysql_db, 0, NULL, 0)) {
        fprintf(stderr, "MySQL接続失敗: %s\n", mysql_error(conn));
        mysql_close(conn);
        return EXIT_FAILURE;
    }

    // クエリ作成
    char query[256];
    snprintf(query, sizeof(query),
        "INSERT INTO bme280_log (temperature, humidity, pressure, created_at) "
        "VALUES (%.2f, %.2f, %.2f, NOW())",
        comp_data.temperature, comp_data.humidity, comp_data.pressure / 100.0f);

    // 実行
    if (mysql_query(conn, query)) {
        fprintf(stderr, "INSERT失敗: %s\n", mysql_error(conn));
        mysql_close(conn);
        return EXIT_FAILURE;
    }

    mysql_close(conn);
    printf("記録完了: T=%.2f°C, H=%.2f%%, P=%.2f hPa\n",
           comp_data.temperature, comp_data.humidity, comp_data.pressure / 100.0f);

    return EXIT_SUCCESS;
}
