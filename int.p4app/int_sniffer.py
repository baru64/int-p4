import sys

from scapy.all import sniff, raw
from scapy.all import IP, TCP, UDP
from scapy.all import Packet, BitField, ByteField, ShortField, bind_layers

class INT(Packet):
    name = "int"
    fields_desc=[
            BitField("version",1,4),
            BitField("rep",0,2),
            BitField("c",0,1),
            BitField("e",0,1),
            BitField("m",0,1),
            BitField("reserved1",0,10),
            BitField("hop_ml", None, 5),
            ByteField("rem_hop_cnt", None),
            ShortField("ins_map", 0),
            ShortField("reserved2", 0),
            ]

class INT_shim(Packet):
    name = "int shim"
    fields_desc=[
            ByteField("type",1),
            ByteField("reserved1",1),
            ByteField("len",3),
            BitField("dscp",0, 6),
            BitField("reserved2", 0, 2)
            ]

def handle_pkt(pkt):
    if (pkt[IP].tos >> 2) == 0x17:
        int_pkt = None
        if UDP in pkt:
            int_pkt = INT_shim(raw(pkt[UDP].payload))
        elif TCP in pkt:
            int_pkt = INT_shim(raw(pkt[TCP].payload))
        int_pkt.show()
        int_words = memoryview(int_pkt.load[:int_pkt[INT_shim].len]).cast("I")
        for i, word in enumerate(int_words):
            if i % int_pkt[INT].hop_ml:
                print("=== HOP ===")
            print('{:#034b}'.format(word))


def main(iface):
    sniff(filter="tcp or udp", iface=iface, prn = lambda pkt: handle_pkt(pkt))

if __name__ == "__main__":
    bind_layers(INT_shim, INT)
    if len(sys.argv) < 2:
        print("usage:\t\t", sys.argv[0], "INTERFACE")
    else:
        main(sys.argv[1])
