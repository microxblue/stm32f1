import os
import sys
import time
import argparse
import usb.core
import usb.util
import string
import socket
import binascii

if os.name == 'nt':
    # Windows
    import msvcrt
else:
    # Posix (Linux, OS X)
    import termios
    import atexit
    from select import select

def value_to_bytes (value, length):
    return value.to_bytes(length, 'little')

def bytes_to_value (bytes):
    return int.from_bytes (bytes, 'little')

def print_bytes (data, indent=0, offset=0, show_ascii = False):
    bytes_per_line = 16
    printable = ' ' + string.ascii_letters + string.digits + string.punctuation
    str_fmt = '{:s}{:04x}: {:%ds} {:s}' % (bytes_per_line * 3)
    bytes_per_line
    data_array = bytearray(data)
    for idx in range(0, len(data_array), bytes_per_line):
        hex_str = ' '.join('%02X' % val for val in data_array[idx:idx + bytes_per_line])
        asc_str = ''.join('%c' % (val if (chr(val) in printable) else '.')
                          for val in data_array[idx:idx + bytes_per_line])
        print (str_fmt.format(indent * ' ', offset + idx, hex_str, ' ' + asc_str if show_ascii else ''))

def log2 (x):
    return x.bit_length() - 1

def crc32 (data) :
     crc = 0xFFFFFFFF
     for each in data:
         crc = crc ^ each
         for i in range(8):
             crc = (crc >> 1) ^ (0xEDB88320 & ((-(crc & 1)) & 0xffffffff))
     return ~crc & 0xffffffff

def flags_eval (flags, expr):
    new_expr = []
    for each in expr:
        if each.isalpha():
            new_expr.append ('1' if each in flags else '0')
        else:
            new_expr.append (each)
    result = True if (eval (''.join(new_expr)) & 1) else False
    return result

class KbHit:
    def __init__(self):
        '''Creates a KBHit object that you can call to do various keyboard things.
        '''

        if os.name == 'nt':
            pass

        else:

            # Save the terminal settings
            self.fd = sys.stdin.fileno()
            self.new_term = termios.tcgetattr(self.fd)
            self.old_term = termios.tcgetattr(self.fd)

            # New terminal setting unbuffered
            self.new_term[3] = (self.new_term[3] & ~termios.ICANON &
                                ~termios.ECHO)
            termios.tcsetattr(self.fd, termios.TCSAFLUSH, self.new_term)

            # Support normal-terminal reset at exit
            atexit.register(self.set_normal_term)

    def set_normal_term(self):
        ''' Resets to normal terminal.  On Windows this is a no-op.
        '''

        if os.name == 'nt':
            pass

        else:
            termios.tcsetattr(self.fd, termios.TCSAFLUSH, self.old_term)

    def getch(self):
        ''' Returns a keyboard character after kbhit() has been called.
            Should not be called in the same program as getarrow().
        '''
        if os.name == 'nt':
            ch = msvcrt.getch()
            if ch in b'\x00\xe0':
                # Function key or arrow key
                ch2 = msvcrt.getch()
                if ch == b'\xe0':
                    conv = b'HPKM'
                    if ch2 in conv:
                        ch = b'\x1b[%c' % (ord('A') + conv.index(ch2))
            else:
                ch = ch.decode('utf-8')
            return ch
        else:
            return sys.stdin.read(1)

    def getarrow(self):
        ''' Returns an arrow-key code after kbhit() has been called. Codes are
        0 : up
        1 : right
        2 : down
        3 : left
        Should not be called in the same program as getch().
        '''

        if os.name == 'nt':
            msvcrt.getch()  # skip 0xE0
            c = msvcrt.getch()
            vals = [72, 77, 80, 75]

        else:
            c = sys.stdin.read(3)[2]
            vals = [65, 67, 66, 68]

        return vals.index(ord(c.decode('utf-8')))

    def kbhit(self):
        ''' Returns True if keyboard character was hit, False otherwise.
        '''
        if os.name == 'nt':
            return msvcrt.kbhit()

        else:
            dr, dw, de = select([sys.stdin], [], [], 0)
            return dr != []


class STM32_NET_DEV:

    DEF_TIMEOUT = .3
    MAX_PKT     = 1024
    DEV_MAX_PKT = 64

    def __init__(self, devaddr, product, interface = 0):
        # find our device
        self.port   = 8888 if interface == 0 else 8800
        self.stream = True if interface == 0 else False
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt (socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.setsockopt (socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.socket.settimeout (STM32_NET_DEV.DEF_TIMEOUT)
        self.socket.connect((devaddr, self.port))
        self.rx_buf = b''

    def read (self, length = MAX_PKT, timeout = DEF_TIMEOUT):
        if timeout != STM32_NET_DEV.DEF_TIMEOUT:
            self.socket.settimeout (timeout * 1.0 / 1000)
        if length == STM32_NET_DEV.DEV_MAX_PKT and not self.stream:
            length = length + 4
        try:
            data = self.socket.recv (length)
        except socket.timeout as e:
            data = b''
        except:
            raise SystemExit ('\n%s' % repr(e))

        self.rx_buf = self.rx_buf + data
        pkt_buf = b''
        if len(self.rx_buf) > 0:
            if not self.stream:
                if self.rx_buf[0:2] != b'$P':
                    print("Unexpected packet header received !")
                    pkt_buf = b''
                    self.rx_buf  = b''
                else:
                    pkt_len = bytes_to_value(self.rx_buf[2:4])
                    pkt_buf = bytearray(self.rx_buf[4:4+pkt_len])
                    self.rx_buf = self.rx_buf[4+pkt_len:]
            else:
                pkt_buf = bytearray(self.rx_buf)
                self.rx_buf = b''

        if 0 and len(pkt_buf):
            print ('RX:', self.port)
            print_bytes (pkt_buf, 2)

        if timeout != STM32_NET_DEV.DEF_TIMEOUT:
            self.socket.settimeout (STM32_NET_DEV.DEF_TIMEOUT)

        return pkt_buf

    def write (self, data, timeout = DEF_TIMEOUT):
        if timeout != STM32_NET_DEV.DEF_TIMEOUT:
            self.socket.settimeout (timeout * 1.0 / 1000)

        if not self.stream:
            data = bytearray(b'$P') + value_to_bytes(len(data) & 0xffff,2) + data

        if 0 and len(data):
            print ('TX:', self.port)
            print_bytes (data, 2)

        try:
            ret = self.socket.send (data)
            if not self.stream and ret > 4:
                ret -= 4
        except socket.timeout as e:
            ret = 0
        except:
            raise SystemExit ('\n%s' % repr(e))

        if timeout != STM32_NET_DEV.DEF_TIMEOUT:
            self.socket.settimeout (STM32_NET_DEV.DEF_TIMEOUT)

        return ret

    def __del__ (self):
        self.socket.close()


class STM32_USB_DEV:

    MAX_PKT = 64

    MY_VID  = 0x0686
    MY_PID  = 0x1023
    MY_PID2 = 0x0925
    MY_PID3 = 0x0918

    def __init__(self, devaddr, product,  interface = 0):
        # find our device
        self.dev    = None
        self.infidx = None
        self.epout  = None
        self.epin   = None

        devs = usb.core.find(idVendor=STM32_USB_DEV.MY_VID,
                             idProduct=product,
                             find_all=True)

        if interface:
            infnum = 1
        else:
            infnum = 0

        tgts = []
        for dev in devs:
            cfg = next(iter(dev), None)
            infidx = 0
            for inf in iter(cfg):
                if inf.bInterfaceNumber == infnum:
                    tgts.append((dev, infidx))
                infidx = infidx + 1

        if len(tgts) > 0:
            if devaddr == '?':
                # list all devices
                for idx, tgt in enumerate(tgts):
                    print ('Device %d (%04X:%04X) at address %d' % (idx, tgt[0].idVendor, tgt[0].idProduct, tgt[0].address))
                sys.exit (-1)
            elif devaddr == '':
                self.dev, self.infidx = tgts[0]
            else:
                if devaddr.startswith('0x'):
                    addr = int(devaddr, 16)
                else:
                    addr = int(devaddr)
                for dev, infidx in tgts:
                    if dev.address == addr:
                        self.dev = dev
                        self.infidx = infidx

        if self.dev is None:
            print  ('Cannot find matched StmUsb device!')
            return

        # set the active configuration. With no arguments, the first
        # configuration will be the active one
        if sys.platform == "win32":
            self.dev.set_configuration()

        # get an endpoint instance
        self.cfg  = self.dev.get_active_configuration()
        self.intf = self.cfg[(self.infidx, 0)]

        self.epout = usb.util.find_descriptor(
            self.intf,
            # match the first OUT endpoint
            custom_match=
            lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT)

        self.epin = usb.util.find_descriptor(
            self.intf,
            # match the first OUT endpoint
            custom_match=
            lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN)

        if self.epout is None or self.epin is None:
            print ('Cannot find End Point!')
            return

    def read (self, length = MAX_PKT, timeout = 100):
        try:
            data = self.dev.read(self.epin, length, timeout)
        except usb.USBError as e:
            err_str = repr(e)
            if ('timeout error' in err_str) or ('timed out' in err_str):
                data = b''
            else:
                raise SystemExit ('\n%s' % repr(e))
        return data

    def write (self, data, timeout = 100):
        try:
            ret = self.dev.write(self.epout, data)
        except usb.USBError as e:
            if 'timeout error' in repr(e):
                ret = 0
            else:
                raise SystemExit ('\n%s' % repr(e))
        return len(data)

    def close (self):
        usb.util.dispose_resources(self.dev)
        self.dev.reset()


class STM32_CON:

    def __init__(self, devaddr='', product=0x1023):
        if product == 0:
            self.stm_usb = STM32_NET_DEV(devaddr, product, 0)
        else:
            self.stm_usb = STM32_USB_DEV(devaddr, product, 0)



    def console_drain (self):
        res = b' '
        while len(res) > 0:
            res = self.stm_usb.read(STM32_USB_DEV.MAX_PKT)

    def console_run_cmd (self, cmd, timeout = 1.0):
        self.console_drain ()
        self.stm_usb.write (cmd.encode() + b'\n')

        text  = []
        start = time.time()

        while (timeout == -1) or (time.time() - start < timeout):
            res = self.stm_usb.read(STM32_USB_DEV.MAX_PKT)
            if len(res) > 0:
                part = bytearray(res).decode()
                text.append(part)
                if part[-1] == '>':
                  break

        text.append('\n')
        resp = ''.join(text)

        lines = resp.splitlines()
        start_idx = 0
        for idx, line in enumerate (lines):
            if line.startswith(cmd):
                start_idx = idx + 1
        end_idx = len(lines)
        while len(lines) > 0 and lines[-1] in ['>', '']:
            del lines[-1]

        return lines[start_idx:end_idx]


    def console (self):
        self.stm_usb.read(STM32_USB_DEV.MAX_PKT)
        sys.stdout.write('>')
        sys.stdout.flush()

        prep_quit = False
        kb = KbHit()
        while True:
            outputs = self.stm_usb.read(STM32_USB_DEV.MAX_PKT)
            if len(outputs):
                sys.stdout.write(bytearray(outputs).decode())
                sys.stdout.flush()

            if kb.kbhit():
                keys = kb.getch()
                if len(keys) == 1 and keys[0] == '\x1b':
                    try:
                        self.stm_usb.write('\n')
                        self.stm_usb.read(STM32_USB_DEV.MAX_PKT)
                    except:
                        pass
                    break
                else:
                    self.stm_usb.write(keys.encode())

        sys.stdout.write('\n\n')
        sys.stdout.flush()


class STM32_COMM:

    USB_WR_TIMEOUT   = 1000

    CMD_PKT_LEN      = 12
    MAX_LINE_LEN     = 32

    # target
    TARGET_DUMMY     = 0
    TARGET_IROM      = 1
    TARGET_SRAM      = 2
    TARGET_FLASH     = 3
    TARGET_DEDIPROG  = 4
    TARGET_MAX       = 5

    # command
    CMD_NOP = 0x00
    CMD_SET_ADDR_LEN = 0x01
    CMD_ERASE_BLOCK = 0x02
    CMD_SEND_DONE = 0x03
    CMD_READ_BLOCK = 0x04
    CMD_SKIP_PAGE = 0x05
    CMD_CSUM_BLOCK = 0x06
    CMD_GET_STATUS = 0x07
    CMD_CHECK = 0xAA
    CMD_INVALID = 0xFF

    TARGET_CFGS = {
      0: {'blocksize':0x00010000, 'pagesize':0x00000100, 'base':0x00000000, 'limit':0x80000000, 'progcmd':"usbt"},  # DUMMY
      1: {'blocksize':0x00000400, 'pagesize':0x00000040, 'base':0x08000000, 'limit':0x08020000, 'progcmd':"fp  "},  # IROM
      2: {'blocksize':0x00000100, 'pagesize':0x00000100, 'base':0x20000000, 'limit':0x20020000, 'progcmd':"dl 0"},  # SRAM
      3: {'blocksize':0x00010000, 'pagesize':0x00000100, 'base':0x00000000, 'limit':0x00800000, 'progcmd':"dl 1"},  # SPI FLASH
      4: {'blocksize':0x00010000, 'pagesize':0x00010000, 'base':0x00000000, 'limit':0x01000000, 'progcmd':"dprg"},  # DEDIPROG
    }

    def __init__(self, devaddr='', product=0x1023):
        if product == 0:
            self.stm_usb = STM32_NET_DEV(devaddr, product, 1)
        else:
            self.stm_usb = STM32_USB_DEV(devaddr, product, 1)

    def drain(self):
        # Drain all data
        res = b' '
        while len(res) > 0:
            res = self.stm_usb.read(STM32_USB_DEV.MAX_PKT)
        self.short_cmd(STM32_COMM.CMD_SEND_DONE, 0, 0, 0)
        self.short_cmd(STM32_COMM.CMD_SEND_DONE, 0, 0, 0)
        time.sleep(0.1)

    def shell_cmd(self, cmd):
        cmdbuf = bytearray(cmd.encode())
        if len(cmdbuf) < STM32_COMM.MAX_LINE_LEN:
            cmdbuf.extend(bytearray(b'\x00' * (STM32_COMM.MAX_LINE_LEN - len(cmdbuf))))
        ret = self.stm_usb.write(cmdbuf)
        if ret != len(cmdbuf):
            return 1
        else:
            return 0

    def short_cmd(self, cmd, flag, param0, param1):
        cmdbuf = bytearray(b'\x00' * STM32_COMM.CMD_PKT_LEN)
        cmdbuf[0] = ord('@')
        cmdbuf[3] = flag & 0xFF
        cmdbuf[4:8]  = bytearray(value_to_bytes(param0, 4))
        cmdbuf[8:12] = bytearray(value_to_bytes(param1, 4))
        if (cmd == STM32_COMM.CMD_SET_ADDR_LEN):  # Change address & length
            cmdbuf[1] = ord('A')
        elif (cmd == STM32_COMM.CMD_ERASE_BLOCK): # Erase block
            cmdbuf[1] = ord('E')
        elif (cmd == STM32_COMM.CMD_SEND_DONE):   # Done
            cmdbuf[1] = ord('D')
        elif (cmd == STM32_COMM.CMD_READ_BLOCK):  # Read
            cmdbuf[1] = ord('R')
        elif (cmd == STM32_COMM.CMD_SKIP_PAGE):   # Skip page
            cmdbuf[1] = ord('S')
        elif (cmd == STM32_COMM.CMD_CSUM_BLOCK):  # Sum
            cmdbuf[1] = ord('C')
        elif (cmd == STM32_COMM.CMD_NOP):         # Nop
            cmdbuf[1] = ord('N')
        elif (cmd == STM32_COMM.CMD_GET_STATUS):  # Status
            cmdbuf[1] = ord('G')
        elif (cmd == STM32_COMM.CMD_CHECK):       # Check, if it is in Shell prompt
            cmdbuf[1] = ord('H')
        else:  # Invalid
            cmdbuf[0] = 0x00
        ret = self.stm_usb.write(cmdbuf)
        if ret != len(cmdbuf):
            return 1
        else:
            return 0

    def check_result (self):
        self.short_cmd  (STM32_COMM.CMD_CHECK, 0x10, 0, 0)
        sts = self.stm_usb.read(STM32_USB_DEV.MAX_PKT, 1000)
        if len(sts) != STM32_USB_DEV.MAX_PKT:
            return bytearray ()
        else:
            sts = bytearray (sts)
            if sts[0:4] == b'MSTS':
                # The old response is two packets, read one more, but ignore it
                self.stm_usb.read(STM32_USB_DEV.MAX_PKT)
            return sts

    def get_status (self):
        if self.short_cmd (STM32_COMM.CMD_GET_STATUS, 0, 0, 0):
            return 1
        else:
            return 0

    def send_done (self, param = 0):
        if self.short_cmd  (STM32_COMM.CMD_SEND_DONE, 0, param, 0) :
            raise SystemExit ("ERR: failed to send the EOP")


    def speed_test (self, plen, test_time):
        if (self.shell_cmd  ("@usbt 0")) :
            raise SystemExit ("ERR: failed to send shell command !")
        time.sleep(.1)

        test_time = 2.0

        loop  = 0
        tick1 = time.time()
        while time.time() - tick1 < test_time:
            ret = self.stm_usb.write(b'\xff' * plen, STM32_COMM.USB_WR_TIMEOUT)
            if ret != plen:
                break
            loop += 1

        tick2 = time.time()
        self.short_cmd  (STM32_COMM.CMD_SEND_DONE, 0, 0, 0)
        if (ret != plen) :
            print ("USB write test failed!")
        else :
            print ("USB write test speed %8.2f KB/S!" % (plen * loop / 1000.0 / (tick2 - tick1)))

        time.sleep(0.1)
        if (self.shell_cmd  ("@usbt 1")) :
            raise SystemExit ("ERR: failed to send shell command")

        time.sleep(0.1)

        loop  = 0
        tick1 = time.time()
        while time.time() - tick1 < test_time:
            data = self.stm_usb.read(plen, STM32_COMM.USB_WR_TIMEOUT)
            if (len(data) != plen) :
                 break
            loop += 1
        tick2 = time.time()

        if (len(data) != plen) :
            print ("USB read test failed!")
        else :
            print ("USB read  test speed %8.2f KB/S!" % (plen * loop / 1000.0 / (tick2 - tick1)))

        self.short_cmd  (STM32_COMM.CMD_SEND_DONE, 0, 0, 0)


def update_dev (remain):
    usbaddr = ''
    pid     = STM32_USB_DEV.MY_PID

    remain = remain.strip()
    if remain.startswith('.'):
        if len(remain) > 1:
            if   remain[1] == '@':
                pid = STM32_USB_DEV.MY_PID2
            elif remain[1] == '!':
                pid = STM32_USB_DEV.MY_PID3
            elif remain[1:].startswith('0x'):
                pid = int(remain[1:], 16)
            else:
                pid = 0
                usbaddr = remain[1:].strip()
    elif len(remain) > 0:
        usbaddr = remain
    return usbaddr, pid


def usage ():
    print ("  STMTALK  FileName  Address[:Length]  Target[:UsbAddress]  [Options]");
    print ("  STMTALK  \"Shell Command\" [UsbAddress] \n");
    print ("    Target\n"
            "      SRAM | IROM | FLASH | DEDIPROG\n");
    print ("    Options\n"
            "      r - Read flash\n"
            "      v - Verify flash\n"
            "      b - Blank check\n"
            "      o - Create output file\n"
            "      c - Calculate CRC\n"
            "      f - Force without CRC\n"
            "      e - Erase flash\n"
            "      m - Module mode\n"
            "      n - Use network interface\n"
            "      p - Program flash\n"
            "      u - Use module test command\n"
            "      x - Use Flash module command\n"
            "      t - Speed test mode\n"
            "      s - Select 2nd flash\n"
            "      0,1,2 - Select voltage (0: 3.3v  1:1.8v  2: 2.5v)\n"
            );
    print ("    UsbAddress\n"
            "      .  - Select 1st device\n"
            "      N  - Select device with USB address N\n"
            "      ?  - List all devices\n"
            "      .@ - Select device of PID2 (0x0925), \n"
            "      .! - Select device of PID3 (0x0918), \n"
            "      .0x???? - Select device PID specified \n"
            );


def main():

    options = ''
    usbaddr, pid = update_dev ('')

    argc = len(sys.argv)
    if argc == 2 or argc == 3:
        # command string
        if argc == 3:
            usbaddr, pid = update_dev (sys.argv[2])
    else:
        if (argc < 4 or argc > 5) :
            usage ()
            return 0

        elif (argc == 5) :
            options = sys.argv[4]
            # t is exclusive
            if 't' in options and len(options) > 1 :
                raise SystemExit ("ERR: invalid options '%s' combination!" % options)

        chipcs  = 0
        tgtstr  = sys.argv[3].upper()
        if ':' in tgtstr:
            pos    = tgtstr.index(':')
            usbaddr, pid = update_dev (tgtstr[pos+1:])
            tgtstr = tgtstr[:pos]
        if tgtstr == "FLASH" :
            target = STM32_COMM.TARGET_FLASH
        elif tgtstr == "IROM" :
            target = STM32_COMM.TARGET_IROM
        elif tgtstr == "SRAM" :
            target = STM32_COMM.TARGET_SRAM
        elif tgtstr == "DEDIPROG":
            target = STM32_COMM.TARGET_DEDIPROG
            if 's' in options:
                chipcs = 1
        else :
            raise SystemExit ("ERR: Invalid target %s!" % sys.argv[3])

        if 'x' in options :
            cmdstr = "sm 80 00"
            if '2' in options:
                cmdstr = cmdstr + "; mt 2"
            elif '1' in options:
                cmdstr = cmdstr + "; mt 1"
            elif '0' in options:
                cmdstr = cmdstr + "; mt 0"
            else :
                cmdstr = cmdstr + "; mt"

        elif 'u' in options :
            cmdstr = "mt"
        else :
            cmdstr = STM32_COMM.TARGET_CFGS[target]['progcmd']

        if chipcs == 1 and target == TARGET_DEDIPROG :
            strcat(cmdstr,  " 1")

        location = sys.argv[2]
        parts = location.split(':')
        if len(parts) > 1:
            address  = int(parts[0], 16)
            length   = int(parts[1], 16)
        else :
            address  = int(parts[0], 16)
            length   = 0

        if flags_eval (options, "r | t | b | (e & ~p)"):
            if length == 0:
                length = 0x10000
            size = length
        else :
            size = os.path.getsize(sys.argv[1])
            if length == 0:
                length = size
            if length < size :
                size = length

        blksize  = STM32_COMM.TARGET_CFGS[target]['blocksize']
        pagesize = STM32_COMM.TARGET_CFGS[target]['pagesize']
        if 'r' not in options:
            address = address & ~(blksize - 1) & 0xffffffff

        if address < STM32_COMM.TARGET_CFGS[target]['base'] or address >= STM32_COMM.TARGET_CFGS[target]['limit'] :
            raise SystemExit ("ERR: Base address is not within the %s range!\n", argv[3])


        if (address + size) < STM32_COMM.TARGET_CFGS[target]['base'] or (address + size) > STM32_COMM.TARGET_CFGS[target]['limit'] :
            raise SystemExit ("ERR: Length is too big to fit into the %s range!\n", argv[3])

    stm_comm = STM32_COMM (usbaddr, pid)
    stm_comm.drain ()

    # Check connection, ask to send back 0x80 bytes
    result = stm_comm.check_result ()
    if len(result) != STM32_USB_DEV.MAX_PKT:
        raise SystemExit ("ERR: could not get status packet from target!")

    if (argc == 2) or (argc == 3):
        # @ : Do not display anything on USB console
        # ! : Display output on USB console
        # - : Do not display anything on USB console and do not send response packet back for the command
        sh_cmd = sys.argv[1].strip()
        if len(sh_cmd) > 1 and sh_cmd[0] in '@!-':
            if sh_cmd[0] == '-':
                # Use '-' to skip waiting for a response packet
                sh_cmd = sh_cmd[1:]
                echo = False
            else:
                echo = True

            if stm_comm.shell_cmd (sh_cmd):
                raise SystemExit ("ERR: failed to send shell command !")

            if echo:
                result = stm_comm.check_result ()
                if len(result) != STM32_USB_DEV.MAX_PKT:
                    print ('Command failed !')
                else:
                    print ('%08x %08x' % (bytes_to_value(result[16:20]), bytes_to_value(result[20:24])))
        else:
            stm_con = STM32_CON (usbaddr, pid)

            if len(sh_cmd) == 0:
                stm_con.console ()
            else:
                outputs = stm_con.console_run_cmd (sh_cmd)
                print ('\n'.join(outputs))

        return 0

    if 't' in options :
        stm_comm.speed_test (0x100000, 2.0)
        return 0

    if (stm_comm.shell_cmd  (cmdstr)) :
        raise SystemExit ("ERR: failed to send shell command !")

    time.sleep(.05)

    #
    # Check target status by sending a PING
    #
    if flags_eval (options, "r | e | p | c"):
        if target == STM32_COMM.TARGET_DEDIPROG:
            if stm_comm.short_cmd  (STM32_COMM.CMD_GET_STATUS, 0, 0, 0) :
                raise SystemExit ("ERR: failed to get status\n")

            tmp = stm_comm.stm_usb.read(STM32_USB_DEV.MAX_PKT)
            tmp = stm_comm.stm_usb.read(STM32_USB_DEV.MAX_PKT)
            if len(tmp)  !=  STM32_USB_DEV.MAX_PKT:
                raise SystemExit ("ERR: bulk read status failed !")
            elif tmp[0] == 0xFF or (tmp[0] == 0x00 and tmp[1] == 0x00):
                raise SystemExit ("ERR: target status is not ready, no flash detected !")

    fpo = None
    if flags_eval (options, "r & o & ~b"):
        filename = sys.argv[1]
        try:
            fpo = open(filename, "wb")
        except:
            raise SystemExit ("ERR: Can not create file %s!" % filename)

    if flags_eval (options, "r | b | v"):
        idx  = 0
        tlen = (length + 0xFF) & 0xFFFFFF00
        while (tlen) :
            if (tlen > 0x10000) :
                rlen = 0x10000
            else :
                rlen = tlen

            if (stm_comm.short_cmd  (STM32_COMM.CMD_READ_BLOCK, 0, address, rlen)) :
                raise SystemExit ("ERR: failed to read flash !")

            resp = stm_comm.stm_usb.read(rlen, 2000)
            if len(resp) != rlen:
                raise SystemExit ("ERR: bulk read failed !")

            if fpo :
                fpo.write (resp)
            else :
                if 'b' in options:
                    for idx in range(rlen):
                        if (resp[idx] != 0xFF) :
                            print ("%08X: %02X" % (address + idx, resp[idx]))
                            tlen = rlen
                            break

                elif 'v' in options : # Verify
                    fpi  = open(sys.argv[1], 'rb')
                    for idx in range(rlen):
                        cb = fpi.read(1)
                        if len(cb) == 0:
                            idx  = rlen
                            tlen = rlen
                            break

                        cb = ord(cb)
                        if resp[idx] != cb:
                            print ("%08X: %02X(Flash) %02X(File)" % (address + idx, resp[idx], cb))
                            tlen = rlen
                            break
                    fpi.close ()
                else :  # Read
                    for idx in range(rlen):
                        if (idx & 0x0F) == 0x00:
                            print ("%08X:" % (address + idx), end='')
                        print (" %02X" % resp[idx], end='')
                        if (idx & 0x0F) == 0x0F:
                            print ('')

            address += rlen
            tlen    -= rlen

        if 'b' in options:
            if idx + 1 == rlen:
                print ("Device is BLANK !")
            else :
                print ("Device is NOT BLANK !")

        elif 'v' in options:
            if idx + 1 == rlen:
                print ("Flash and File contents are identical !")
            else :
                print ("Flash and File contents are different !")

        stm_comm.send_done ()
        return 0

    blknum = (size + blksize - 1) // blksize
    blk    = bytearray(blknum)
    if target == STM32_COMM.TARGET_DEDIPROG:
        # Calulate CRC in file
        print("Calculating...")
        if flags_eval (options, "(e | p | c) & ~f"):
            sum2 = []
            fpi = open(sys.argv[1], 'rb')
            for idx in range (blknum):
                buf2 = fpi.read (blksize)
                if len(buf2) < blksize:
                    buf2 = buf2 + b'\xff' * (blksize - len(buf2))
                sum2.append (binascii.crc32 (buf2))
            fpi.close()

            # Calculate check sum in flash
            if (stm_comm.short_cmd  (STM32_COMM.CMD_CSUM_BLOCK, log2(blksize), address, blknum * blksize)) :
                raise SystemExit ("ERR: failed to caculate checksum !")

            sum1 = []
            for idx in range ((blknum * 4 + STM32_USB_DEV.MAX_PKT * 2 - 1) // (STM32_USB_DEV.MAX_PKT * 2)):
                sumx = stm_comm.stm_usb.read(STM32_USB_DEV.MAX_PKT, 15000)
                sumx = sumx + stm_comm.stm_usb.read(STM32_USB_DEV.MAX_PKT, 15000)
                if len(sumx) != STM32_USB_DEV.MAX_PKT * 2:
                    raise SystemExit ("ERR: bulk read checksum failed !")
                for i in range(0, len(sumx), 4):
                    sum1.append (bytes_to_value(sumx[i:i+4]))

            blk = bytearray(blknum)
            for idx in range (blknum):
                if  (sum1[idx] == sum2[idx]):
                    blk[idx] = 2   # Same
                elif (sum1[idx] == 0xDEAB7E4E):
                    blk[idx] = 1   # Empty
                else:
                    blk[idx] = 0

            if 'c' in options:
                for idx in range (blknum):
                    if  (blk[idx] == 2) :
                        result = "Identical"
                    elif (blk[idx] == 1) :
                        result = "Empty"
                    else :
                        result = "Different"
                    print ("Block %3d at 0x%08X (FLASH CRC:0x%08X, FILE CRC:0x%08X) %s" % (idx, address + idx * blksize, sum1[idx], sum2[idx], result))
        print('')
        sys.stdout.flush()

    if 'e' in options:
        print('Erasing...')
        psize = address
        for loop in range ((size + blksize - 1) // blksize):
            val = blk[(psize - address) >> 16]
            if (val == 0) or (val == 2 and ('p' not in options)):
                print("Erasing   block   0x%08X - " % psize, end = '')
                if stm_comm.short_cmd  (STM32_COMM.CMD_ERASE_BLOCK, 0,  psize, 0) :
                    raise SystemExit ("ERR: failed to erase flash !")

                if (target == STM32_COMM.TARGET_DEDIPROG) and stm_comm.get_status ():
                    raise SystemExit ("ERR: failed to wait for erasing done !")

                print("DONE")
            elif val == 1 :
                print("Empty     block   0x%08X - SKIP" % psize)
            else :
                print("Identical block   0x%08X - SKIP" % psize)

            psize += blksize
            sys.stdout.flush()

        print('')
        sys.stdout.flush()


    if 'p' in options:
        print('Programming...')
        flag = 0
        if 'm' in options:
            # find available slot to update module
            flag |= 0x01

        if stm_comm.short_cmd  (STM32_COMM.CMD_SET_ADDR_LEN, flag, address, size) :
            raise SystemExit ("ERR:  failed to send the command")

        psize = address

        fpi  = open(sys.argv[1], 'rb')
        for loop in range ((size + pagesize - 1) // pagesize):
            tmp = fpi.read(pagesize)
            if len(tmp) == 0:
                break

            if len(tmp) < pagesize:
                tmp = tmp + b'\xff' * (pagesize - len(tmp))

            if (blk[(psize - address)>>16] == 2):
                if (psize & (blksize - 1)) == 0:
                    print("Identical block   0x%08X - SKIP" % psize)
                if stm_comm.short_cmd  (STM32_COMM.CMD_SKIP_PAGE, 0, pagesize, 0):
                    raise SystemExit ("ERR: failed to send the command !")

            else :
                if (psize & (blksize - 1)) == 0:
                    print("Programming block 0x%08X - " % psize, end = '')
                if stm_comm.stm_usb.write (tmp, 5 * STM32_COMM.USB_WR_TIMEOUT) != pagesize:
                    raise SystemExit ("ERR: failed to send the data !")

                if (psize & (blksize - 1)) == 0:
                    print ("DONE")

            psize += pagesize
            sys.stdout.flush()

        fpi.close ()
        print('')
        sys.stdout.flush()


    # Last packet to signal the end
    if 'd' in options:
        param = 1
    else :
        param = 0

    stm_comm.send_done (param)


if __name__ == '__main__':
    sys.exit(main())
