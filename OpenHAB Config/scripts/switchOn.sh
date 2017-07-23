#!/bin/bash
for run in {1..2}
do
/opt/rc-switch/rcswitch-pi/send 01010 $1 1
done
