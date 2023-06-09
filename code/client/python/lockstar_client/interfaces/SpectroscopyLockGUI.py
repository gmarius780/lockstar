import sys
import matplotlib
import matplotlib.pyplot as plt
matplotlib.use('Qt5Agg')


from PyQt5 import QtCore, QtWidgets
from PyQt5.QtWidgets import QGridLayout, QPushButton, QWidget, QLabel, QDoubleSpinBox, QGroupBox, QVBoxLayout

from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import numpy as np
import asyncio

from time import perf_counter

from lockstar_client.SpectroscopyLockClient import SpectroscopyLockClient

class ScopeCanvas(FigureCanvas):

    def __init__(self, parent=None, width=5, height=4, dpi=100):
        self.fig, (self.ax_in, self.ax_out) = plt.subplots(nrows=2, sharex=True, figsize=(width, height), dpi=dpi)
        super(ScopeCanvas, self).__init__(self.fig)


class SpectroscopyLockGUI(QtWidgets.QMainWindow):

    def __init__(self, client, refresh_rate, scope_buffer_size):
        super(SpectroscopyLockGUI, self).__init__()

        self.client = client
        self.refresh_rate = refresh_rate
        self.scope_buffer_size = scope_buffer_size
        
        self.time_axis = np.arange(scope_buffer_size)/(scope_buffer_size*refresh_rate)

        self.t_last_draw = 0
        self.t_last_fail = 0

        self.line_ch_in = None
        self.line_ch_out = None

        #=== members
        self.p = self.i = self.d = 0
        self.locked = False

        #===setup gui

        #=== setup plot
        self.scope_canvas = ScopeCanvas(self, width=8, height=6, dpi=200)
        

        #=== setup form
        form_layout = QVBoxLayout()

        #==PID ONE
        pid_box = QGroupBox("PID")

        pid_box_layout = QGridLayout()

        self.num_p = SpectroscopyLockGUI.create_numeric_widget('p', 0, 10, 0.01, self.pid_changed)
        pid_box_layout.addWidget(self.num_p, 1, 0, 1, 1)
        self.num_i = SpectroscopyLockGUI.create_numeric_widget('i', 0, 1e7, 1, self.pid_changed)
        pid_box_layout.addWidget(self.num_i, 1, 1, 1, 1)
        self.num_d = SpectroscopyLockGUI.create_numeric_widget('d', 0, 10, 0.01, self.pid_changed)
        pid_box_layout.addWidget(self.num_d, 1, 2, 1, 1)
        self.btn_set_pid = QPushButton('Set')
        self.btn_set_pid.clicked.connect(self.set_pid_clicked)
        self.btn_set_pid.setCheckable(False)
        pid_box_layout.addWidget(self.btn_set_pid, 1, 3, 1, 1)


        self.num_dither_amp = SpectroscopyLockGUI.create_numeric_widget('amp', -10, 10, 0.1, self.dither_changed)
        pid_box_layout.addWidget(self.num_dither_amp, 2, 0, 1, 2)
        self.num_dither_offset = SpectroscopyLockGUI.create_numeric_widget('offset', -10, 10, 0.1, self.dither_changed)
        pid_box_layout.addWidget(self.num_dither_offset, 2, 2, 1, 3)
        self.btn_set_dither = QPushButton('Set')
        self.btn_set_dither.clicked.connect(self.set_dither_clicked)
        self.btn_set_dither.setCheckable(False)
        self.btn_set_dither.setAutoDefault(True)
        pid_box_layout.addWidget(self.btn_set_dither, 2, 5, 1, 1)


        self.btn_lock = QPushButton('Lock')
        self.btn_lock.clicked.connect(self.lock_clicked)
        self.btn_lock.setCheckable(True)
        self.btn_lock.setAutoDefault(True)
        pid_box_layout.addWidget(self.btn_lock, 3, 0, 1, 6)

        pid_box.setLayout(pid_box_layout)
        form_layout.addWidget(pid_box)


        #=== setup layout
        main_layout = QGridLayout()

        main_layout.addWidget(self.scope_canvas, 0, 0)
        
        main_layout.addLayout(form_layout, 0, 1)

        widget = QWidget()
        widget.setLayout(main_layout)
        self.setCentralWidget(widget)

        
        self.last_successful_update = 0
        self.update_plot()
        self.show()
        #===setup events
        
        

        # Setup a timer to trigger the redraw by calling update_plot.
        self.timer = QtCore.QTimer()
        self.timer.setInterval(int(1000/refresh_rate))
        self.timer.timeout.connect(self.update_plot)
        self.timer.start()

    @staticmethod
    def create_numeric_widget(label, min_val, max_val, increment, value_changed):
        widget = QDoubleSpinBox()

        widget.setMinimum(min_val)
        widget.setMaximum(max_val)

        widget.setPrefix(f"{label}: ")
        widget.setSingleStep(increment)
        widget.valueChanged.connect(value_changed)

        return widget

    #==== GUI callback methods
    def pid_changed(self):
        self.p = self.num_p.value()
        self.i = self.num_i.value()
        self.d = self.num_d.value()
    
    def set_pid_clicked(self):
        asyncio.run(client.set_pid(self.p, self.i, self.d))

    def dither_changed(self):
        self.dither_amp = self.num_dither_amp.value()
        self.dither_offset = self.num_dither_offset.value()

    def lock_clicked(self):
        if self.locked:
            success = asyncio.run(client.unlock()).is_ACK()
        else:
            success = asyncio.run(client.lock()).is_ACK()
        
        if success:
            self.locked = not self.locked
            if self.locked:
                self.btn_lock.setText('Unlock')
            else:
                self.btn_lock.setText('Lock')

    def set_dither_clicked(self):
        asyncio.run(client.set_dither(self.dither_amp, self.dither_offset))

    def update_plot(self):
        # Drop off the first y element, append a new one.
        if self.locked: #the module does not automatically enable the scope when it is lock --> save resources
            asyncio.run(self.client.enable_scope())
        t1 = perf_counter()
        scope_data = asyncio.run(self.client.get_scope_data())
        print(f'request took: {perf_counter() - t1:.5f} seconds')
        if type(scope_data) is dict:
            t1 = perf_counter()
            for trace_name in scope_data.keys():
                trace = np.array(scope_data[trace_name])
                
                if len(trace) > 0:
                    # print(f'{perf_counter() - self.last_successful_update:.1f}s')
                    self.last_successful_update = perf_counter()
                    if trace_name == 'in_one':
                        if self.line_ch_in is None:
                            self.line_ch_in, = self.scope_canvas.ax_in.plot(self.time_axis, trace, 'o', label=trace_name, ms=1)
                            self.scope_canvas.ax_in.legend()
                        else:
                            self.line_ch_in.set_ydata(trace)
                            ymin = np.min(trace)
                            ymin = 0.9*ymin if ymin >= 0 else 1.1*ymin
                            ymax = np.max(trace)
                            ymax = 1.1*ymax if ymax >= 0 else 0.9*ymax
                            self.scope_canvas.ax_in.set_ylim((ymin, ymax))
                    elif trace_name == 'out_one':
                        if self.line_ch_out is None:
                            self.line_ch_out, = self.scope_canvas.ax_out.plot(self.time_axis, trace, 'o', label=trace_name, ms=1)
                            self.scope_canvas.ax_out.legend()
                        else:
                            self.line_ch_out.set_ydata(trace)
                            ymin = np.min(trace)
                            ymin = 0.9*ymin if ymin >= 0 else 1.1*ymin
                            ymax = np.max(trace)
                            ymax = 1.1*ymax if ymax >= 0 else 0.9*ymax
                            self.scope_canvas.ax_out.set_ylim((ymin, ymax))
                    
                    self.scope_canvas.fig.canvas.draw()
                    self.scope_canvas.fig.canvas.flush_events()
                    
            print(f'drawing took: {perf_counter() - t1:.5f} seconds')
            print(f'fps: {1/(perf_counter() - self.t_last_draw)}')
            self.t_last_draw = perf_counter()
            # self.scope_canvas.ax_in.legend()
            # self.scope_canvas.ax_out.legend()
        else:
            print(f'fails per second: {1/(perf_counter() - self.t_last_fail)}')
            self.t_last_fail = perf_counter()
        # Trigger the canvas to update and redraw.
        

if __name__ == "__main__":
    client = SpectroscopyLockClient('192.168.88.25', 10780, 1234)
    scope_buffer_length = 400
    dither_frequency = 10
    print(asyncio.run(client.setup_scope(
        sampling_rate=1000,
        nbr_samples_in_one=scope_buffer_length,
        nbr_samples_in_two=0,
        nbr_samples_out_one=scope_buffer_length,
        nbr_samples_out_two=0,
        adc_active_mode=False,
        double_buffer_mode=True
    )))
    print(asyncio.run(client.set_dither_frq(dither_frequency)))
    # asyncio.run(client.set_ch_one_output_limits(-10, 10))
    # asyncio.run(client.set_ch_two_output_limits(-10, 10))

    app = QtWidgets.QApplication(sys.argv)
    w = SpectroscopyLockGUI(client, dither_frequency, scope_buffer_length)
    app.exec_()

