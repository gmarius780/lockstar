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

from lockstar_client.DoubleDitherLockClient import DoubleDitherLockClient

class ScopeCanvas(FigureCanvas):

    def __init__(self, parent=None, width=5, height=4, dpi=100):
        fig, (self.ax_in, self.ax_out) = plt.subplots(nrows=2, sharex=True, figsize=(width, height), dpi=dpi)
        # fig = Figure(figsize=(width, height), dpi=dpi)
        # self.axes = fig.add_subplot(111)
        super(ScopeCanvas, self).__init__(fig)


class DoubleDitherLockGUI(QtWidgets.QMainWindow):

    def __init__(self, client, refresh_rate, scope_buffer_size, scope_sampling_rate):
        super(DoubleDitherLockGUI, self).__init__()

        self.client = client
        self.refresh_rate = refresh_rate
        self.scope_buffer_size = scope_buffer_size
        self.scope_sampling_rate = scope_sampling_rate
        
        self.time_axis = np.arange(scope_buffer_size)/scope_sampling_rate

        #=== members
        self.p_one = self.i_one = self.d_one = self.setpoint_one = self.dither_amp_one = self.dither_offset_one = 0
        self.locked_one = False

        self.p_two = self.i_two = self.d_two = self.setpoint_two = self.dither_amp_two = self.dither_offset_two = 0
        self.locked_two = False

        #===setup gui

        #=== setup plot
        self.scope_canvas = ScopeCanvas(self, width=8, height=6, dpi=200)
        

        #=== setup form
        form_layout = QVBoxLayout()

        #==PID ONE
        pid_one_box = QGroupBox("PID one")

        pid_one_box_layout = QGridLayout()

        self.num_p_one = DoubleDitherLockGUI.create_numeric_widget('p', 0, 10, 0.01, self.pid_one_changed)
        pid_one_box_layout.addWidget(self.num_p_one, 1, 0, 1, 1)
        self.num_i_one = DoubleDitherLockGUI.create_numeric_widget('i', 0, 1e7, 1, self.pid_one_changed)
        pid_one_box_layout.addWidget(self.num_i_one, 1, 1, 1, 1)
        self.num_d_one = DoubleDitherLockGUI.create_numeric_widget('d', 0, 10, 0.01, self.pid_one_changed)
        pid_one_box_layout.addWidget(self.num_d_one, 1, 2, 1, 1)
        self.btn_set_pid_one = QPushButton('Set')
        self.btn_set_pid_one.clicked.connect(self.set_pid_one_clicked)
        self.btn_set_pid_one.setCheckable(False)
        pid_one_box_layout.addWidget(self.btn_set_pid_one, 1, 3, 1, 1)

        self.num_setpoint_one = DoubleDitherLockGUI.create_numeric_widget('set(mV)', -10000, 10000, 0.1, self.setpoint_one_changed)
        pid_one_box_layout.addWidget(self.num_setpoint_one, 1, 4, 1, 2)

        self.num_dither_amp_one = DoubleDitherLockGUI.create_numeric_widget('amp', -10, 10, 0.1, self.dither_one_changed)
        pid_one_box_layout.addWidget(self.num_dither_amp_one, 2, 0, 1, 2)
        self.num_dither_offset_one = DoubleDitherLockGUI.create_numeric_widget('offset', -10, 10, 0.1, self.dither_one_changed)
        pid_one_box_layout.addWidget(self.num_dither_offset_one, 2, 2, 1, 3)
        self.btn_set_dither_one = QPushButton('Set')
        self.btn_set_dither_one.clicked.connect(self.set_dither_one_clicked)
        self.btn_set_dither_one.setCheckable(False)
        self.btn_set_dither_one.setAutoDefault(True)
        pid_one_box_layout.addWidget(self.btn_set_dither_one, 2, 5, 1, 1)


        self.btn_lock_one = QPushButton('Lock')
        self.btn_lock_one.clicked.connect(self.lock_one_clicked)
        self.btn_lock_one.setCheckable(True)
        self.btn_lock_one.setAutoDefault(True)
        pid_one_box_layout.addWidget(self.btn_lock_one, 3, 0, 1, 6)

        pid_one_box.setLayout(pid_one_box_layout)
        form_layout.addWidget(pid_one_box)

        #==PID TWO
        pid_two_box = QGroupBox("PID two")
        
        pid_two_box_layout = QGridLayout()
        self.num_p_two = DoubleDitherLockGUI.create_numeric_widget('p', 0, 10, 0.01, self.pid_two_changed)
        pid_two_box_layout.addWidget(self.num_p_two, 5, 0, 1, 1)
        self.num_i_two = DoubleDitherLockGUI.create_numeric_widget('i', 0, 1e7, 1, self.pid_two_changed)
        pid_two_box_layout.addWidget(self.num_i_two, 5, 1, 1, 1)
        self.num_d_two = DoubleDitherLockGUI.create_numeric_widget('d', 0, 10, 0.01, self.pid_two_changed)
        pid_two_box_layout.addWidget(self.num_d_two, 5, 2, 1, 1)
        self.btn_set_pid_two = QPushButton('Set')
        self.btn_set_pid_two.clicked.connect(self.set_pid_two_clicked)
        self.btn_set_pid_two.setCheckable(False)
        pid_two_box_layout.addWidget(self.btn_set_pid_two, 5, 3, 1, 1)

        self.num_setpoint_two = DoubleDitherLockGUI.create_numeric_widget('set(mV)', -10000, 10000, 0.1, self.setpoint_two_changed)
        pid_two_box_layout.addWidget(self.num_setpoint_two, 5, 4, 1, 2)

        self.num_dither_amp_two = DoubleDitherLockGUI.create_numeric_widget('amp', -10, 10, 0.1, self.dither_two_changed)
        pid_two_box_layout.addWidget(self.num_dither_amp_two, 6, 0, 1, 2)
        self.num_dither_offset_two = DoubleDitherLockGUI.create_numeric_widget('offset', -10, 10, 0.1, self.dither_two_changed)
        pid_two_box_layout.addWidget(self.num_dither_offset_two, 6, 2, 1, 3)
        self.btn_set_dither_two = QPushButton('Set')
        self.btn_set_dither_two.clicked.connect(self.set_dither_two_clicked)

        self.btn_set_dither_two.setAutoDefault(True) #make clickable with enter key

        self.btn_set_dither_two.setCheckable(False)
        pid_two_box_layout.addWidget(self.btn_set_dither_two, 6, 5, 1, 1)


        self.btn_lock_two = QPushButton('Lock')
        self.btn_lock_two.clicked.connect(self.lock_two_clicked)
        self.btn_lock_two.setCheckable(True)
        self.btn_lock_two.setAutoDefault(True)
        pid_two_box_layout.addWidget(self.btn_lock_two, 7, 0, 1, 6)

        pid_two_box.setLayout(pid_two_box_layout)
        form_layout.addWidget(pid_two_box)

        #=== setup layout
        main_layout = QGridLayout()

        main_layout.addWidget( self.scope_canvas, 0, 0)
        
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
    def pid_one_changed(self):
        self.p_one = self.num_p_one.value()
        self.i_one = self.num_i_one.value()
        self.d_one = self.num_d_one.value()
    
    def set_pid_one_clicked(self):
        asyncio.run(client.set_pid_one(self.p_one, self.i_one, self.d_one))

    def setpoint_one_changed(self):
        self.setpoint_one = self.num_setpoint_one.value()

    def dither_one_changed(self):
        self.dither_amp_one = self.num_dither_amp_one.value()
        self.dither_offset_one = self.num_dither_offset_one.value()

    def lock_one_clicked(self):
        if self.locked_one:
            success = asyncio.run(client.unlock_one()).is_ACK()
        else:
            success = asyncio.run(client.lock_one()).is_ACK()
        
        if success:
            self.locked_one = not self.locked_one
            if self.locked_one:
                self.btn_lock_one.setText('Unlock')
            else:
                self.btn_lock_one.setText('Lock')
    def set_dither_one_clicked(self):
        asyncio.run(client.set_dither_one(self.dither_amp_one, self.dither_offset_one))


    def pid_two_changed(self):
        self.p_two = self.num_p_two.value()
        self.i_two = self.num_i_two.value()
        self.d_two = self.num_d_two.value()

    def set_pid_two_clicked(self):
        asyncio.run(client.set_pid_one(self.p_two, self.i_two, self.d_two))

    def setpoint_two_changed(self):
        self.setpoint_two = self.num_setpoint_two.value()

    def dither_two_changed(self):
        self.dither_amp_two = self.num_dither_amp_two.value()
        self.dither_offset_two = self.num_dither_offset_two.value()

    def lock_two_clicked(self):
        if self.locked_two:
            success = asyncio.run(client.unlock_two()).is_ACK()
        else:
            success = asyncio.run(client.lock_two()).is_ACK()
        
        if success:
            self.locked_two = not self.locked_two
            if self.locked_two:
                self.btn_lock_two.setText('Unlock')
            else:
                self.btn_lock_two.setText('Lock')
    def set_dither_two_clicked(self):
        asyncio.run(client.set_dither_two(self.dither_amp_two, self.dither_offset_two))

    def update_plot(self):
        # Drop off the first y element, append a new one.
        t1 = perf_counter()
        scope_data = asyncio.run(self.client.get_scope_data())
        # print(f'request took: {perf_counter() - t1:.1f} seconds')
        if type(scope_data) is dict:
            self.scope_canvas.ax_in.cla()  # Clear the canvas.
            self.scope_canvas.ax_out.cla()  # Clear the canvas.
            for trace_name in scope_data.keys():
                trace = np.array(scope_data[trace_name])
                
                if len(trace) > 0:
                    # print(f'{perf_counter() - self.last_successful_update:.1f}s')
                    self.last_successful_update = perf_counter()
                    if trace_name in ['in_one', 'in_two']:
                        self.scope_canvas.ax_in.plot(self.time_axis, trace, 'o', label=trace_name, ms=1)
                    else:
                        self.scope_canvas.ax_out.plot(self.time_axis, trace, 'o', label=trace_name, ms=1)
            
            # self.scope_canvas.ax_in.legend()
            # self.scope_canvas.ax_out.legend()
        else:
            
            print(scope_data)
        # Trigger the canvas to update and redraw.
        self.scope_canvas.draw()

if __name__ == "__main__":
    client = DoubleDitherLockClient('192.168.88.25', 10780, 1234)
    scope_sampling_rate = 20
    # scope_sampling_rate = 800
    # scope_buffer_length = 400
    scope_buffer_length = 10
    update_rate = 2

    print(asyncio.run(client.setup_scope(
        sampling_rate=scope_sampling_rate,
        nbr_samples_in_one=scope_buffer_length,
        nbr_samples_in_two=0,
        nbr_samples_out_one=scope_buffer_length,
        nbr_samples_out_two=0,
        adc_active_mode=True,
        double_buffer_mode=True
    )))
    print(asyncio.run(client.enable_scope()))
    print(asyncio.run(client.set_scope_sampling_rate(scope_sampling_rate)))
    asyncio.run(client.set_ch_one_output_limits(-10, 10))
    asyncio.run(client.set_ch_two_output_limits(-10, 10))

    app = QtWidgets.QApplication(sys.argv)
    w = DoubleDitherLockGUI(client, update_rate, scope_buffer_length, scope_sampling_rate)
    app.exec_()

