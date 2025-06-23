#!/bin/bash

./realm_bmk/realm1 & ./realm_bmk/realm2

wait
echo "[CCA] All Realm VMs finished"
exit 0
