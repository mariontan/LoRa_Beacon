#!/bin/bash

bash bump_kiosk_cnc.sh &
python beaconrx.py &
python  getbeacon_data_count.py 