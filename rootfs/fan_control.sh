#!/bin/bash
#usage: to get system power and contorl fan speed.

head -1 /sys/class/thermal/thermal_zone0/temp
TEMP1=$(/bin/cat /sys/class/thermal/thermal_zone0/temp)
echo $TEMP1
echo 79 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio79/direction
echo 1 > /sys/class/gpio/gpio79/value

echo 80 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio80/direction
echo 1 > /sys/class/gpio/gpio80/value

echo 68 > /sys/class/gpio/export
echo in >  /sys/class/gpio/gpio68/direction

LOCK=$(/bin/cat /sys/class/gpio/gpio68/value)
while [ 0 -eq 0 ]
do
	if [ $LOCK -ne 0 ]
	then
      	TEMP1=$(/bin/cat /sys/class/thermal/thermal_zone0/temp)
      	#echo $TEMP1
  		#TEMP1≥ 49 且 TEMP1 ≤55 
      	if [ $TEMP1 -ge 49000  ] && [ $TEMP1 -le 55000 ]
      	then
        #	echo "fanControl:Fan speed is normal!"
        #	echo "fanControl:Middle Speed!"
        	echo 0 > /sys/class/gpio/gpio79/value 
        	echo 1 > /sys/class/gpio/gpio80/value
        #小于 49° 
      	elif [ $TEMP1 -lt 49000 ]
      	then
      	# 	echo "fanControl:No Speed!" 
        	echo 1 > /sys/class/gpio/gpio79/value
        	echo 1 > /sys/class/gpio/gpio80/value                                
      	else                                                   
        #	echo "fanControl:Full Speed" 
        	echo 0 > /sys/class/gpio/gpio79/value 
        	echo 0 > /sys/class/gpio/gpio80/value                 
      	fi
      	sleep 3                                              
	else                                 
      	#echo "fan state is error!"
      	sleep 2     
	fi 
done

echo 79 > /sys/class/gpio/unexport   
echo 68 > /sys/class/gpio/unexport
echo 80 > /sys/class/gpio/unexport
