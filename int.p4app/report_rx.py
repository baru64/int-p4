import io
import socket

OFFSET = 14 + 20 + 8
SWITCH_ID_BIT = 0b10000000
L1_PORT_IDS_BIT = 0b01000000
HOP_LATENCY_BIT = 0b00100000
QUEUE_BIT = 0b00010000
INGRESS_TSTAMP_BIT = 0b00001000
EGRESS_TSTAMP_BIT = 0b00000100
L2_PORT_IDS_BIT = 0b00000010
EGRESS_PORT_TX_UTIL_BIT = 0b00000001

class HopMetadata():
    def __init__(self):
        self.switch_id = None
        self.l1_ingress_port_id = None
        self.l1_egress_port_id = None
        self.hop_latency = None
        self.q_id = None
        self.q_occupancy = None
        self.ingress_tstamp = None
        self.egress_tstamp = None
        self.l2_ingress_port_id = None
        self.l2_egress_port_id = None
        self.egress_port_tx_util = None
    
    @staticmethod
    def from_bytes(data, ins_map):
        hop = HopMetadata()
        d = io.BytesIO(data)
        if ins_map & SWITCH_ID_BIT:
            hop.switch_id = int.from_bytes(d.read(4), byteorder='big')
        if ins_map & L1_PORT_IDS_BIT:
            hop.l1_ingress_port_id = int.from_bytes(d.read(2), byteorder='big')
            hop.l1_egress_port_id = int.from_bytes(d.read(2), byteorder='big')
        if ins_map & HOP_LATENCY_BIT:
            hop.hop_latency = int.from_bytes(d.read(4), byteorder='big')
        if ins_map & QUEUE_BIT:
            hop.q_id = int(d.read(1), byteorder='big')
            hop.q_occupancy = int.from_bytes(d.read(3), byteorder='big')
        if ins_map & INGRESS_TSTAMP_BIT:
            hop.ingress_tstamp = int.from_bytes(d.read(4), byteorder='big')
        if ins_map & EGRESS_TSTAMP_BIT:
            hop.egress_tstamp = int.from_bytes(d.read(4), byteorder='big')
        if ins_map & L2_PORT_IDS_BIT:
            hop.l2_ingress_port_id = int.from_bytes(d.read(4), byteorder='big')
            hop.l2_egress_port_id = int.from_bytes(d.read(4), byteorder='big')
        if ins_map & EGRESS_PORT_TX_UTIL_BIT:
            hop.egress_port_tx_util = int.from_bytes(d.read(4), byteorder='big')
        return hop

    def __str__(self):
        return str(vars(self))

class Report():
    def __init__(self, data):
        # report header
        hdr = data[:16]
        self.ver = hdr[0] >> 4
        self.len = hdr[0] & 0x0f
        self.nprot = hdr[1] >> 5
        self.rep_md_bits = (hdr[1] & 0x1f) + (hdr[2] >> 7)
        self.d = hdr[2] & 0x01
        self.q = hdr[3] >> 7
        self.f = (hdr[3] >> 6) & 0x01
        self.hw_id = hdr[3] & 0x3f
        self.switch_id = int.from_bytes(hdr[4:8], byteorder='big')
        self.seq_num = int.from_bytes(hdr[8:12], byteorder='big')
        self.ingress_tstamp = int.from_bytes(hdr[12:16], byteorder='big')

        # flow id
        ip_hdr = data[30:50]
        udp_hdr = data[50:58]
        self.flow_id = (
            ip_hdr[12:16],  # src_ip
            ip_hdr[16:20],  # dst_ip
            udp_hdr[:2],    # src_port
            udp_hdr[2:4],   # dst_port
            ip_hdr[9]       # protocol
        )

        # int shim
        self.int_shim = data[OFFSET + 16:OFFSET + 20]
        self.int_data_len = int(self.int_shim[2]) - 3

        # int header
        self.int_hdr = data[OFFSET + 20:OFFSET + 28]
        self.hop_data_len = int(self.int_hdr[2] & 0x1f)
        self.ins_map = int.from_bytes(self.int_hdr[4:6], byteorder='big')
        self.hop_count = int(self.int_data_len / self.hop_data_len)

        # int metadata
        self.int_meta = data[OFFSET + 28:]
        print(self.int_meta)
        self.hop_metadata = []
        for i in range(self.hop_count):
            metadata_source = self.int_meta[i*self.hop_data_len*4:(i+1)*self.hop_data_len*4]
            print(metadata_source)
            self.hop_metadata.append(HopMetadata.from_bytes(metadata_source, self.ins_map))

    def __str__(self):
        hop_info = ''
        for hop in self.hop_metadata:
            hop_info += str(hop) + '\n'
        return "sw: {} seq: {} tstamp: {} ins_map: {} \n {}".format(
            self.switch_id,
            self.seq_num,
            self.ingress_tstamp,
            self.ins_map,
            hop_info
        )

class FlowInfo():

    def __init__(self):
        # flow id - 5 tuple: (src_ip, dst_ip, src_port, dst_port, ip_proto)
        self.flow_id = None
        
        self.switch_ids = []
        self.l1_ingress_port_ids = []
        self.l1_egress_port_ids = []
        self.hop_latencies = []
        self.q_ids = []
        self.q_occups = []
        self.ingress_tstamps = []
        self.egress_tstamps = []
        self.l2_ingress_port_ids = []
        self.l2_egress_port_ids = []
        self.egress_port_tx_utils = []

    @staticmethod
    def from_report(report: Report):
        flow = FlowInfo()
        flow.flow_id = report.flow_id
        
        for hop in report.hop_metadata:
            if hop.switch_id:
                flow.switch_ids.append(hop.switch_id)
            if hop.l1_ingress_port_id:
                flow.l1_ingress_port_ids.append(hop.l1_ingress_port_id)
            if hop.l1_egress_port_id:
                flow.l1_egress_port_ids.append(hop.l1_egress_port_id)
            if hop.hop_latency:
                flow.hop_latencies.append(hop.hop_latency)
            if hop.q_id:
                flow.q_ids.append(hop.q_id)
                flow.q_occups.append(hop.q_occupancy)
            if hop.ingress_tstamp:
                flow.ingress_tstamps.append(hop.ingress_tstamp)
            if hop.egress_tstamp:
                flow.egress_tstamps.append(hop.egress_tstamp)
            if hop.l2_ingress_port_id:
                flow.l2_ingress_port_ids.append(hop.l2_ingress_port_id)
                flow.l2_egress_port_ids.append(hop.l2_egress_port_id)
            if hop.egress_port_tx_util:
                flow.egress_port_tx_utils.append(hop.egress_port_tx_util)

        return flow

class Collector():

    def __init__(self):
        self.flow_table = {}

HOST = ''
PORT = 9555

def receiver():
    # temporary
    collector = Collector()
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.bind((HOST, PORT))
        try:
            while True:
                data, addr = s.recvfrom(512)
                print("-- Received report from {} --------".format(addr))
                rep = Report(data)
                print(rep)
                # TODO update collector data
                new_flow = FlowInfo.from_report(rep)
                collector.flow_table[new_flow.flow_id] = new_flow
                print(collector.flow_table)
        except KeyboardInterrupt:
            s.close()

if __name__ == "__main__":
    receiver()