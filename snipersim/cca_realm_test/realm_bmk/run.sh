#!/bin/bash

$SNIPER_HOME/dynamorio/bin64/drrun -c $SNIPER_HOME/dynamorio/clients/lib64/release/libdr-frontend.so -roi 0 -f 0 -d 0 -b 0 -o trace1 -e 0 -s 0 -r 0 -stop 0 -verbose -- ./realm_bmk/realm1 &
$SNIPER_HOME/dynamorio/bin64/drrun -c $SNIPER_HOME/dynamorio/clients/lib64/release/libdr-frontend.so -roi 0 -f 0 -d 0 -b 0 -o trace2 -e 0 -s 0 -r 0 -stop 0 -verbose -- ./realm_bmk/realm2

wait
echo "[CCA] All Realm VMs finished"
exit 0
