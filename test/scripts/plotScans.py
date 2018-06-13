#!/usr/bin/env python

import os
import re
import sys
import time
from collections import OrderedDict

import ROOT
from ROOT import gROOT,gStyle


class PlotScans:

    def __init__(self):
        self.colors  = [1,4,417,2,51,95,65,41,46,39,32,49]
        self.markers = [20,21,22,23,34,33,29,24,25,26,27,28,29]
        self.latex = ROOT.TLatex()
        self.latex.SetNDC(1)
        self.nominalSize = None
        self.axisTitleSize = 0.044
        self.labelSize = 0.042
        self.marginSize = 0.114


    def addOptions(self,parser):
        parser.add_argument("cases",nargs="+",help="List of cases to plot")
        parser.add_argument("-t","--tag",help="Title tag in plot canvas")
        parser.add_argument("-t2","--subtag",help="Subtitle tag in plot canvas")
        parser.add_argument("-tt","--title",help="Canvas title")
        parser.add_argument("-xt","--titleX",help="X axis title")
        parser.add_argument("-yt","--titleY",help="Y axis title")
        parser.add_argument("-yr","--titleR",help="Y axis title for rate")
        parser.add_argument("--minx",type=float,help="X axis range, minimum")
        parser.add_argument("--maxx",type=float,help="X axis range, maximum")
        parser.add_argument("--miny",type=float,help="Y axis range, minimum")
        parser.add_argument("--maxy",type=float,help="Y axis range, maximum")
        parser.add_argument("--ratemaxy",type=float,help="Y axis range for rate, maximum")
        parser.add_argument("--rate",type=float,default="100",help="Rate in kHz to be displayed on the plot [default: %(default)s kHz]")
        parser.add_argument("--nologx",default=False,action="store_true",help="Use logarithmic scale on x axis [default: %(default)s]")
        parser.add_argument("--logy",default=False,action="store_true",help="Use logarithmic scale on y axis [default: %(default)s]")
        parser.add_argument("--hideDate",default=False,action="store_true",help="Hide the date")
        parser.add_argument("--hideRate",default=False,action="store_true",help="Hide the rate curves")
        parser.add_argument("--hideLineSpeed",default=False,action="store_true",help="Hide the line speed line")
        parser.add_argument("-o","--outputName",default="plot.pdf",help="File for plot output [default: %(default)s]")
        parser.add_argument("-a","--app",nargs="*",default=['RU0'],help="Take values from given application [default: %(default)s]")
        parser.add_argument('--legend',nargs="*",help="Give a list of custom legend entries to be used")
        parser.add_argument('--colors',nargs="*",type=int,help="Give a list of custom colors to be used")
        parser.add_argument('--markers',nargs="*",type=int,help="Give a list of custom markers to be used")
        parser.add_argument("--totalThroughput",default=False,action="store_true",help="Plot the total event-builder throughput")
        parser.add_argument("--showRelEventSize",default=False,action="store_true",help="Show the relative event size")
        parser.add_argument("--showVertices",nargs=2,type=float,help="Show priary vertices with abscissa and slope given as arguments")
        parser.add_argument("--plotMaxRU",default=False,action="store_true",help="Plot the RU with the highest througput")
        parser.add_argument("--plotSuperFragmentSize",default=False,action="store_true",help="Plot the measured super-fragment size instead of set fragment size")


    def createCanvas(self):
        gROOT.SetBatch()
        gStyle.SetOptTitle(0)
        gStyle.SetOptStat(0)
        oname = self.args['outputName']
        if oname.endswith('.pdf') or oname.endswith('.png'): oname = oname[:-4]
        if self.args['showRelEventSize'] or self.args['showVertices']:
            self.canvas = ROOT.TCanvas(oname,"Plot",0,0,1024,768)
            self.topMargin = self.marginSize
        else:
            self.canvas = ROOT.TCanvas(oname,"Plot",0,0,1024,699)
            self.topMargin = 0.02


    def createThroughputPad(self):
        self.throughputPad = ROOT.TPad("throughputPad","",0,0,1,1)
        self.throughputPad.Draw()
        self.throughputPad.cd()
        if not self.args['nologx']:
            self.throughputPad.SetLogx()
        if self.args['logy']:
            self.throughputPad.SetLogy()
        # self.throughputPad.SetGridx()
        # self.throughputPad.SetGridy()
        self.throughputPad.SetMargin(self.marginSize,self.marginSize,self.marginSize,self.topMargin)
        app = re.sub(r'[0-9]+$','',self.cases[0]['app'])
        titleY = "Throughput on "+app+" (MB/s)"
        titleYoffset = 1.3
        if self.args['totalThroughput']:
            range = [100,6000,0,200]
            title = "Throughput vs. Event Size"
            titleX = "Event Size (kB)"
            titleY = "Total EvB throughput (GB/s)"
            titleYoffset = 1.2
        elif self.args['plotSuperFragmentSize']:
            range = [0,100,0,5900]
            title = "Throughput vs. Super-fragment Size"
            titleX = "Super-fragment Size (kB)"
        elif 'BU' in app:
            range = [100,5000,0,10000]
            title = "Throughput vs. Event Size"
            titleX = "Event Size (kB)"
        else:
            range = [250,17000,0,5900]
            title = "Throughput vs. Fragment Size"
            titleX = "Fragment Size (bytes)"
        if self.args['plotMaxRU']:
            range = [5,100,0,5900]
            title = "Throughput vs. Super-fragment Size"
            titleX = "Super-fragment Size (kB)"
            titleY = "Max throughput on RU (MB/s)"
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
        self.throughputTH2D = ROOT.TH2D('throughputTH2D','A',100,self.args['minx'],self.args['maxx'],100,self.args['miny'],self.args['maxy'])
        self.throughputTH2D.SetTitle(self.args['title'])
        self.throughputTH2D.GetXaxis().SetTitle(self.args['titleX'])
        self.throughputTH2D.GetYaxis().SetTitle(self.args['titleY'])
        self.throughputTH2D.GetXaxis().SetMoreLogLabels()
        self.throughputTH2D.GetXaxis().SetNoExponent()
        self.throughputTH2D.GetYaxis().SetTitleOffset(titleYoffset)
        self.throughputTH2D.GetYaxis().SetLabelSize(self.labelSize)
        self.throughputTH2D.GetYaxis().SetTitleSize(self.axisTitleSize)
        self.throughputTH2D.GetXaxis().SetTitleOffset(1.2)
        self.throughputTH2D.GetXaxis().SetTitleSize(self.axisTitleSize)
        self.throughputTH2D.GetXaxis().SetLabelSize(self.labelSize)
        self.throughputTH2D.Draw()
        if self.args['showRelEventSize'] or self.args['showVertices']:
            if self.args['showRelEventSize']:
                minValueX = self.args['minx'] / self.nominalSize
                maxValueX = self.args['maxx'] / self.nominalSize
                title="Relative event size"
            else:
                minValueX = (self.args['minx']-self.args['showVertices'][0])/self.args['showVertices'][1]
                maxValueX = (self.args['maxx']-self.args['showVertices'][0])/self.args['showVertices'][1]
                title="Number of rec. primary vertices"
            if self.args['nologx']:
                logOpt=''
            else:
                logOpt='G'
            self.relEventSizeAxis = ROOT.TGaxis(self.args['minx'],self.args['maxy'],self.args['maxx'],self.args['maxy'],minValueX,maxValueX,510,logOpt+'-');
            self.relEventSizeAxis.SetTitle(title)
            self.relEventSizeAxis.SetTitleOffset(1.3)
            self.relEventSizeAxis.SetMoreLogLabels()
            self.relEventSizeAxis.SetNoExponent()
            self.relEventSizeAxis.SetTitleFont(self.throughputTH2D.GetXaxis().GetTitleFont())
            self.relEventSizeAxis.SetTitleSize(self.throughputTH2D.GetXaxis().GetTitleSize())
            self.relEventSizeAxis.SetLabelFont(self.throughputTH2D.GetXaxis().GetLabelFont())
            self.relEventSizeAxis.SetLabelSize(self.throughputTH2D.GetXaxis().GetLabelSize())
            self.relEventSizeAxis.Draw()


    def createRatePad(self):
        self.ratePad = ROOT.TPad("ratePad","",0,0,1,1)
        self.ratePad.SetFillStyle(4000) ## transparent
        self.ratePad.SetFillColor(0)
        self.ratePad.SetFrameFillStyle(4000)
        self.ratePad.SetMargin(self.marginSize,self.marginSize,self.marginSize,self.topMargin)
        if not self.args['nologx']:
            self.ratePad.SetLogx()
        if self.args['logy']:
            self.ratePad.SetLogy()
        self.ratePad.Draw()
        self.ratePad.cd()
        app = re.sub(r'[0-9]+$','',self.cases[0]['app'])
        if 'BU' in app and not self.args['totalThroughput']:
            ratemaxy = 20
            titleR = "Event Rate at "+app+" (kHz)"
            titleOffset = 1.1
        else:
            ratemaxy = 380
            titleR = "Event Rate at EVM (kHz)"
            titleOffset = 1.3
        if not self.args['ratemaxy']:
            self.args['ratemaxy'] = ratemaxy
        if not self.args['titleR']:
            self.args['titleR'] = titleR
        self.rateTH2D = ROOT.TH2D('rateTH2D','A',100,self.args['minx'],self.args['maxx'],100,0,self.args['ratemaxy'])
        self.rateTH2D.GetXaxis().SetTickLength(0);
        self.rateTH2D.GetXaxis().SetLabelOffset(999);
        self.rateTH2D.GetYaxis().SetTitle(self.args['titleR'])
        self.rateTH2D.GetYaxis().SetTitleOffset(titleOffset)
        self.rateTH2D.GetYaxis().SetTitleSize(self.axisTitleSize)
        self.rateTH2D.GetYaxis().SetLabelSize(self.labelSize);
        self.rateTH2D.Draw('Y+')


    def createAuxPad(self):
        self.auxPad = ROOT.TPad("auxPad","",0,0,1,1)
        self.auxPad.SetFillStyle(4000) ## transparent
        self.auxPad.SetFillColor(0)
        self.auxPad.SetFrameFillStyle(4000)
        self.auxPad.SetMargin(self.marginSize,self.marginSize,self.marginSize,self.topMargin)
        self.auxPad.Draw()

    def createLineSpeed(self):
        self.throughputPad.cd()
        if 'BU' in self.cases[0]['app']:
            self.lineSpeed = ROOT.TLine(self.args['minx'],7000,self.args['maxx'],7000)
        else:
            self.lineSpeed = ROOT.TLine(self.args['minx'],5000,self.args['maxx'],5000)
        self.lineSpeed.SetLineColor(ROOT.kGray+2)
        self.lineSpeed.SetLineStyle(7)
        self.lineSpeed.Draw()


    def createRequirementLine(self):
        self.ratePad.cd()
        if 'BU' in self.cases[0]['app'] and not self.args['totalThroughput']:
            rate = self.args['rate']/73
        else:
            rate = self.args['rate']
        self.rateRequired = ROOT.TLine(self.args['minx'],rate,self.args['maxx'],rate)
        self.rateRequired.SetLineColor(ROOT.kGray+2)
        self.rateRequired.SetLineStyle(3)
        self.rateRequired.Draw()


    def createCMSpreliminary(self):
        self.auxPad.cd()
        #self.CMSpave = ROOT.TPave(0.62,0.80,0.899,0.898,0,'NDC')
        #self.CMSpave.SetFillStyle(1001)
        #self.CMSpave.SetFillColor(0)
        #self.CMSpave.Draw()
        self.latex.SetTextFont(62)
        self.latex.SetTextSize(0.05)
        self.latex.DrawLatex(0.617,0.944-self.topMargin,"CMS")
        self.latex.SetTextFont(52)
        self.latex.DrawLatex(0.70,0.944-self.topMargin,"Preliminary")


    def createDate(self):
        self.auxPad.cd()
        self.datePave = ROOT.TPaveText(0.005,0.005,0.1,0.03,'NDC')
        self.datePave.SetTextFont(42)
        self.datePave.SetTextSize(0.03)
        self.datePave.SetFillStyle(1001)
        self.datePave.SetFillColor(0)
        self.datePave.SetBorderSize(0)
        self.datePave.SetTextAlign(12)
        self.datePave.AddText( self.getFileDate() )
        self.datePave.Draw()


    def createLegend(self):
        self.throughputPad.cd()
        self.latex.SetTextFont(42)
        legendPosX = 0.125
        legendPosY = 0.944-self.topMargin
        width = 0.33
        height = 0
        firstLine = 0
        if self.args['tag']:
            self.latex.SetTextSize(0.055)
            tag = self.latex.DrawLatex(legendPosX+0.01,legendPosY-height,self.args['tag'])
            width = max(width,tag.GetYsize())
            height += tag.GetYsize() + 0.015
            firstLine = max(firstLine,tag.GetYsize())
        if self.args['subtag']:
            self.latex.SetTextSize(0.04)
            subtag = self.latex.DrawLatex(legendPosX+0.01,legendPosY-height,self.args['subtag'])
            width = max(width,subtag.GetXsize())
            height += subtag.GetYsize()
            firstLine = max(firstLine,subtag.GetYsize())
        self.pave = ROOT.TPave(legendPosX,legendPosY+firstLine,legendPosX+width,legendPosY-height,0,'NDC')
        self.pave.SetFillStyle(1001)
        self.pave.SetFillColor(0)
        self.pave.Draw()
        try:
            tag.Draw()
        except UnboundLocalError:
            pass
        try:
            subtag.Draw()
        except UnboundLocalError:
            pass
        nlegentries = len(self.cases)
        self.legend = ROOT.TLegend(legendPosX,legendPosY-height-nlegentries*0.045,legendPosX+width,legendPosY-height)
        self.legend.SetFillStyle(1001)
        self.legend.SetFillColor(0)
        self.legend.SetTextFont(42)
        self.legend.SetTextSize(0.039)
        self.legend.SetTextAlign(12);
        self.legend.SetBorderSize(0)
        self.legend.SetMargin(0.15)
        self.legend.Draw()


    def fillLegend(self):
        useLegend = False
        if self.args['legend']:
            if len(self.args['legend']) != len(self.throughputGraphs):
                print("Legends doesn't match with filelist, falling back to default")
            else:
                useLegend = True
        for n,graph in enumerate(self.throughputGraphs):
            if useLegend:
                self.legend.AddEntry(graph,self.args['legend'][n],'P')
            else:
                self.legend.AddEntry(graph,graph.GetTitle(),'P')


    def getThroughputGraph(self,n,case):
        try:
            from numpy import array
            graph = ROOT.TGraphErrors(len(case['sizes']),
                                    array(case['sizes'],'f'),
                                    array(case['throughputs'],'f'),
                                    array(case['rmsSizes'],'f'),
                                    array(case['rmsThroughputs'],'f'))
            graph.SetTitle(case['name'])
            graph.SetLineWidth(2)
            graph.SetLineColor(self.colors[n])
            graph.SetMarkerColor(self.colors[n])
            graph.SetMarkerStyle(self.markers[n])
            graph.SetMarkerSize(1.7)
            graph.Draw("PL")
            return graph
        except KeyError:
            return None


    def getRateGraph(self,n,case):
        try:
            from numpy import array
            graph = ROOT.TGraphErrors(len(case['sizes']),
                                        array(case['sizes'],'f'),
                                        array(case['rates'],'f'),
                                        array(case['rmsSizes'],'f'),
                                        array(case['rmsRates'],'f'))
            graph.SetTitle(case['name'])
            graph.SetLineWidth(1)
            graph.SetLineStyle(2)
            graph.SetLineColor(self.colors[n])
            graph.Draw("L")
            return graph
        except KeyError:
            return None


    def createThroughputGraphs(self):
        self.throughputPad.cd()
        self.throughputGraphs = []
        for n,case in enumerate(self.cases):
            graph = self.getThroughputGraph(n,case)
            if graph:
                self.throughputGraphs.append(graph)


    def createRateGraphs(self):
        self.ratePad.cd()
        self.rateGraphs = []
        for n,case in enumerate(self.cases):
            graph = self.getRateGraph(n,case)
            if graph:
                self.rateGraphs.append(graph)


    def getFileDate(self):
        maxTime = 0.
        for case in self.args['cases']:
            maxTime = max(maxTime,os.path.getctime(case))
        return time.strftime("%d %b %Y",time.gmtime(maxTime))


    def readCaseData(self,case,app):
        from numpy import mean,sqrt,square
        entry = {}
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
                if self.args['totalThroughput']:
                    if 'BU0' in dataPoint[0]['sizes']:
                        app = 'BU0'
                    else:
                        app = 'BU1'
                elif app not in dataPoint[0]['sizes']:
                    app = 'RU0'
                if self.args['plotMaxRU']:
                    maxThroughput = 0
                    for ru in list(k for k in dataPoint[0]['rates'].keys() if k.startswith('RU')):
                        throughput = mean(list(x['sizes'][ru]*x['rates'][ru]/1000000000. for x in dataPoint))
                        if throughput > maxThroughput:
                            maxThroughput = throughput
                            app = ru
                    print(value['fragSize'],app,maxThroughput)
                if 'BU' in app or self.args['plotMaxRU'] or self.args['plotSuperFragmentSize']:
                    rawSizes = list(x['sizes'][app]/1000. for x in dataPoint)
                    sizes = list(x for x in rawSizes if x > 0)
                    if len(sizes) == 0:
                        continue
                    averageSize = mean(sizes)
                    entry['sizes'].append(averageSize)
                    entry['rmsSizes'].append(sqrt(mean(square(averageSize-sizes))))
                    if value['fragSize'] == 2048 and not self.nominalSize:
                        self.nominalSize = averageSize
                else:
                    entry['sizes'].append(value['fragSize'])
                    entry['rmsSizes'].append(value['fragSizeRMS'])
                if self.args['totalThroughput']:
                    rawRates = list(x['rates']['RU1']/1000. for x in dataPoint)
                    rawThroughputs = list(sum(x['sizes'][k]*x['rates'][k]/1000000000. for k in x['rates'].keys() if k.startswith(('EVM','RU'))) for x in dataPoint)
                else:
                    rawRates = list(x['rates'][app]/1000. for x in dataPoint)
                    rawThroughputs = list(x['sizes'][app]*x['rates'][app]/1000000. for x in dataPoint)
                rates = list(x for x in rawRates if x > 0)
                averageRate = mean(rates)
                entry['rates'].append(averageRate)
                entry['rmsRates'].append(sqrt(mean(square(averageRate-rates))))
                throughputs = list(x for x in rawThroughputs if x > 0)
                averageThroughput = mean(throughputs)
                entry['throughputs'].append(averageThroughput)
                entry['rmsThroughputs'].append(sqrt(mean(square(averageThroughput-throughputs))))
        # Sort the entries by size
        keys = ['sizes'] + [k for k in entry.keys() if k is not 'sizes']
        sortedEntries = zip(*sorted(zip(*[entry[k] for k in keys]), key=lambda pair: pair[0]))
        return dict({'name':os.path.splitext(os.path.basename(case))[0],'app':app},**dict(zip(keys,sortedEntries)))


    def readData(self):
        self.cases = []
        for case in self.args['cases']:
            for app in self.args['app']:
                try:
                    self.cases.append( self.readCaseData(case,app) )
                except IOError as e:
                    print(e)
                    self.args['cases'].remove(case)


    def printTable(self):
        for n,case in enumerate(self.cases):
            print(47*"-")
            print("Case: "+case['name']+" - "+case['app']+" - color:"+str(self.colors[n])+" - marker:"+str(self.markers[n]))
            print(47*"-")
            if self.args['totalThroughput']:
                sizeUnit = '(kB)'
                througputUnit = '(GB/s)'
            else:
                througputUnit = '(MB/s)'
                if 'BU' in case['app']:
                    sizeUnit = '(kB)'
                else:
                    sizeUnit = '(B)'
            print("Size %4s : Throughput %7s :      Rate (kHz)"%(sizeUnit,througputUnit))
            print(47*"-")
            try:
                for entry in zip(case['sizes'],case['throughputs'],case['rmsThroughputs'],case['rates'],case['rmsRates']):
                    print("%9d :  %6.1f +- %6.1f :  %5.1f +- %5.1f"%entry)
            except KeyError:
                pass
            print(47*"-")


    def fillColorsMarkers(self):
        if self.args['colors']:
            self.colors = list(OrderedDict.fromkeys(self.args['colors']+self.colors))
        if self.args['markers']:
            self.markers = list(OrderedDict.fromkeys(self.args['markers']+self.markers))


    def doIt(self,args):
        self.args = vars(args)
        #print(self.args)
        self.fillColorsMarkers()
        self.readData()
        self.printTable()
        self.createCanvas()
        self.createThroughputPad()
        self.createAuxPad()
        if not self.args['hideLineSpeed']:
            self.createLineSpeed()
        if not self.args['hideDate']:
            self.createDate()
        if not self.args['hideRate']:
            self.createRatePad()
            self.createRequirementLine()
            self.createRateGraphs()
        self.createLegend()
        self.createThroughputGraphs()
        self.fillLegend()
        self.createCMSpreliminary()
        self.canvas.Print(self.canvas.GetName()+".pdf")
        self.canvas.Print(self.canvas.GetName()+".png")
        self.canvas.Print(self.canvas.GetName()+".C")


if __name__ == "__main__":
    from argparse import ArgumentParser
    parser = ArgumentParser()
    plotScans = PlotScans()
    plotScans.addOptions(parser)
    plotScans.doIt( parser.parse_args() )
