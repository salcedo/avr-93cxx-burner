#!/usr/bin/python2
#
# program 93C56
#

import serial
import sys

dump = range(0, 256) 

if len(sys.argv) != 3:
	print('Usage: %s <serial port> <file name>' % sys.argv[0])
	sys.exit()

ser = serial.Serial(sys.argv[1], 9600)

print('Waiting for EEPROM...')
ok = ser.read(2)

if ok != 'OK':
	print('Did not receive OK. Exiting.')
	sys.exit()

f = open(sys.argv[2], 'r')

print('Got OK...')

sys.stdout.write('Sending program data')
for i in range(0, 256):
	ser.write(f.read(1))
	sys.stdout.write('.')
	sys.stdout.flush()

sys.stdout.write('\nEEPROM programming...\n')
sys.stdout.flush()

ok = ser.read(2)

if ok != 'OK':
	print('Did not receive OK. Exiting.')
	sys.exit()

print('Got OK... done!')
ser.close()
f.close()
