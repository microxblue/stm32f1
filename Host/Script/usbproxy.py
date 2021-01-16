import os
import sys
import time
import usb.core
import usb.util
import socket
import queue
from threading import Thread
from stmtalk import KbHit, STM32_USB_DEV, bytes_to_value, value_to_bytes, print_bytes


# A class that extends the Thread class
class UsbPipeThread(Thread):

    def __init__(self, rx_qu, tx_qu, quit, inf = 0):
        Thread.__init__(self)
        self.rx_qu = rx_qu
        self.tx_qu = tx_qu
        self.quit  = quit
        self.inf   = inf

    def run(self):
        while not self.quit[0]:
            try:
                self.stm_usb = STM32_USB_DEV('', STM32_USB_DEV.MY_PID2, self.inf)
            except:
                self.stm_usb = None

            if self.stm_usb is None or self.stm_usb.epout is None or self.stm_usb.epin is None:
                time.sleep (3)
                continue

            print('=> USB started running (inf:%d)' % self.inf)
            while not self.quit[0]:
                try:
                    rx_buf = self.stm_usb.read(64)
                except usb.USBError as e:
                    err_str = repr(e)
                    if ('timeout error' in err_str) or ('timed out' in err_str):
                        rx_buf = b''
                    else:
                        raise

                if len (rx_buf) > 0:
                    if self.tx_qu.full():
                        self.tx_qu.get ()
                    tx_data = bytearray(rx_buf)
                    #print ("USB RX: %s" % tx_data.decode())
                    self.tx_qu.put (tx_data)

                if not self.rx_qu.empty():
                    rx_data = self.rx_qu.get()
                    #print ("USB TX: %s" % rx_data.decode())
                    try:
                        self.stm_usb.write(rx_data, 10)
                    except:
                        pass

            print('=> USB stopped running (inf:%d)' % self.inf)

            time.sleep (.3)

        if self.stm_usb is not None:
            self.stm_usb.close()

        print ("=> USB thread quit (inf:%d)" % self.inf)



class TcpThread(Thread):

    MAX_PKT = 1024

    def __init__(self, rx_qu, tx_qu, quit, host, port, stream = True):
        Thread.__init__(self)
        self.rx_qu  = rx_qu
        self.tx_qu  = tx_qu
        self.quit   = quit
        self.port   = port
        self.stream = stream

        self.socket = socket.socket()  # get instance
        self.socket.setsockopt (socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        # look closely. The bind() function takes tuple as argument
        self.socket.bind((host, port))  # bind host address and port together

        # configure how many client the server can listen simultaneously
        self.socket.listen(1)

    def run (self):
        while not self.quit[0]:
            self.socket.settimeout (.3)
            try:
                conn, address = self.socket.accept()  # accept new connection
            except socket.timeout as e:
                continue

            conn.setblocking(True)
            conn.settimeout(.05)

            rx_buf = b''
            print("=> Connection created with (%s:%d)" % (repr(address[0]), self.port))
            while not self.quit[0]:
                # receive data stream. it won't accept data packet greater than 1024 bytes
                try:
                    tmp_buf = conn.recv(TcpThread.MAX_PKT)
                    if len (tmp_buf) == 0:
                        break
                except socket.timeout as e:
                    tmp_buf = b''
                except:
                    break
                if len(tmp_buf) > 0:
                    rx_buf = rx_buf + tmp_buf

                if len(rx_buf) > 0:
                    if 0:
                        print ('TCP RX:', self.port)
                        print_bytes (rx_buf, 2)

                    if not self.stream:
                        if rx_buf[0:2] != b'$P':
                            print("Unexpected packet header received !")
                            pkt_buf = b''
                            rx_buf  = b''
                        else:
                            pkt_len = bytes_to_value(rx_buf[2:4])
                            pkt_buf = bytearray(rx_buf[4:4+pkt_len])
                            rx_buf  = rx_buf[4+pkt_len:]
                    else:
                        pkt_buf = bytearray(rx_buf)
                        rx_buf  = b''

                    if len(pkt_buf) > 0:
                        if self.rx_qu.full():
                            self.rx_qu.get ()

                        rx_data = pkt_buf
                        self.rx_qu.put (rx_data)

                if not self.tx_qu.empty():
                    tx_data = self.tx_qu.get()

                    if not self.stream:
                        tx_data = bytearray(b'$P') + value_to_bytes(len(tx_data) & 0xffff,2) + tx_data

                    if 0:
                        print ('TCP TX:', self.port)
                        print_bytes (tx_data, 2)

                    try:
                        conn.send(tx_data)
                    except:
                        pass

            conn.close()  # close the connection

            print("=> Connection closed with (%s:%d)" % (repr(address[0]), self.port))

        print ("=> TCP thread quit (port:%d)" % self.port)

class  UsbProxy:
    def __init__ (self, host):
        self.tx_qu0 = queue.Queue(64)
        self.rx_qu0 = queue.Queue(64)
        self.tx_qu1 = queue.Queue(64)
        self.rx_qu1 = queue.Queue(64)

        self.quit  = [0]
        self.usb_thread0 = UsbPipeThread(self.tx_qu0, self.rx_qu0, self.quit, 0)
        self.usb_thread1 = UsbPipeThread(self.tx_qu1, self.rx_qu1, self.quit, 1)
        self.tcp_thread0 = TcpThread(self.tx_qu0, self.rx_qu0, self.quit, host, 8888)
        self.tcp_thread1 = TcpThread(self.tx_qu1, self.rx_qu1, self.quit, host, 8800, False)

    def start (self):
        self.usb_thread0.start()
        self.usb_thread1.start()
        self.tcp_thread0.start()
        self.tcp_thread1.start()

    def stop (self):
        self.quit[0] = 1

if len(sys.argv) > 1:
    addr = sys.argv[1]
else:
    addr = 'localhost'

kb    = KbHit()
proxy = UsbProxy (addr)
proxy.start()
while True:
    if kb.kbhit():
        break
    time.sleep (.5)
proxy.stop ()

