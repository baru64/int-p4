#!/usr/bin/python

# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from mininet.net import Mininet
from mininet.link import Intf
from mininet.topo import Topo
from mininet.log import setLogLevel
from mininet.cli import CLI

from p4_mininet import P4Switch, P4Host

import argparse
from time import sleep
import os
import subprocess

_THIS_DIR = os.path.dirname(os.path.realpath(__file__))
_THRIFT_BASE_PORT = 22222

parser = argparse.ArgumentParser(description='Mininet demo')
parser.add_argument('--behavioral-exe', help='Path to behavioral executable',
                    type=str, action="store", required=True)
parser.add_argument('--json', help='Path to JSON config file',
                    type=str, action="store", required=True)
parser.add_argument('--cli', help='Path to BM CLI',
                    type=str, action="store", required=True)
parser.add_argument('--exporter', help='Exporter option',
                    type=str, action="store", required=True)
args = parser.parse_args()

class MyTopo(Topo):
    def __init__(self, sw_path, json_path, nb_hosts, nb_switches, links, **opts):
        # Initialize topology and default options
        Topo.__init__(self, **opts)
        for i in xrange(nb_switches):
           self.addSwitch('s%d' % (i + 1),
                            sw_path = sw_path,
                            json_path = json_path,
                            thrift_port = _THRIFT_BASE_PORT + i,
                            pcap_dump = True,
                            device_id = i,
                            enable_debugger = True)

        for h in xrange(nb_hosts):
            self.addHost('h%d' % (h + 1), ip="10.0.%d.%d" % ((h + 1) , (h + 1)),
                    mac="00:00:00:00:0%d:0%d" % ((h+1), (h+1)))

        for a, b in links:
            self.addLink(a, b)

def read_topo():
    nb_hosts = 0
    nb_switches = 0
    links = []
    with open("topo.txt", "r") as f:
        line = f.readline()[:-1]
        w, nb_switches = line.split()
        assert(w == "switches")
        line = f.readline()[:-1]
        w, nb_hosts = line.split()
        assert(w == "hosts")
        for line in f:
            if not f: break
            a, b = line.split()
            links.append( (a, b) )
    return int(nb_hosts), int(nb_switches), links


def readRegister(register, thrift_port, idx=None):
        p = subprocess.Popen(['simple_switch_CLI', '--thrift-port', str(thrift_port)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if idx is not None:
            stdout, stderr = p.communicate(input="register_read %s %d" % (register, idx))
            reg_val = filter(lambda l: ' %s[%d]' % (register, idx) in l, stdout.split('\n'))[0].split('= ', 1)[1]
            return long(reg_val)
        else:
            stdout, stderr = p.communicate(input="register_read %s" % (register))
            return stdout


def main(arg):
    nb_hosts, nb_switches, links = read_topo()
    topo = MyTopo(args.behavioral_exe,
                  args.json,
                  nb_hosts, nb_switches, links)

    net = Mininet(topo = topo,
                  host = P4Host,
                  switch = P4Switch,
                  controller = None,
                  autoStaticArp=True)

    print "create veth pair on mininet host"
    subprocess.Popen(['/bin/bash', '-c', '/scripts/add_veth.sh'])
    print "add veth to h3"
    h3 = net.get('h3')
    _intf = Intf('int-veth1', node=h3)
    h3.cmd("ip addr add 10.0.128.3/24 dev int-veth1")
    h3.cmd("ip link set up int-veth1")
    
    net.start()
   
    # EXPORTER OPTIONS -------------------------------------------------------
    if arg == 'py_prometheus':
        print "run pyton report collector on h3"
        h3.cmd("/usr/bin/python3 /tmp/report_rx.py &> /tmp/rx_log &")
        print "forward collector metrics to prometheus"
        subprocess.Popen(['/usr/bin/socat','TCP-LISTEN:8000,fork','TCP:10.0.128.3:8000'])
    elif arg == 'c_graphite':
        print "run c report collector on h3"
        h3.cmd("/tmp/report_collector_c/int_collector &> /tmp/rx_log &")
        print "forward collector metrics to graphite"
        subprocess.Popen(['/usr/bin/socat','TCP-LISTEN:2003,fork','TCP:graphite:2003'],
	     stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    elif arg == 'c_influxdb':
        print "run c report collector on h3"
        h3.cmd("/tmp/report_collector_c/int_collector &> /tmp/rx_log &")
        print "forward collector metrics to influxdb"
        subprocess.Popen(['/usr/bin/socat','TCP-LISTEN:8086,fork','TCP:influxdb:8086'],
	    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    elif arg == 'xdp_prometheus':
        print "run xdp report collector on h3"
        h3.cmd("/usr/bin/python /tmp/report_collector_xdp/xdp_collector_exporter.py h3-eth0 &> /tmp/rx_log &")
        print "forward collector metrics to prometheus"
        subprocess.Popen(['/usr/bin/socat','TCP-LISTEN:8000,fork','TCP:10.0.128.3:8000'])
    elif arg == 'xdp_graphite':
        print "run xdp report collector on h3"
        h3.cmd("/usr/bin/python /tmp/report_collector_xdp/xdp_collector_graphite.py h3-eth0 &> /tmp/rx_log &")
        print "forward collector metrics to graphite"
        subprocess.Popen(['/usr/bin/socat','TCP-LISTEN:2004,fork','TCP:graphite:2004'])
    elif arg == 'xdp_influxdb':
        print "not implemented"
        # h3.cmd("/usr/bin/python /tmp/report_collector_xdp/xdp_collector_influxdb.py &> /tmp/rx_log &")
        # print "forward collector metrics to influxdb"
        # subprocess.Popen(['/usr/bin/socat','TCP-LISTEN:8086,fork','TCP:influxdb:8086'],
	#     stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    # ------------------------------------------------------------------------
 
    for n in xrange(nb_hosts):
        h = net.get('h%d' % (n + 1))
        for off in ["rx", "tx", "sg"]:
            cmd = "/sbin/ethtool --offload eth0 %s off" % off
            print cmd
            h.cmd(cmd)
        print "disable ipv6"
        h.cmd("sysctl -w net.ipv6.conf.all.disable_ipv6=1")
        h.cmd("sysctl -w net.ipv6.conf.default.disable_ipv6=1")
        h.cmd("sysctl -w net.ipv6.conf.lo.disable_ipv6=1")
        h.cmd("sysctl -w net.ipv4.tcp_congestion_control=reno")
        h.cmd("iptables -I OUTPUT -p icmp --icmp-type destination-unreachable -j DROP")

    sleep(1)

    for i in xrange(nb_switches):
        cmd = [args.cli, "--json", args.json,
               "--thrift-port", str(_THRIFT_BASE_PORT + i)
               ]
        with open("commands"+str((i+1))+".txt", "r") as f:
            print " ".join(cmd)
            try:
                output = subprocess.check_output(cmd, stdin = f)
                print output
            except subprocess.CalledProcessError as e:
                print e
                print e.output

        s = net.get('s%d' % (n + 1))
        s.cmd("sysctl -w net.ipv6.conf.all.disable_ipv6=1")
        s.cmd("sysctl -w net.ipv6.conf.default.disable_ipv6=1")
        s.cmd("sysctl -w net.ipv6.conf.lo.disable_ipv6=1")
        s.cmd("sysctl -w net.ipv4.tcp_congestion_control=reno")
        s.cmd("iptables -I OUTPUT -p icmp --icmp-type destination-unreachable -j DROP")

    sleep(1)
    print "Ready !"
	
    CLI( net )
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    if args.exporter:
        main(args.exporter)
    else:
        main('py_prometheus')
