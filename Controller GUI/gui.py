# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""

import tkinter as tk
from twisted.internet import tksupport, reactor
from functools import partial
from PIL import Image, ImageTk
import numpy as np

class Host:
    def __init__(self, name, ip):
        self.name = name
        self.ip = ip
        self.status = "inactive"
        
    def connect(self):
        self.status = 'active'
    
    def show(self):
        pass


class ControlGUI(tk.Tk):
    def __init__(self):
        tk.Tk.__init__(self)
        self.winfo_toplevel().title("Lockstar Master Controller")
        self.winfo_toplevel().config(bg='white')
        self.geometry("540x540")
        self.resizable(False, False)
        self.iconphoto(False, tk.PhotoImage(file='guitar.png'))
        
        self.host_list = [Host("TP Power", 0), Host("Cavity 1 Lock", 1), Host("Cavity 2 Lock", 1), Host("Cavity 1 Probe", 1), Host("Cavity 2 Probe", 1)]
        
        self.canvas = tk.Canvas(self, width=540, height=540)
        self.canvas.bind("<Button-1>", self.canvas_clicked)
        self.canvas.pack()
        
        self.redraw_canvas()
        
    def redraw_canvas(self):
        # clear canvas
        self.canvas.delete(tk.ALL)
        
        # draw background
        background_image = Image.open("concert.jpg")
        self.background_image = ImageTk.PhotoImage(background_image)
        self.canvas.create_image(0,0, image=self.background_image, anchor = tk.NW)
        
        # draw HOSTS label
        self.canvas.create_rectangle(200,  20, 340, 60, fill="black", stipple="gray50", width=0)
        self.canvas.create_text(270, 40, text='HOSTS', font=('Broadway', 26), fill='white')
        
        green = '#83f68b'
        red = '#FF8B76'
        # list all hosts
        for i, host in enumerate(self.host_list):
            self.canvas.create_rectangle(95,  75+i*40, 445, 110+i*40, fill="black", stipple="gray75", width=0)
            self.canvas.create_text(270, 92+i*40, text=host.name, font=('Arial', 14), fill='white', anchor=tk.E)
            if host.status=='inactive':
                self.canvas.create_text(310, 92+i*40, text='inactive', font=('Arial', 14), fill=red)
                self.canvas.create_rectangle(350,  80+i*40, 440, 105+i*40, fill="black")
                self.canvas.create_text(395, 92+i*40, text='Connect', font=('Arial', 14), fill='white', activefill='silver')
            else:
                self.canvas.create_text(310, 92+i*40, text='active', font=('Arial', 14), fill=green)
                self.canvas.create_rectangle(350,  80+i*40, 440, 105+i*40, fill="black")
                self.canvas.create_text(395, 92+i*40, text='Show', font=('Arial', 14), fill='white', activefill='silver')
        
    def canvas_clicked(self, event):
        # was the event on a button
        coord_x = (event.x-350)/90.
        coord_y = (event.y-80)/40.
        if coord_x<0 or coord_x>1 or coord_y<0 or coord_y%1>25/40.:
            return
        # which host was clicked?
        host = int(np.floor(coord_y))
        if self.host_list[host].status == 'inactive':
            self.host_list[host].connect()
            self.redraw_canvas()
        else:
            self.host_list[host].show()
        
        

window = ControlGUI()

# Install the Reactor support
tksupport.install(window)

window.mainloop()