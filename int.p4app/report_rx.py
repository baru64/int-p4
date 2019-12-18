import socket

OFFSET = 14 + 20 + 8

class report():
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
        # int shim
        self.int_shim = data[OFFSET + 16:OFFSET + 20]
        # int header
        self.int_hdr = data[OFFSET + 20:OFFSET + 28]
        # int metadata
        self.int_meta = data[OFFSET + 28:]

    def __repr__(self):
        return "sw: {} seq: {} tstamp: {} \n {} \n {} \n {}".format(
            self.switch_id,
            self.seq_num,
            self.ingress_tstamp,
            self.int_shim,
            self.int_hdr,
            self.int_meta
        )

HOST = ''
PORT = 9555

with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
    s.bind((HOST, PORT))
    try:
        while True:
            data, addr = s.recvfrom(512)
            print("-- Received report from {} --------".format(addr))
            rep = report(data)
            print(rep)
    except KeyboardInterrupt as e:
        s.close()
