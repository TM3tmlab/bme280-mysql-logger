
## installation

```shell
sudo apt update
sudo apt install -y mariadb-server mariadb-client libmariadb-dev
```

## setup mariadb

### mariadb root パスワード

mariadb の root パスワードを決め、メモしましょう。

脆弱なサンプル: mariaroot

### MariaDB の初期設定スクリプト実行

```shell
sudo mysql_secure_installation
```

```shell
NOTE: RUNNING ALL PARTS OF THIS SCRIPT IS RECOMMENDED FOR ALL MariaDB
      SERVERS IN PRODUCTION USE!  PLEASE READ EACH STEP CAREFULLY!

In order to log into MariaDB to secure it, we'll need the current
password for the root user. If you've just installed MariaDB, and
haven't set the root password yet, you should just press enter here.

Enter current password for root (enter for none): ①
OK, successfully used password, moving on...

Setting the root password or using the unix_socket ensures that nobody
can log into the MariaDB root user without the proper authorisation.

You already have your root account protected, so you can safely answer 'n'.

Switch to unix_socket authentication [Y/n] n ②
 ... skipping.

You already have your root account protected, so you can safely answer 'n'.

Change the root password? [Y/n] Y ③
New password: ④
Re-enter new password: ⑤
Password updated successfully!
Reloading privilege tables..
 ... Success!


By default, a MariaDB installation has an anonymous user, allowing anyone
to log into MariaDB without having to have a user account created for
them.  This is intended only for testing, and to make the installation
go a bit smoother.  You should remove them before moving into a
production environment.

Remove anonymous users? [Y/n] Y ⑥
 ... Success!

Normally, root should only be allowed to connect from 'localhost'.  This
ensures that someone cannot guess at the root password from the network.

Disallow root login remotely? [Y/n] Y ⑦
 ... Success!

By default, MariaDB comes with a database named 'test' that anyone can
access.  This is also intended only for testing, and should be removed
before moving into a production environment.

Remove test database and access to it? [Y/n] Y ⑧
 - Dropping test database...
 ... Success!
 - Removing privileges on test database...
 ... Success!

Reloading the privilege tables will ensure that all changes made so far
will take effect immediately.

Reload privilege tables now? [Y/n] Y ⑨
 ... Success!

Cleaning up...

All done!  If you've completed all of the above steps, your MariaDB
installation should now be secure.

Thanks for using MariaDB!
```

### MariaDB のスキーマ初期化

データベースとユーザを作成

```shell
mariadb -u root -p < resource/init.sql 
```

ログイン確認

secretpass

```shell
mariadb -u logger -p -D sensor_db
```

```shell
mariadb -u reader -p -D sensor_db
```


テーブルを作成

```shell
mariadb -u logger -p < resource/schema.sql 
```

## Systemd サービスの追加

```shell
sudo make install-systemd
```
