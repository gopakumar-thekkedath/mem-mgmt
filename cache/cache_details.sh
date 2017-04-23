#!/bin/bash
#This script is not very intelligent, it assumes the user knows what he is doing
# 

MAX_CACHE_LEVEL=3

cpu_total=`cat /proc/cpuinfo | awk '/^processor/{print $3}' | tail -1`
if [ $# != 1 ]; then
	echo "Give the cpu number [0 - $cpu_total]"
	exit
fi

input=$@

echo "input=$input"
#echo "Max cpu number =$cpu_total, input cpu number is $input"

if [ $input -gt $cpu_total ]; then
	echo "$input > max cpu number ($cpu_total)"
	exit
fi


for ((i=0; i <=$MAX_CACHE_LEVEL; i++))
do
	echo "------------- CPU Cache ---------------------"
	level=`cat /sys/devices/system/cpu/cpu$input/cache/index$i/level`
	echo "		Level:L$level"
	type=`cat /sys/devices/system/cpu/cpu$input/cache/index$i/type`
	echo "		type:$type"
	cache_line=`cat /sys/devices/system/cpu/cpu$input/cache/index$i/coherency_line_size`
	echo "		Cache line size:$cache_line bytes"
	sets=`cat /sys/devices/system/cpu/cpu$input/cache/index$i/number_of_sets`
	echo "		Sets:$sets"
	ways=`cat /sys/devices/system/cpu/cpu$input/cache/index$i/ways_of_associativity`
	echo "		Ways:$ways"
	size=`cat /sys/devices/system/cpu/cpu$input/cache/index$i/size`
	echo "		Cache Size:$size bytes"
	cpu_list=`cat /sys/devices/system/cpu/cpu$input/cache/index$i/shared_cpu_list`
	echo "		Shared by CPUs:$cpu_list"
done
