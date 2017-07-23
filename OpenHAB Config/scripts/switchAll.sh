#!/bin/bash
if [ $1 == 1 ]
        then
        /opt/rc-switch/switchOn.sh 1
        /opt/rc-switch/switchOn.sh 2
        /opt/rc-switch/switchOn.sh 3
fi

if [ $1 == 0 ]
        then
        /opt/rc-switch/switchOff.sh 1
        /opt/rc-switch/switchOff.sh 2
        /opt/rc-switch/switchOff.sh 3
fi