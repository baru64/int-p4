#!/bin/bash
while [ 1 ]; do
    curl http://10.0.2.2:8000/report_rx.py > /dev/null
    sleep 0.7
done
