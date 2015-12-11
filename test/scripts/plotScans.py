#!/usr/bin/env python

import os
import re
import sys
import time

import ROOT
from ROOT import gROOT,gStyle
#from ROOT import TGraphErrors,  TLegend, TH2D, TLatex, TPave

class PlotScans:


    def addOptions(self,parser):
        parser.add_argument("cases",nargs="+",help="List of cases to plot")
        parser.add_argument("-t","--tag",help="Title tag in plot canvas")
        parser.add_argument("-t2","--subtag",help="Subtitle tag in plot canvas")
        parser.add_argument("-tt","--title",help="Canvas title")
        parser.add_argument("-xt","--titleX",help="X axis title")
        parser.add_argument("-yt","--titleY",help="Y axis title")
        parser.add_argument("--minx",type=float,help="X axis range, minimum")
        parser.add_argument("--maxx",type=float,help="X axis range, maximum")
        parser.add_argument("--miny",type=float,help="Y axis range, minimum")
        parser.add_argument("--maxy",type=float,help="Y axis range, maximum")
        parser.add_argument("--logx",default=True,action="store_true",help="Use logarithmic scale on x axis [default: %(default)s]")
        parser.add_argument("--logy",default=False,action="store_true",help="Use logarithmic scale on y axis [default: %(default)s]")
        parser.add_argument("-o","--outputName",default="plot.pdf",help="File for plot output [default: %(default)s]")
        parser.add_argument("-a","--app",default="RU1",help="Take values from given application [default: %(default)s]")


    def createCanvas(self):
        gROOT.SetBatch()
        gStyle.SetOptTitle(0)
        gStyle.SetOptStat(0)
        oname = self.args['outputName']
        if oname.endswith('.pdf') or oname.endswith('.png'): oname = oname[:-4]
        self.canvas = ROOT.TCanvas(oname,"Plot",0,0,1024,768)


    def createThroughputPad(self):
        self.throughputPad = ROOT.TPad("throughputPad","",0,0,1,1)
        self.throughputPad.Draw()
        self.throughputPad.cd()
        if self.args['logx']:
            self.throughputPad.SetLogx()
        if self.args['logy']:
            self.throughputPad.SetLogy()
        # self.throughputPad.SetGridx()
        # self.throughputPad.SetGridy()


    def createAxes(self):
        if 'BU' in self.args['app']:
            range = [6,800,0,10000]
            title = "Throughput vs. Event Size"
            titleX = "Event Size (kB)"
        else:
            range = [250,17000,0,5900]
            title = "Throughput vs. Fragment Size"
            titleX = "Fragment Size (bytes)"
        titleY = "Throughput on %(app)s (MB/s)"%(self.args)
        if not self.args['minx']:
            self.args['minx'] = range[0]
        if not self.args['maxx']:
            self.args['maxx'] = range[1]
        if not self.args['miny']:
            self.args['miny'] = range[2]
        if not self.args['maxy']:
            self.args['maxy'] = range[3]
        if not self.args['title']:
            self.args['title'] = title
        if not self.args['titleX']:
            self.args['titleX'] = titleX
        if not self.args['titleY']:
            self.args['titleY'] = titleY
        self.axes = ROOT.TH2D('axes','A',100,self.args['minx'],self.args['maxx'],100,self.args['miny'],self.args['maxy'])
        self.axes.SetTitle(self.args['title'])
        self.axes.GetXaxis().SetTitle(self.args['titleX'])
        self.axes.GetYaxis().SetTitle(self.args['titleY'])
        self.axes.GetXaxis().SetMoreLogLabels()
        self.axes.GetXaxis().SetNoExponent()
        self.axes.GetYaxis().SetTitleOffset(1.4)
        self.axes.GetXaxis().SetTitleOffset(1.2)
        self.axes.Draw()


    def createLineSpeed(self):
        if 'BU' in self.args['app']:
            self.lineSpeed = ROOT.TLine(self.args['minx'],7000,self.args['maxx'],7000)
        else:
            self.lineSpeed = ROOT.TLine(self.args['minx'],5000,self.args['maxx'],5000)
        self.lineSpeed.SetLineColor(ROOT.kGray+2)
        self.lineSpeed.SetLineStyle(7)
        self.lineSpeed.Draw()


    def createLegend(self):
        legendPosY = 0.828
        self.tl = ROOT.TLatex()
        self.tl.SetTextFont(42)
        self.tl.SetNDC(1)
        if self.args['tag']:
            width = 0.12+0.020*len(self.args['tag'])
            if width > 0.9: width=0.899
            self.pave = ROOT.TPave(0.12, legendPosY-0.03, width, legendPosY+0.069, 0, 'NDC')
            self.pave.SetFillStyle(1001)
            self.pave.SetFillColor(0)
            self.pave.Draw()
            self.tl.SetTextSize(0.05)
            self.tl.DrawLatex(0.14, legendPosY, self.args['tag'])
            legendPosY -= 0.06
        if self.args['subtag']:
            width2 = 0.12+0.015*len(self.args['subtag'])
            if width2 > 0.9: width2=0.899
            if width2 < width: width2 = width
            self.pave2 = ROOT.TPave(0.12, legendPosY-0.02, width2, legendPosY+0.03, 0, 'NDC')
            self.pave2.SetFillStyle(1001)
            self.pave2.SetFillColor(0)
            self.pave2.Draw()
            self.tl.SetTextSize(0.035)
            self.tl.DrawLatex(0.145, legendPosY, self.args['subtag'])
            legendPosY -= 0.02
        nlegentries = len(self.args['cases'])
        self.legend = ROOT.TLegend(0.12,legendPosY-nlegentries*0.04,0.38,legendPosY)
        self.legend.SetFillStyle(1001)
        self.legend.SetFillColor(0)
        self.legend.SetTextFont(42)
        self.legend.SetTextSize(0.033)
        self.legend.SetBorderSize(0)
        self.legend.Draw()


    def getThroughputGraph(self,n,case):
        if len(case['sizes']) == 0:
            return None
        from numpy import array
        colors  = [1,2,ROOT.kGreen+1,4,51,95,65,41,46,39,32]
        markers = [20,21,22,23,34,33,29,24,25,26,27,28]
        graph = ROOT.TGraphErrors(len(case['sizes']),
                                  array(case['sizes'],'f'),
                                  array(case['throughputs'],'f'),
                                  array(case['rmsSizes'],'f'),
                                  array(case['rmsThroughputs'],'f'))
        graph.SetLineWidth(2)
        graph.SetLineColor(colors[n])
        graph.SetMarkerColor(colors[n])
        graph.SetMarkerStyle(markers[n])
        graph.SetMarkerSize(1.7)
        graph.Draw("PL")
        self.legend.AddEntry(graph,case['name'],'P')
        return graph


    def createThroughputGraphs(self):
        self.throughputGraphs = []
        for n,case in enumerate(self.cases):
            graph = self.getThroughputGraph(n,case)
            if graph:
                self.throughputGraphs.append(graph)


    def readCaseData(self,case):
        from numpy import mean,sqrt,square
        entry = {}
        entry['name'] = os.path.splitext(os.path.basename(case))[0]
        entry['sizes'] = []
        entry['rmsSizes'] = []
        entry['rates'] = []
        entry['rmsRates'] = []
        entry['throughputs'] = []
        entry['rmsThroughputs'] = []
        with open(case,'r') as file:
            for line in file:
                try:
                    value = eval(line)
                except ValueError:
                    continue
                dataPoint = value['measurement']
                if 'BU' in self.args['app']:
                    sizes = list(x['sizes'][self.args['app']]/1000. for x in dataPoint)
                    averageSize = mean(sizes)
                    entry['sizes'].append(averageSize)
                    entry['rmsSizes'].append(sqrt(mean(square(averageSize-sizes))))
                else:
                    entry['sizes'].append(value['fragSize'])
                    entry['rmsSizes'].append(value['fragSizeRMS'])
                rates = list(x['rates'][self.args['app']]/1000. for x in dataPoint)
                averageRate = mean(rates)
                entry['rates'].append(averageRate)
                entry['rmsRates'].append(sqrt(mean(square(averageRate-rates))))
                throughputs = list(x['sizes'][self.args['app']]*x['rates'][self.args['app']]/1000000. for x in dataPoint)
                averageThroughput = mean(throughputs)
                entry['throughputs'].append(averageThroughput)
                entry['rmsThroughputs'].append(sqrt(mean(square(averageThroughput-throughputs))))
        return entry


    def readData(self):
        self.cases = []
        for case in self.args['cases']:
            self.cases.append( self.readCaseData(case) )


    def printTable(self):
        for case in self.cases:
            print(47*"-")
            print("Case: "+case['name']+" - "+self.args['app'])
            print(47*"-")
            if 'BU' in self.args['app']:
                unit = '(kB)'
            else:
                unit = '(B)'
            print("Size %4s : Throughput (MB/s) :      Rate (kHz)"%(unit))
            print(47*"-")
            for entry in zip(case['sizes'],case['throughputs'],case['rmsThroughputs'],case['rates'],case['rmsRates']):
                print("%9d :  %6.1f +- %6.1f :  %5.1f +- %5.1f"%entry)
            print(47*"-")


    def doIt(self,args):
        self.args = vars(args)
        #print(self.args)

        self.readData()
        self.printTable()
        self.createCanvas()
        self.createThroughputPad()
        self.createAxes()
        self.createLineSpeed()
        self.createLegend()
        self.createThroughputGraphs()
        self.canvas.Print(self.canvas.GetName()+".pdf")
        self.canvas.Print(self.canvas.GetName()+".png")


if __name__ == "__main__":
    from argparse import ArgumentParser
    parser = ArgumentParser()
    plotScans = PlotScans()
    plotScans.addOptions(parser)
    plotScans.doIt( parser.parse_args() )
