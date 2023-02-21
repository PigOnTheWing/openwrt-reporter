#!/bin/ash

pkg_name=$1

echo "Update pkgs"
opkg update

echo "Install deps"
opkg install mosquitto-ssl

echo "Install app"
opkg install $pkg_name
