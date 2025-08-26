#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <mariadb/mysql.h>
#include "bme280.h"

// I2C デバイスのパス
#define I2C_DEV "/dev/i2c-1"
// BME280 I2C アドレス
#define BME280_I2C_ADDR BME280_I2C_ADDR_PRIM

// --------------------------------------
// Bosch BME280 は 読み書きなどの通信部分はユーザに任されている。
// そのためのコールバック関数を定義する。
// I2C 書き込み関数
static BME280_INTF_RET_TYPE user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    int fd = *(int *)intf_ptr;
    uint8_t buf[len + 1];
    buf[0] = reg_addr;

    for (uint32_t i = 0; i < len; i++)
    {
        buf[i + 1] = reg_data[i];
    }

    if (write(fd, buf, len + 1) != (int)(len + 1))
    {
        return BME280_E_COMM_FAIL;
    }

    return BME280_OK;
}

// I2C 読み込み関数
static BME280_INTF_RET_TYPE user_i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr)
{
    int fd = *(int *)intf_ptr;

    if (write(fd, &reg_addr, 1) != 1)
    {
        return BME280_E_COMM_FAIL;
    }

    if (read(fd, data, len) != (int)len)
    {
        return BME280_E_COMM_FAIL;
    }

    return BME280_OK;
}

// スリープ関数 (us単位)
static void user_delay_us(uint32_t period, void *intf_ptr)
{
    usleep(period);
}
// コールバック関数定義終わり
// --------------------------------------

int main(void)
{
    struct bme280_dev dev;
    struct bme280_settings settings;
    struct bme280_data comp_data;
    int fd;

    // I2C オープン
    if ((fd = open(I2C_DEV, O_RDWR)) < 0)
    {
        fprintf(stderr, "I2Cデバイスオープン失敗: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (ioctl(fd, 0x0703 /* I2C_SLAVE */, BME280_I2C_ADDR) < 0)
    {
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

    if (bme280_init(&dev) != BME280_OK)
    {
        fprintf(stderr, "BME280初期化失敗\n");
        close(fd);
        return EXIT_FAILURE;
    }

    // 設定
    // オーバーサンプリング回数指定
    settings.osr_h = BME280_OVERSAMPLING_1X;
    settings.osr_p = BME280_OVERSAMPLING_4X;
    settings.osr_t = BME280_OVERSAMPLING_1X;
    // 平滑などフィルタの設定
    settings.filter = BME280_FILTER_COEFF_OFF;

    uint8_t settings_sel = BME280_SEL_OSR_TEMP | BME280_SEL_OSR_PRESS | BME280_SEL_OSR_HUM | BME280_SEL_FILTER;
    if (bme280_set_sensor_settings(settings_sel, &settings, &dev) != BME280_OK)
    {
        fprintf(stderr, "BME280設定失敗\n");
        close(fd);
        return EXIT_FAILURE;
    }

    // 強制モードで測定
    if (bme280_set_sensor_mode(BME280_POWERMODE_FORCED, &dev) != BME280_OK)
    {
        fprintf(stderr, "BME280測定開始失敗\n");
        close(fd);
        return EXIT_FAILURE;
    }

    // 測定完了待ち（必要時間はデータシート参照）
    /* 時間目安 (bme280_cal_meas_delay で計算可能)
     * T_meas = 1.25ms  // 計測の基本
     * + (2.3ms * osr_t)
     * + (2.3ms * osr_p + 0.575ms)
     * + (2.3ms * osr_h + 0.575ms)
     */
    dev.delay_us(40000, dev.intf_ptr); // 40ms程度

    // データ取得
    // 取得値はキャリブレーション補正済み
    if (bme280_get_sensor_data(BME280_ALL, &comp_data, &dev) != BME280_OK)
    {
        fprintf(stderr, "BME280データ取得失敗\n");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    // センサーデータ取得完了

    // MySQL接続情報取得（環境変数から）
    const char *mysql_host = getenv("MYSQL_HOST");
    const char *mysql_user = getenv("MYSQL_USER");
    const char *mysql_pass = getenv("MYSQL_PASSWORD");
    const char *mysql_db = getenv("MYSQL_DATABASE");

    if (!mysql_host || !mysql_user || !mysql_pass || !mysql_db)
    {
        fprintf(stderr, "MySQL接続情報が環境変数に設定されていません\n");
        return EXIT_FAILURE;
    }

    // MySQL 接続
    MYSQL *conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, mysql_host, mysql_user, mysql_pass, mysql_db, 0, NULL, 0))
    {
        fprintf(stderr, "MySQL接続失敗: %s\n", mysql_error(conn));
        mysql_close(conn);
        return EXIT_FAILURE;
    }

    // クエリ作成
    char query[256];
    snprintf(query, sizeof(query),
             "INSERT INTO bme280_log (temperature, humidity, pressure, timestamp) "
             "VALUES (%.2f, %.2f, %.2f, NOW())",
             comp_data.temperature, comp_data.humidity, comp_data.pressure / 100.0f);

    // 実行
    if (mysql_query(conn, query))
    {
        fprintf(stderr, "INSERT失敗: %s\n", mysql_error(conn));
        mysql_close(conn);
        return EXIT_FAILURE;
    }

    mysql_close(conn);
    printf("記録完了: T=%.2f°C, H=%.2f%%, P=%.2f hPa\n",
           comp_data.temperature, comp_data.humidity, comp_data.pressure / 100.0f);

    return EXIT_SUCCESS;
}
