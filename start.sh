#!/bin/bash
if [ $1 == 'c_exporter' ]; then
    if [ ! -e int.p4app/report_collector_c/int_collector ]; then
        make -C int.p4app/report_collector_c/
    fi
    docker-compose up -d grafana graphite
    p4app run ./int.p4app c_exporter
    docker-compose down
elif [ $1 == 'xdp_prometheus'  ]; then
    docker-compose up -d grafana prometheus
    p4app run ./int.p4app xdp_prom
    docker-compose down
elif [ $1 == 'py_prometheus'  ]; then
    docker-compose up -d grafana prometheus
    p4app run ./int.p4app py_prom
    docker-compose down
elif [ $1 == 'xdp_graphite'  ]; then
    docker-compose up -d grafana graphite
    p4app run ./int.p4app xdp_graphite
    docker-compose down
elif [ $1 == 'xdp_influxdb'  ]; then
    echo "not implemented"
fi
