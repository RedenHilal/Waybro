#!/bin/bash


function get_bkl_devpath {
	local BACKLIGHT_DEV_PATH=$(find /sys/class/backlight/ -mindepth 1)
	echo $BACKLIGHT_DEV_PATH
}

BKL_DEVPATH=$(get_bkl_devpath)
BKL_SCALE=$(cat $BKL_DEVPATH/max_brightness)

echo "-DBRIGHTNESS_PATH=\"$BKL_DEVPATH/brightness\" -DBRIGHTNESS_SCALE=$BKL_SCALE"
