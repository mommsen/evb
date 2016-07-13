#!/usr/bin/env python

from argparse import ArgumentParser


parser = ArgumentParser()
parser.add_argument("paramFile",help="calculate the FED fragment sizes using the parameters in the given file")
parser.add_argument("relEventSize",type=float,help="scale the event size by this factor")
parser.add_argument("outputFile",help="file name of the output file")
args = vars(parser.parse_args())

eventSize = 0
with open(args['outputFile'],'w') as outputFile:
    outputFile.write("#fedId,size,rms\n")
    with open(args['paramFile']) as paramFile:
        for line in paramFile:
            (fedIds,a,b,c,rms,_) = line.split(',',5)
            try:
                for id in fedIds.split('+'):
                    fedSize = float(a)+float(b)*args['relEventSize']+float(c)*args['relEventSize']**2
                    outputFile.write("%d,%d,%d,1\n" % (int(id),int(fedSize)&~0x7,int(fedSize*float(rms))&~0x7))
                    eventSize += int(fedSize)
            except ValueError:
                pass

print("Total event size is %1.3f MB\n" % (float(eventSize)/1000000.))
