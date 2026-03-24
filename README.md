not yet

setup Ubuntu 

two times! first from usb

next from label on desktop



1\) Установка зависимостей (одним блоком, без повторов)

sudo apt update



sudo apt install -y \\

&#x20; git build-essential cmake ninja-build \\

&#x20; qt6-base-dev qt6-declarative-dev qt6-tools-dev qt6-tools-dev-tools \\

&#x20; qt6-serialport-dev qt6-serialbus-dev \\

&#x20; qml6-module-qtquick qml6-module-qtquick-window \\

&#x20; qml6-module-qtquick-controls qml6-module-qtquick-layouts \\

&#x20; qml6-module-qtquick-templates qml6-module-qtqml qml6-module-qtqml-workerscript \\

&#x20; qml6-module-qt-labs-settings





Опционально, если у тебя реально конфликтует USB-UART:

sudo apt remove -y brltty



2\) Клонирование проекта

cd \~

git clone https://github.com/dmpetr12/panelFull.git

cd panelFull



Сделать постоянные имена портов через udev

Подключи оба USB-RS485 адаптера и посмотри их атрибуты:

ls -l /dev/serial/by-id

ls -l /dev/serial/by-path

udevadm info -a -n /dev/ttyUSB0

udevadm info -a -n /dev/ttyUSB1

Ищи:

idVendor

idProduct

serial

Лучший вариант — привязка по serial.

Создай файл:

sudo nano /etc/udev/rules.d/99-rs485.rules

Пример для двух FTDI:

&#x20; GNU nano 7.2                           /etc/udev/rules.d/99-rs485.rules                                    

SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001",  SYMLINK+="rs485\_server"

SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523",  SYMLINK+="rs485\_internal""

Если serial нет, можно привязаться к физическому USB-порту, но это хуже.

Применить правила:

sudo udevadm control --reload-rules

sudo udevadm trigger

Проверить:

ls -l /dev/rs485\_internal

ls -l /dev/rs485\_server

2\. Дать права на последовательные порты

sudo usermod -aG dialout $USER

Потом перелогиниться или перезагрузиться.

(если виндщус просто пиши в конфиг com5 com6)

(можно

SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", SYMLINK+="rs485\_ftdi"

SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="rs485\_ch340"

Это значит:

FTDI 0403:6001 → /dev/rs485\_ftdi

CH340 1a86:7523 → /dev/rs485\_ch340)



убрать загрузку 

sudo nano /etc/default/grub

GRUB\_CMDLINE\_LINUX\_DEFAULT="quiet loglevel=0 systemd.show\_status=0 vt.global\_cursor\_default=0"

sudo update-grub

sudo reboot



3\) Сборка (out-of-source)

cmake ..

make 

После сборки появятся бинарники примерно такие:



build/hmi/panel-hmi

build/service/panel-backend

4\. Проверить запуск

backend

./service/panel-backend

HMI

./hmi/panel-hmi





!!! файлы кофига должны лежать в корне проекта



5\) atjaste yor comp 

stop wifi

stop bluetooth

energosever display off

show display change

remoute control unabled





Остановить:



pkill apppanel



5\. Установить программы в систему



Лучше положить в:



/opt/panel



Создаём структуру:



sudo mkdir -p /opt/panel/backend

sudo mkdir -p /opt/panel/hmi



&#x20;dont realise Копируем:

sudo cp service/panel-backend /opt/panel/backend/

sudo cp hmi/panel-hmi /opt/panel/hmi/





Создаём:



sudo nano /etc/systemd/system/panel-backend.service

\[Unit]

Description=Emergency Lighting Backend

After=network.target



\[Service]

Type=simple

User=peter

WorkingDirectory=/home/peter/panelFull/build/service

ExecStart=/home/peter/panelFull/build/service/panel-backend

AmbientCapabilities=CAP\_NET\_BIND\_SERVICE CAP\_SYS\_TIME

Restart=always

RestartSec=2



\[Install]

WantedBy=multi-user.target



Запускаем:



sudo systemctl daemon-reload

sudo systemctl enable panel-backend

sudo systemctl start panel-backend



Проверка:



systemctl status panel-backend

7\. Автозапуск HMI



Создаём:

Error writing /home/peter/.config/autostart/panel.desktop: No such file or direc



\~/.config/autostart/panel.desktop

nano \~/.config/autostart/panel.desktop



\[Desktop Entry]

Type=Application

Name=Panel HMI

Exec=/home/peter/panelFull/build/hmi/panel-hmi

X-GNOME-Autostart-enabled=true

Terminal=false





5\) Если используешь USB/COM (serial), дай права

sudo usermod -aG dialout $USER

\# перелогинься или перезагрузи систему



Автозапуск на панели (systemd user service) — удобно для “панель включилась → приложение стартовало”



Создай сервис:



mkdir -p \~/.config/systemd/user

nano \~/.config/systemd/user/apppanel.service





Вставь:



\[Unit]

Description=AppPanel

After=graphical-session.target



\[Service]

WorkingDirectory=%h/panel

ExecStart=%h/panel/build/apppanel

Restart=always

RestartSec=2

Environment=QT\_LOGGING\_RULES=\*.debug=false



\[Install]

WantedBy=default.target





Включи и запусти:



systemctl --user daemon-reload

systemctl --user enable --now apppanel.service





Логи:



journalctl --user -u apppanel.service -f





Остановить:



systemctl --user stop apppanel.service



sudo systemctl daemon-reload

sudo systemctl restart panel-backend





): выключить демон уведомлений

pkill xfce4-notifyd





Чтобы он не запускался автоматически:



mkdir -p \~/.config/autostart

cp /etc/xdg/autostart/xfce4-notifyd.desktop \~/.config/autostart/ 2>/dev/null

nano \~/.config/autostart/xfce4-notifyd.desktop





Найди строку:



X-GNOME-Autostart-enabled=true





и сделай:



X-GNOME-Autostart-enabled=false

Hidden=true





✅ После перезахода в сессию уведомлений не будет.





2\) Убрать курсор мыши (самое частое решение — unclutter)



Это стандартный способ для киосков.



Установить:

sudo apt install -y unclutter



Запустить (прячем через 0.1 сек):

unclutter -idle 0.1 -root \&



✅ Сделать скрытие курсора постоянным (автозапуск)



Создай автозапуск:



nano \~/.config/autostart/unclutter.desktop





Вставь:



\[Desktop Entry]

Type=Application

Name=HideCursor

Exec=unclutter -idle 0.1 -root

X-GNOME-Autostart-enabled=true





B) NetworkManager (Wi-Fi, сеть)



Можно отключить апплет:



pkill nm-applet





И убрать автозапуск:



cp /etc/xdg/autostart/nm-applet.desktop \~/.config/autostart/ 2>/dev/null

nano \~/.config/autostart/nm-applet.desktop





Поставь:



Hidden=true

У тебя уже правильная CMake-структура проекта. По скриншоту видно:



EmergencyLighting

&#x20;├ core

&#x20;│   └ core (библиотека)

&#x20;├ hmi

&#x20;│   └ panel-hmi       ← Qt интерфейс

&#x20;└ service

&#x20;    └ panel-backend   ← backend сервис



То есть:



panel-backend → сервис (логика, Modbus, управление)



panel-hmi → Qt/QML интерфейс панели



core → общая библиотека



Такую структуру очень удобно устанавливать на Ubuntu.



Ниже — как правильно развернуть такой проект на панели.



1\. Установить зависимости на Ubuntu



На чистой системе сначала ставим Qt и сборочные инструменты:



sudo apt update



sudo apt install -y \\

build-essential \\

cmake \\

git \\

qt6-base-dev \\

qt6-declarative-dev \\

qt6-tools-dev \\

qml6-module-qtquick \\

qml6-module-qtquick-controls \\

qml6-module-qtquick-layouts

2\. Скопировать проект на панель



Например:



/home/panel/EmergencyLighting



Через git:



git clone <repo>

cd EmergencyLighting



или просто скопировать.



3\. Собрать проект



Т.к. у тебя верхний CMakeLists, собираем всё сразу.



mkdir build

cd build



cmake ..

make -j4



После сборки появятся бинарники примерно такие:



build/hmi/panel-hmi

build/service/panel-backend

4\. Проверить запуск

backend

./service/panel-backend

HMI

./hmi/panel-hmi



Если QML лежит рядом — всё сразу заработает.





8\. После перезагрузки



Ubuntu будет:



1️⃣ запускать backend как сервис

2️⃣ запускать HMI автоматически



То есть панель будет вести себя как промышленный контроллер.



Очень важная вещь для твоего проекта



Сейчас у тебя CMake уже идеально подходит, но лучше добавить install() в CMakeLists.



Тогда установка будет одной командой:



sudo make install



Если хочешь, я покажу:



как правильно написать install() для твоего проекта



как собрать .deb пакет



как обновлять панель одной командой через SSH



Это реально очень упростит обслуживание панели.















