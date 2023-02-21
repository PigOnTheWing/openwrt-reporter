#!/bin/ash

pkg_name=$REPORTER

echo "Update pkgs"
opkg update

echo "Install deps"
opkg install mosquitto-ssl

echo "Install app"
opkg install /tmp/$pkg_name
