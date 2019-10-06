import serial
import time
import sys
from datetime import datetime

serial_port = sys.argv[1]

with serial.Serial(port=serial_port, baudrate=115200, timeout=.3) as ser:
    print(ser)

    #cmd = b"\ngarbage\x02query\x1ewww.grc.com\x1ec3FybDovL3d3dy5ncmMuY29tL3Nxcmw_bnV0PW9HWEVVRW1Ua1BHMHowRWthM3BISlE\x03\nmore Garbage!\n"
    #cmd = b"\x02query\x1ewww.grc.com\x1ec3FybDovL3d3dy5ncmMuY29tL3Nxcmw_bnV0PW9HWEVVRW1Ua1BHMHowRWthM3BISlE\x03"
    #cmd = b"\x02ident\x1ewww.grc.com\x1edmVyPTENCm51dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQp0aWY9NQ0KcXJ5PS9zcXJsP251dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzdWs9UEJGdWZRNmR2emgtYXB3dU1tXzR6MmFybmZNdjRDVUxVRTRWZVVFYWdWOA0KdXJsPWh0dHBzOi8vd3d3LmdyYy5jb20vc3FybC9kaWFnLmh0bT9fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzaW49MA0K\x1e0\x1etrue\x03"
    cmd = b"\x02unlock\x1e<scrypt-binary-key-goes-here>\x03"

    #print(len(cmd), hex(len(cmd)))
    print(repr(cmd))
    ser.write(cmd)

    i = 0
    resp = None
    while i < 2:
        a = ser.read(1)
        if a == b'':
            i +=1
            print("i =", i)
            continue
        else:
            i = 0

        if a == b'\x02':
            resp = a
        elif resp and a == b'\x03':
            resp += a
            resp_parts = resp[1:-1].split(b'\x1e')
            if resp_parts[0] == b'log':
                log = b' '.join(resp_parts[1:]).strip()
                #print('%s log: %r' % (datetime.now(), log.decode()))
                print(b'log: %r' % (log))
            elif resp_parts[0] == b'idresp':
                print('Found key response')
                print('  cmd    ', resp_parts[1])
                print('  result ', resp_parts[2])
                print('  status ', resp_parts[3])
            elif resp_parts[0] == b'resp':
                print('Found command response')
                print('  cmd    ', resp_parts[1])
                print('  client ', resp_parts[2])
                print('  server ', resp_parts[3])
                print('  ids    ', resp_parts[4])
            else:
                print(b'err: unknown resp: %r' % resp)
            resp = None
        elif resp:
            resp += a
        else:
            if a != b'\n':
                print(b'garbage: %r' % a)

