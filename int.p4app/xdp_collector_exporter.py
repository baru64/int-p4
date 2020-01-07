from bcc import BPF
from prometheus_client import Gauge, start_http_server
import ctypes

class Event(ctypes.Structure):
    _fields_ = []

class Collector:

    def __init__(self):
        self.xdp_collector = BPF(src_file="xdp_report_collector.c", debug=0,
            cflags=[]) # TODO ADD FLAGS
        self.collector_fn = self.xdp_collector.load_func("collector", BPF.XDP)
        
        self.tb_flow = self.xdp_collector.get_table("tb_flow")
        self.tb_switch = self.xdp_collector.get_table("tb_switch")
        self.tb_link = self.xdp_collector.get_table("tb_link")
        self.tb_queue = self.xdp_collector.get_table("tb_queue")

    def attach_iface(self, iface):
        self.xdp_collector.attach_xdp(iface, self.collector_fn)

    def open_events(self):
        def _process_event(ctx, data, size):
            pass
        self.xdp_collector["events"].open_perf_buffer(_process_event)
        

    def poll_events(self):
        self.xdp_collector.perf_buffer_poll()

if __name__ == "__main__":
    pass