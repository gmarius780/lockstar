import sys
import matplotlib
matplotlib.use('Qt5Agg')


from PyQt5 import QtCore, QtWidgets

from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import numpy as np
import asyncio

from lockstar_client.CavityLockClient import CavityLockClient

class MplCanvas(FigureCanvas):

    def __init__(self, parent=None, width=5, height=4, dpi=100):
        fig = Figure(figsize=(width, height), dpi=dpi)
        self.axes = fig.add_subplot(111)
        super(MplCanvas, self).__init__(fig)


class ScopeViewer(QtWidgets.QMainWindow):

    def __init__(self, client, refresh_rate, scope_buffer_size, scope_sampling_rate):
        super(ScopeViewer, self).__init__()

        self.client = client
        self.refresh_rate = refresh_rate
        self.scope_buffer_size = scope_buffer_size
        self.scope_sampling_rate = scope_sampling_rate
        
        self.time_axis = np.arange(scope_buffer_size)/scope_sampling_rate

        #setup gui
        self.canvas = MplCanvas(self, width=10, height=8, dpi=300)
        self.setCentralWidget(self.canvas)

        self.update_plot()
        self.show()

        # Setup a timer to trigger the redraw by calling update_plot.
        self.timer = QtCore.QTimer()
        self.timer.setInterval(int(1000/refresh_rate))
        self.timer.timeout.connect(self.update_plot)
        self.timer.start()

    def update_plot(self):
        # Drop off the first y element, append a new one.
        scope_data = asyncio.run(self.client.get_scope_data())
        
        if type(scope_data) is dict:
            self.canvas.axes.cla()  # Clear the canvas.
            for trace_name in scope_data.keys():
                trace = np.array(scope_data[trace_name])
                print(np.sum(trace==0))
                if len(trace) > 0:
                    self.canvas.axes.plot(self.time_axis, trace, label=trace_name)
            
            self.canvas.axes.legend()
        else:
            print(scope_data)
        # Trigger the canvas to update and redraw.
        self.canvas.draw()

if __name__ == "__main__":
    client = CavityLockClient('192.168.88.25', 10780, 1234)
    # scope_sampling_rate = 2000
    scope_sampling_rate = 200
    scope_buffer_length = 200
    # scope_buffer_length = 1000
    update_rate = 2

    # print(asyncio.run(client.setup_scope(
    #     sampling_rate=scope_sampling_rate,
    #     sample_in_one=True,
    #     sample_in_two=True,
    #     sample_out_one=True,
    #     sample_out_two=True,
    #     buffer_length=scope_buffer_length,
    #     adc_active_mode=True
    # )))
    # print(asyncio.run(client.enable_scope()))

    app = QtWidgets.QApplication(sys.argv)
    w = ScopeViewer(client, update_rate, scope_buffer_length, scope_sampling_rate)
    app.exec_()

