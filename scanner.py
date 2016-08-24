#! /usr/bin/env python
__author__ = 'HB9TXW'
import serial
import matplotlib.pyplot as plt
import argparse
# import sys, getopt
# import numpy as np

# STARTFREQ, STOPFREQ, POINTS, ORDRE = 1000000, 30000000, 100, 'H'
bavard, sortie = False, False
fichier, comport = 'bidon.txt', '/dev/ttyUSB0'


parser = argparse.ArgumentParser()
parser.add_argument('-o', '--output', help='destination file where the result is stored')
parser.add_argument('-v', '--verbose', dest='verbose', action='store_true')
parser.add_argument('-p', '--port', help='comport used by the scanner')
args = parser.parse_args()
if args.verbose == True:
    bavard = True
if type(args.output) == str:
    sortie = True
    fichier = args.output
if type(args.port) == str:
    comport = args.port
if bavard: print type(args.output)

ser = serial.Serial(port = comport, baudrate = 57600, timeout = 1)
print(ser.name)

def getnum (chaine):
    i = 0
    # l = len(chaine)
    # isnum = True
    numchain = ''
    a = [0.0] * 5
    for car in chaine:
        if car == ',':
            a [i] = float(numchain)
            i = i+1
            numchain = ''
        else:
            numchain = numchain + car
    a [i] = float(numchain)
    return a

def debut():
    ffloat = 1000000*float(raw_input('Please enter begin frequency in MHz: '))
    if ffloat > 60000000:
        ffloat = 60000000
    elif ffloat < 1000000:
        ffloat = 1000000
    startf = int(ffloat)
    ser.write (str(startf)+'A')
    return startf

def fin():
    ffloat = 1000000*float(raw_input('Please enter end frequency in MHz: '))
    if ffloat > 60000000:
        ffloat = 60000000
    elif ffloat < 1000000:
        ffloat = 1000000
    stopf = int(ffloat)
    ser.write (str(stopf)+'B')
    return stopf

def points():
    pts = int(raw_input('Please enter number of points: '))
    if pts < 10:
        pts=10
    elif pts > 1001:
        pts = 1001
    ser.write (str(pts)+'N')
    return pts

def mesurer(d,f,p):
    i = 0
    F = [0.0] * (p + 1)
    SWR = [0.0] * (p + 1)
    if bavard:
        print('On va effectuer un scan.')
    ser.write ('S')
    while i <= p:
        ligne = ser.readline()
        if bavard:
            print (ligne),
            print ( getnum (ligne) )
        resultat = getnum (ligne)
        F [i] = resultat[0] * 0.000001
        SWR [i] = resultat[2] * 0.001
        i = i+1
    ligne = ser.readline()
    if bavard:
        print (ligne),
        print (F)
        print (SWR)
    plt.clf()
    if bavard:
        print ('On va dessiner les courbes')
    plt.plot(F, SWR)
    plt.draw()
    if bavard:
        print ('On va dessiner les labels')
    plt.xlabel('frequency (MHz)')
    plt.ylabel('VSWR')
    plt.title('This is how it works with Python folks')
    if bavard:
        print ('On va dessiner les axes')
    plt.grid(True)
    if bavard:
        print ('On va sauver le graphe')
    plt.savefig("test.png")
    if bavard:
        print ('On va attendre 1 ms')
    plt.pause(0.001)
    plt.show(block=False)
    if sortie:
        measurements = ''
        measurements += 'frequency,VSWR\nMHz,-\n'
        for i in range(len(F)): measurements += (str(F[i])+','+ str(SWR[i])+'\n')
        measurements+=('\n\n')
        fo = open(fichier, 'a')
        fo.write(measurements)
        fo.close()

def interroge():
    i=0
    param = [0.0] * 3
    print('Valeur chargees dans le scanner')
    ser.write ('?')
    while i <= 2:
        ligne = ser.readline()
        print (ligne),
        i = i+1
        if ligne.startswith('Start Freq:'):
            ligne = ligne.replace('Start Freq:','')
            param [0] = float(ligne)
        elif ligne.startswith('Stop Freq:'):
            ligne = ligne.replace('Stop Freq:','')
            param [1] = float(ligne)
        elif ligne.startswith('Num Steps:'):
            ligne = ligne.replace('Num Steps:','')
            param [2] = float(ligne)
    return param

def aide():
    print('B = begin frequency in MHz')
    print('E = end frequency in MHz')
    print('N = number of measurement points')
    print('S = perform scan')
    print('? = interrogate the scanner')
    print('H = help')
    print('Q = quit')


def scanner():
    PARAM  = interroge()
    STARTFREQ = int(PARAM [0])
    STOPFREQ = int(PARAM [1])
    POINTS = int(PARAM [2])
    if bavard:
        print (STARTFREQ, STOPFREQ, POINTS)
        print 'bavard ',
        print (bavard)
    aide()
    BOUCLE = True
    while BOUCLE:
        ORDRE = raw_input('Please enter what you want to do: ')
        if ORDRE == 'B':
            STARTFREQ = debut()
        elif ORDRE == 'E':
            STOPFREQ = fin()
        elif ORDRE == 'N':
            POINTS = points()
        elif ORDRE == 'S':
            mesurer(STARTFREQ, STOPFREQ, POINTS)
        elif ORDRE == '?':
            PARAM = interroge()
            STARTFREQ = int(PARAM [0])
            STOPFREQ = int(PARAM [1])
            POINTS = int(PARAM [2])
        elif ORDRE == 'H':
            aide()
        elif ORDRE == 'Q':
            BOUCLE = False
        else:
            print ('There is no action defined for '+ ORDRE + ' !')
            aide()

if __name__ == '__main__': scanner()

