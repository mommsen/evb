#!/usr/bin/env python

import numpy as np

#----------------------------------------------------------------------

def printSolutions(fname):

    import pandas as pd

    data = pd.read_csv(fname, parse_dates = ['book_time'])

    # top n points
    # nlargest need pandas >= 0.17.0
    nlargest = 20
    # tmp = data.nlargest(nlargest, 'rate').reset_index()
    tmp = data.sort('rate', ascending = False).reset_index()

    parameters = sorted([ col[len('args/'):] for col in data.columns if col.startswith("args/")])
    
    # find top solutions but exclude duplicates
    # and report ranges instead

    # maps from core assingments in order defined in parameters
    # to list of performances
    
    performances = {}

    for index in range(len(data)):
        row = data.iloc[index]
        key = tuple([ row['args/' + param] for param in parameters ])
        
        performances.setdefault(key, []).append(row['rate'] / 1000.)

    average_performances = dict( [(key, 
                                  np.mean(value) )
                                   for key, value in performances.items() ])

    sorted_keys = list(performances.keys())
    sorted_keys.sort(key = lambda key: average_performances[key],
                     reverse = True)

    print "top %d distinct solutions" % nlargest

    print "%-50s:" % "solution rank", " ".join(["%5d" % (index + 1) for index in range(nlargest) ])

    print "%-50s:" % "avg. rate [kHz]", " ".join([ "%3.1f" % np.mean(performances[key]) for key in sorted_keys[:nlargest] ])
    print "%-50s:" % "min. rate [kHz]", " ".join([ "%3.1f" % np.min(performances[key]) for key in sorted_keys[:nlargest] ])
    print "%-50s:" % "max. rate [kHz]", " ".join([ "%3.1f" % np.max(performances[key]) for key in sorted_keys[:nlargest] ])
    print "%-50s:" % "number of measurements", " ".join([ "%5d" % len(performances[key]) for key in sorted_keys[:nlargest] ])

    print
    print "%-50s core assignment" % ""

    for param_index,param in enumerate(parameters):
        print "%-50s:" % param, " ".join(["%5d" % key[param_index] for key in sorted_keys[:nlargest] ])

    print





#----------------------------------------------------------------------
# main
#----------------------------------------------------------------------

if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser()

    parser.add_argument("data",nargs=1, help="csv file with optimization data")

    options = parser.parse_args()

    printSolutions(options.data[0])

