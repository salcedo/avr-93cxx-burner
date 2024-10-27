#!/usr/bin/python2
#
# dump 93C56
#

import serial
import sys

dump = range(0, 256) 

if len(sys.argv) != 3:
	print('Usage: %s <serial port> <file name>' % sys.argv[0])
	sys.exit()

ser = serial.Serial(sys.argv[1], 9600)

print('Waiting for EEPROM...')
dump[0] = ser.read(1)

sys.stdout.write('Reading EEPROM')

for i in range(1, 256):
	dump[i] = ser.read(1)
	sys.stdout.write('.')
	sys.stdout.flush()

sys.stdout.write('\nFinished. Saving to %s\n' % sys.argv[2])
sys.stdout.flush()

f = open(sys.argv[2], 'w')
f.write(''.join(dump))
f.close()
