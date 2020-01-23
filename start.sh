#!/bin/bash
if [ -z $1 ]; then
    echo "usage:           ./start.sh OPTION"
    echo ""
    echo "OPTIONS:"
    echo "            py_prometheus"
    echo "              Simple Python collector"
    echo ""
    echo "            c_graphite"
    echo "              C collector with Graphite exporter"
    echo "            c_influxdb"
    echo "              C collector with InfluxDB exporter"
    echo ""
    echo "            xdp_promeheus"
    echo "            xdp_graphite"
    echo "            xdp_influxdb"
else
    if [ $1 == 'c_graphite' ]; then
        if [ ! -e int.p4app/report_collector/int_collector ]; then
            make -C int.p4app/report_collector/
        fi
        docker-compose up -d grafana graphite
        p4app run ./int.p4app c_exporter
        docker-compose down
    elif [ $1 == 'c_influxdb' ]; then
        make -C int.p4app/report_collector/ CFLAGS=-DINFLUXDB_EXPORTER
        docker-compose up -d grafana influxdb
        p4app run ./int.p4app c_influxdb
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
fi
