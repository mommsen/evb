#!/usr/bin/env python

#----------------------------------------------------------------------

def plotPerformance(fname):

    title_size = 20
    label_size = 15
    figsize = (10,10)

    import matplotlib.pyplot as plt
    import pandas as pd

    data = pd.read_csv(fname, parse_dates = ['book_time'])

    for xvar, xlabel in (
        ("book_time", 'test start time'),     # performance vs. time
        ("generation", 'generation'),    # performance vs. generation
        ):
        plt.figure(figsize = figsize)
        plt.plot(data[xvar], data["rate"] / 1e3, '.')

        # also mark top n points
        # nlargest need pandas >= 0.17.0
        nlargest = 10
        # tmp = data.nlargest(nlargest, 'rate').reset_index()
        tmp = data.sort('rate', ascending = False).reset_index()[:nlargest]

        plt.plot(tmp[xvar], tmp['rate'] / 1e3, 
                 'o', 
                 color = 'red',
                 label = '%d largest (%.1f .. %.1f kHz)' % (nlargest, 
                                                            tmp['rate'].min() / 1e3,
                                                            tmp['rate'].max() / 1e3,       
                                                            ),
                 )

        plt.grid()
        plt.xlabel(xlabel, fontsize = title_size)
        plt.ylabel('readout rate [kHz]', fontsize = title_size)
        plt.gca().tick_params(labelsize = label_size)


        plt.legend(loc = 'lower left')

#----------------------------------------------------------------------
# main
#----------------------------------------------------------------------

if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser()

    parser.add_argument("data",nargs=1, help="csv file with optimization data")

    options = parser.parse_args()

    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.rcParams['figure.facecolor'] = 'white'

    plotPerformance(options.data[0])

    plt.show()
