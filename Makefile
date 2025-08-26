# Makefile for BME280 Logger

CC = gcc
CFLAGS = -Wall -O2
LIBS = -lmariadb

SRCDIR = src
BIN = bme280_logger
# Boschのドライバをcontrib配下から取り込む
BME280_DIR = contlib/bme280_driver
# ビルド対象ソース
SRCS = $(SRCDIR)/bme280_logger.c $(BME280_DIR)/bme280.c
# インクルードパス追加（bme280.h を見つけるため）
CFLAGS += -I$(BME280_DIR)

SH_SCRIPT = bme280_logger.sh
ENV_FILE = .env

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
SYSCONFDIR = /etc/bme280_logger

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

install: all
	# バイナリ
	install -Dm755 $(BIN) $(BINDIR)/$(BIN)
	# シェルスクリプト
	install -Dm755 $(SH_SCRIPT) $(BINDIR)/$(SH_SCRIPT)
	# 環境変数ファイルのサンプル
	install -Dm644 $(ENV_FILE) $(SYSCONFDIR)/$(ENV_FILE)
	@echo "Installed to $(BINDIR) and $(SYSCONFDIR)"

install-systemd:
	install -Dm644 systemd/bme280-logger.service /etc/systemd/system/bme280-logger.service
	install -Dm644 systemd/bme280-logger.timer /etc/systemd/system/bme280-logger.timer
	systemctl daemon-reload
	systemctl enable --now bme280-logger.timer

uninstall-systemd:
	systemctl stop bme280-logger.timer
	systemctl disable --now bme280-logger.timer
	rm -f /etc/systemd/system/bme280-logger.service
	rm -f /etc/systemd/system/bme280-logger.timer
	systemctl daemon-reload
	systemctl reset-failed

uninstall:
	rm -f $(BINDIR)/$(BIN)
	rm -f $(BINDIR)/$(SH_SCRIPT)
	rm -f $(SYSCONFDIR)/.env

clean:
	rm -f $(BIN)
