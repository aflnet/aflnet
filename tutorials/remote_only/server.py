#!/usr/bin/env python3

# simple example from https://64k.space/practical-modbus-fuzzing-with-boofuzz.html

from pymodbus.datastore import ModbusSequentialDataBlock
from pymodbus.server.sync import StartTcpServer
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext

import logging
import os, sys
import netifaces

##
# A sloppy data block: someone forgot to check some address bounds
# somewhere, and reading input registers past 0xFF is going to cause
# this device to crash!
class BadDataBlock(ModbusSequentialDataBlock):
    def __init__(self):
        self.values = [0x00] * 0xFFFF
        super().__init__(0, self.values)

    def getValues(self, addr, count):
        # Uh-oh...
        if (addr <= 0xFF):
            return self.values[addr:addr+count]
        else:
            os._exit(1)

def run_server():

    bad_block = BadDataBlock()

    store = ModbusSlaveContext(
        di=ModbusSequentialDataBlock(0, [0xFF] * 32),
        co=ModbusSequentialDataBlock(0, [0xFF] * 32),
        hr=ModbusSequentialDataBlock(0, [0xFF] * 32),
        ir=bad_block)

    context = ModbusServerContext(slaves=store, single=True)

    ip = netifaces.ifaddresses(sys.argv[1])[netifaces.AF_INET][0]['addr']
    StartTcpServer(context, address=(ip, int(sys.argv[2])))

def usage():
    print(os.path.basename(__file__) + " <listening interface> <port>")

def main():
    if (len(sys.argv) != 3):
        usage()
        sys.exit()

    FORMAT = ('%(asctime)-15s %(threadName)-15s'
                    ' %(levelname)-8s %(module)-15s:%(lineno)-8s %(message)s')
    logging.basicConfig(format=FORMAT)
    log = logging.getLogger()
    log.setLevel(logging.DEBUG)
    run_server()

if __name__ == "__main__":
    main()

