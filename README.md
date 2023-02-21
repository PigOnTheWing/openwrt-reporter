# openwrt-reporter
##Общая информация
Openwrt-reporter - приложение, работающее в среде openwrt. Приложение подключается к MQTT серверу и периодически отправляет некоторые параметры на сервер. Для настройки конфигурации используется uci, для изменения периода ubus.

Данный репозиторий содержит все необходимые ресурсы для компиляции и интеграции приложения в openwrt.

Проект содержит следующие директории:
* **.** - исходный код приложения
* **pkg** - Makefile для сборки приложения в качестве пакета для **opkg** и конфигурационный файл для **uci**
* **docker** - содержит небольшой Dockerfile для быстрой проверки приложения

##Установка
Для установки приложения скопируйте Makefile из **pkg** в репозиторий openwrt:
```
$ mkdir <OPENWRT_SRC>/custom <OPENWRT_SRC>/custom/reporter
$ cp pkg/Makefile <OPENWRT_SRC>/custom/reporter
```
**Чтобы проект собрался нужно присвоить переменной SOURCE_DIR в pkg/Makefile расположение папки с исходниками**
После этого необходимо добавить созданную директорию в feeds.conf:
```
$ cd <OPENWRT_SRC>
$ echo "src-link custom <OPENWRT_SRC>/custom" >> feeds.conf
```
И обновить список пакетов:
```
$ ./scripts/feeds update custom
$ ./scripts/feeds install -a -p custom
```
Далее необходимо включить пакет с помощью **menuconfig** и поставить галочку в секции **"Custom"**:
```
$ make menuconfig
```
Последний шаг сборки - компиляция:
```
$ make package/reporter/{clean,compile}
```
После этого в папке **<OPENWRT_SRC>/bin/packages/<_ARCH_>/custom** должен появиться пакет **reporter_1.0-1_<_ARCH_>.ipk**. Этот файл можно использовать в openwrt:
```
$ opkg install reporter_1.0-1_<_ARCH_>.ipk
```
##Использование
После установки нужно запустить приложение:
```
$ reporter &
```
При старте приложение не посылает отчеты, чтобы это исправить, необходимо послать запрос через ubus:
```
$ ubus call reporter report '{"period": 10}'
```
Эта команда задаст десятисекундный период отправки отчетов.

Куда отправляются отчеты?
Это можно посмотреть с помощью uci:
```
$ uci show reporter_conf.reporter
```
Вывод команды будет примерно следующим:
```
config reporter_t reporter
  option period '10'
  option enabled '1'
  option host 'localhost'
  option topic 'reporter/state'

```
Здесь параметр **host** отвечает за адрес сервера, а **topic** за, собственно, топик

Для того, чтобы отключить отчеты, нужно передать приложению период равный нулю:
```
$ $ ubus call reporter report '{"period": 0}'
```
##Docker
Для случая тестирования приложения для архитектуры x86_64, можно использовать **docker/Dokerfile**
Все что нужно, это скопировать **reporter_1.0-1_x86_64.ipk** в папку docker и вызвать docker build:
```
$ docker build -t reporter .
```
После запуска контейнера установка приложения осущуствляется запуском скрипта **setup.sh**:
```
$ ./setup.sh
```