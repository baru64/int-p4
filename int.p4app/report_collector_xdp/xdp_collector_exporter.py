import sys
import argparse
from bcc import BPF
from prometheus_client import Gauge, start_http_server
import ctypes

MAX_INT_HOP = 4
INT_DST_PORT = 9555
FLOW_LATENCY_THRESHOLD = 100 
HOP_LATENCY_THRESHOLD = 100
LINK_LATENCY_THRESHOLD = 100
QUEUE_OCCUPANCY_THRESHOLD = 1

class Event(ctypes.Structure):
    _fields_ = [
        ("src_ip",          ctypes.c_uint32),
        ("dst_ip",          ctypes.c_uint32),
        ("src_port",        ctypes.c_ushort),
        ("dst_port",        ctypes.c_ushort),
        ("ip_proto",        ctypes.c_ushort),

        ("hop_cnt",         ctypes.c_ubyte),

        ("flow_latency",    ctypes.c_uint32),
        ("switch_ids",      ctypes.c_uint32 * MAX_INT_HOP),
        ("queue_ids",       ctypes.c_uint32 * MAX_INT_HOP),

        ("e_new_flow",      ctypes.c_ubyte),
        ("e_flow_latency",  ctypes.c_ubyte),
        ("e_sw_latency",    ctypes.c_ubyte),
        ("e_link_latency",  ctypes.c_ubyte),
        ("e_q_occupancy",   ctypes.c_ubyte)
    ]

class Collector:

    def __init__(self):
        self.xdp_collector = BPF(src_file="xdp_report_collector.c", debug=1,
            cflags=[
                "-w",
                "-D_MAX_INT_HOP=%s" % MAX_INT_HOP,
                "-D_INT_DST_PORT=%s" % INT_DST_PORT,
                "-D_FLOW_LATENCY_THRESHOLD=%s" % FLOW_LATENCY_THRESHOLD,
                "-D_HOP_LATENCY_THRESHOLD=%s" % HOP_LATENCY_THRESHOLD,
                "-D_LINK_LATENCY_THRESHOLD=%s" % LINK_LATENCY_THRESHOLD,
                "-D_QUEUE_OCCUPANCY_THRESHOLD=%s" % QUEUE_OCCUPANCY_THRESHOLD,
            ])
        self.collector_fn = self.xdp_collector.load_func("report_collector", BPF.XDP)
        
        self.ifaces = []

        self.tb_flow = self.xdp_collector.get_table("tb_flow")
        self.tb_switch = self.xdp_collector.get_table("tb_switch")
        self.tb_link = self.xdp_collector.get_table("tb_link")
        self.tb_queue = self.xdp_collector.get_table("tb_queue")

        self.g_flow_latency = Gauge('flow_latency', 'total flow latency',
            ['src_ip', 'src_port', 'dst_ip', 'dst_port', 'ip_proto'])
        self.g_switch_latency = Gauge('switch_latency', 'switch latency',
            ['switch_id'])
        self.g_link_latency = Gauge('link_latency', 'link latency',
            ['egress_switch_id','egress_port_id',
            'ingress_switch_id', 'ingress_port_id'])
        self.g_queue_occupancy = Gauge('queue_occupancy', 'queue occupancy',
            ['switch_id', "queue_id"])

    def set_flow_latency(self, src_ip, dst_ip, src_port, dst_port, ip_proto):
        key = self.tb_flow.Key(src_ip, dst_ip, src_port, dst_port, ip_proto)
        val = self.tb_flow[key]
        self.g_flow_latency.labels(
            src_ip, dst_ip, src_port, dst_port, ip_proto
        ).set(str(val.flow_latency))
    
    def set_switch_latency(self, switch_id):
        key = self.tb_switch.Key(switch_id)
        val = self.tb_switch[key] # TODO fix KeyError
        self.g_switch_latency.labels(switch_id).set(str(val.hop_latency))

    def set_link_latency(self,  egress_switch_id,
                                egress_port_id,
                                ingress_switch_id,
                                ingress_port_id):
        key = self.tb_link.Key(
            egress_switch_id, egress_port_id, ingress_switch_id, ingress_port_id
        )
        val = self.tb_link[key]
        self.g_link_latency.labels(
            egress_switch_id, egress_port_id, ingress_switch_id, ingress_port_id
        ).set(str(val.link_latency))

    def set_queue_occupancy(self, switch_id, queue_id):
        key = self.tb_queue.Key(switch_id, queue_id)
        val = self.tb_queue[key]
        self.g_queue_occupancy.labels(
            switch_id, queue_id
        ).set(str(val.q_occupancy))

    def attach_iface(self, iface):
        self.ifaces.append(iface)
        self.xdp_collector.attach_xdp(iface, self.collector_fn)

    def detach_all_ifaces(self):
        for iface in self.ifaces:
            self.xdp_collector.remove_xdp(iface, 0)
        self.ifaces = []

    def open_events(self):
        def _process_event(ctx, data, size):
            event = ctypes.cast(data, ctypes.POINTER(Event)).contents
            print("Received packet")
            print(event.e_new_flow, event.e_sw_latency,
                event.e_q_occupancy, event.e_link_latency)
            print(event.src_ip, event.dst_ip, event.src_port,
                event.dst_port, event.ip_proto)
            # TODO detect route change
            if event.e_new_flow:
                self.set_flow_latency(
                    event.src_ip, event.dst_ip, event.src_port,
                    event.dst_port, event.ip_proto
                )

            if event.e_flow_latency:
                self.set_flow_latency(
                    event.src_ip, event.dst_ip, event.src_port,
                    event.dst_port, event.ip_proto
                )
                
            # TODO for every switch, link call set_x_
            print('hop count: ', event.hop_cnt)
            if event.e_sw_latency:
                for i in range(event.hop_cnt):
                    print('switch id:', event.switch_ids[i])
                    self.set_switch_latency(
                        event.switch_ids[i]
                    )
            # TODO add queue_ids to event info
            if event.e_q_occupancy;
                for i in range(event.hop_cnt):
                    print('queue:', event.switch_ids[i], event.queue_ids[i])
                    self.set_queue_occupancy(
                        event.switch_ids[i], event.queue_ids[i]
                    )


        self.xdp_collector["events"].open_perf_buffer(_process_event, page_cnt=512)
        
    def poll_events(self):
        self.xdp_collector.perf_buffer_poll()

########

if __name__ == "__main__":
    start_http_server(8000)
    # handle arguments
    parser = argparse.ArgumentParser(description='INT collector.')
    parser.add_argument("iface")
    args = parser.parse_args()
    
    collector = Collector()

    print("Attaching interface")
    collector.attach_iface(sys.argv[1])
    collector.open_events()
    print("eBPF loaded")
    try:
        while True:
            collector.poll_events()
    except KeyboardInterrupt:
        pass

    finally:
        collector.detach_all_ifaces()
        print("Detaching interfaces")
    
    print("Exitting...")