#pylint: disable=no-init,invalid-name
import mantid,sys
from mantid.kernel import Direction, StringListValidator

try:
    from plotly.offline import plot
    import plotly.graph_objs as go
    have_plotly = True
except ImportError:
    have_plotly = False

class SavePlot1D(mantid.api.PythonAlgorithm):

    _wksp = None

    def category(self):
        """ Category
        """
        return "DataHandling\\Plots"

    def name(self):
        """ Algorithm name
        """
        return "SavePlot1D"

    def summary(self):
        return "Save 1D plots to a file"

    def checkGroups(self):
        return False

    def PyInit(self):
        #declare properties
        self.declareProperty(mantid.api.WorkspaceProperty("InputWorkspace","",mantid.kernel.Direction.Input),
                             "Workspace to plot")
        self.declareProperty(mantid.api.FileProperty('OutputFilename', '', action=mantid.api.FileAction.Save, extensions = ["png"]),
                             doc='Name of the image file to savefile.')
        if have_plotly:
            outputTypes = ['image', 'plotly', 'plotly-full']
        else:
            outputTypes = ['image']
        self.declareProperty('OutputType', 'image',
                             StringListValidator(outputTypes),
                             'Method for rendering plot')
        self.declareProperty("XLabel","",
                             "Label on the X axis. If empty, it will be taken from workspace")
        self.declareProperty("YLabel","",
                             "Label on the Y axis. If empty, it will be taken from workspace")
        self.declareProperty('Result', '', Direction.Output)

    def PyExec(self):
        self._wksp = self.getProperty("InputWorkspace").value
        outputType = self.getProperty('OutputType').value

        if outputType == 'image':
            result = self.saveImage()
        else:
            result = self.savePlotly(outputType == 'plotly-full')

        self.setProperty('Result', result)


    def getData(self, ws, wkspIndex):
        x=ws.readX(wkspIndex)
        y=ws.readY(wkspIndex)
        if x.size==y.size+1:
            x=(x[:-1]+x[1:])*0.5

        ax=ws.getAxis(1)
        if ax.isSpectra():
            label = ax.label(wkspIndex)
        else:
            LHS = a.title()
            if LHS == "":
                LHS = ax.getUnit().caption()
            label = LHS + " = " + str(float(ax.label(wkspIndex)))

        return (x, y, label)

    def getAxesLabels(self, ws, utf8=False):
        xlabel=self.getProperty('XLabel').value
        if xlabel=='':
            xaxis=ws.getAxis(0)
            if utf8:
                unitLabel = xaxis.getUnit().symbol().utf8()
            else:  # latex markup
                unitLabel= '$' + xaxis.getUnit().symbol().latex() + '$'
            xlabel=xaxis.getUnit().caption()+' ('+unitLabel+')'

        ylabel=self.getProperty("YLabel").value
        if ylabel=='':
            ylabel=ws.YUnit()
            if ylabel=='':
                ylabel = ws.YUnitLabel()

        return (xlabel, ylabel)


    def savePlotly(self, fullPage):

        if type(self._wksp)==mantid.api.WorkspaceGroup:
            raise RuntimeError('not ready for workspace groups')
            #for i in range(self._wksp.getNumberOfEntries()):
            #    plt.subplot(self._wksp.getNumberOfEntries(),1,i+1)
            #    self.doPlotImage(self._wksp.getItem(i))
        else:
            data = []
            for i in xrange(self._wksp.getNumberHistograms()):
                (x,y,label) = self.getData(self._wksp, i)
                data.append(go.Scatter(x=x, y=y, name=label))

            (xlabel, ylabel) = self.getAxesLabels(self._wksp, utf8=True)
            layout = go.Layout(yaxis={'title':ylabel},
                               xaxis={'title':xlabel})

            fig = go.Figure(data=data, layout=layout)

            if fullPage:
                filename = self.getProperty("OutputFilename").value
                plotly_args = {'filename':filename}
            else:  # just the div
                plotly_args = {'output_type':'div',
                               'include_plotlyjs':False}

            div = plot(fig, show_link=False, **plotly_args)

            if fullPage:
                return filename
            else:
                return str(div)

    def saveImage(self):
        ok2run=''
        try:
            import matplotlib
            from distutils.version import LooseVersion
            if LooseVersion(matplotlib.__version__)<LooseVersion("1.2.0"):
                ok2run='Wrong version of matplotlib. Required >= 1.2.0'
        except ImportError:
            ok2run='Problem importing matplotlib'
        if ok2run!='':
            raise RuntimeError(ok2run)
        matplotlib=sys.modules['matplotlib']
        matplotlib.use("agg")
        import matplotlib.pyplot as plt
        plt.figure()
        if type(self._wksp)==mantid.api.WorkspaceGroup:
            for i in range(self._wksp.getNumberOfEntries()):
                plt.subplot(self._wksp.getNumberOfEntries(),1,i+1)
                self.doPlotImage(self._wksp.getItem(i))
        else:
            self.doPlotImage(self._wksp)
        plt.tight_layout(1.08)
        plt.show()
        filename = self.getProperty("OutputFilename").value
        plt.savefig(filename,bbox_inches='tight')

        return filename


    def doPlotImage(self,ws):
        plt=sys.modules['matplotlib.pyplot']
        spectra=ws.getNumberHistograms()
        if spectra>10:
            mantid.kernel.logger.warning("more than 10 spectra to plot")
        prog_reporter=mantid.api.Progress(self,start=0.0,end=1.0,\
                    nreports=spectra)

        for j in range(spectra):
            (x, y, plotlabel) = self.getData(ws, j)

            plt.plot(x, y, label=plotlabel)

            (xlabel, ylabel) = self.getAxesLabels(ws)
            plt.xlabel(xlabel)
            plt.ylabel(ylabel)
            prog_reporter.report("Processing")

        if spectra>1 and spectra<=10:
            plt.legend()

mantid.api.AlgorithmFactory.subscribe(SavePlot1D)
