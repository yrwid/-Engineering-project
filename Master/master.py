
import wx
import sys
import serial.tools.list_ports
import threading
import time

def ReadingThread(self):
    while True:
        if self.ser.in_waiting != 0:
            print(self.ser.read(19))
        time.sleep(1)


class MyFrame(wx.Frame):
    def __init__(self):
        super().__init__(parent=None, title='Serial Comands V1.0', size=(550,300), style=wx.DEFAULT_FRAME_STYLE ^ wx.RESIZE_BORDER)
        self.dont_show = False
        self.connected = False

        self.SetBackgroundColour(wx.Colour(0, 25, 51))
        self.panel = wx.Panel(self)
        vBox = wx.BoxSizer(wx.VERTICAL)
        style = wx.TE_MULTILINE | wx.TE_READONLY | wx.HSCROLL
        self.log = wx.TextCtrl(self.panel, wx.ID_ANY, size=(500,200),
                          style=style)
        self.log.SetBackgroundColour(wx.Colour(64, 64, 64))
        self.log.SetForegroundColour(wx.Colour(255, 255, 255))
        vBox.Add(self.log, 0, wx.ALL | wx.CENTER, 5)
        self.hBox = wx.BoxSizer(wx.HORIZONTAL)
        self.text_ctrl = wx.TextCtrl(self.panel, size =(175, 25))
        self.text_ctrl.SetValue("COM6")
        self.hBox.Add(self.text_ctrl, 0, wx.ALL | wx.LEFT, 5)
        self.ButtonCreates()
        vBox.Add(self.hBox, 1, wx.ALL | wx.ALIGN_CENTER)
        self.panel.SetSizer(vBox)
        self.Show()

        # redirect text here
        sys.stdout = self.log


    def SerialConnect(self):
        ports = serial.tools.list_ports.comports()
        if len(ports) == 0:
            print("None COM available, Plug in device ")
            self.plugIn = False
        else:
            print("Available Ports: ")
            for port, desc, hwid in sorted(ports):
                print("{} | {}".format(port, desc))
            self.plugIn = True


    def ShowDialog(self, message='Default Message'):
        dlg = wx.RichMessageDialog(self, f"{message}")
        dlg.ShowModal()  # return value ignored as we have "Ok" only anyhow


    def ButtonCreates(self):
        #create first button
        self.my_btn = wx.Button(self.panel, label='Connect')
        self.my_btn.Bind(wx.EVT_BUTTON, self.OnPress1)
        self.hBox.Add(self.my_btn, 0, wx.ALL | wx.ALIGN_LEFT, 5)

        my_btn = wx.Button(self.panel, label='Command 1')
        my_btn.Bind(wx.EVT_BUTTON, self.OnPress2)
        self.hBox.Add(my_btn, 0, wx.ALL | wx.ALIGN_LEFT, 5)

        my_btn = wx.Button(self.panel, label='Command 2')
        my_btn.Bind(wx.EVT_BUTTON, self.OnPress3)
        self.hBox.Add(my_btn, 0, wx.ALL | wx.ALIGN_LEFT, 5)


    def OnPress1(self, event):
        if self.plugIn == False:
            self.SerialConnect()

        elif self.connected == True:
            command = b'\x42\x42\x42\x0a'  # \CMD 1 i
            print(f'command sended: {command}')
            x = self.ser.write(command)

        elif self.plugIn and self.connected == False:
            value = self.text_ctrl.GetValue() #:010300000001
            self.ser = serial.Serial(f"{value}", 115200)
            x = threading.Thread(target=ReadingThread, args=(self,), daemon=True)
            x.start()
            print("Connedcted !")
            print("Reading Thread active !")
            self.my_btn.SetLabel("Send")
            self.connected = True


    def OnPress2(self, event):
        if self.connected:
            #                                
            #                       Read Coils     :01 01 00 00 00 01 FD (CRLF)
            #            :   0    1    0    1   0     0   0     0   0    0     0    1    F    D    
            # \CMD 1 || 3a  30   31   30   31   30   30   30   30   30   30   30   31    46  44   CR   LF
            #command =[0x3a,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x31,0x46,0x44,0x0d,0x0a]

            #              Read Discret Inputs     :01 02 00 00 00 01 FC (CRLF)
            #            :   0    1    0    2   0     0   0     0   0    0     0    1    F    C    
            # \CMD 1 || 3a  30   31   30   32   30   30   30   30   30   30   30   31    46  42   CR   LF
            #command =[0x3a,0x30,0x31,0x30,0x32,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x31,0x46,0x43,0x0d,0x0a]

             #            Read holding register    :01 03 00 00 00 01 FB (CRLF)
            #            :   0    1    0    3   0     0   0     0   0    0     0    1    F    B    
            # \CMD 1 || 3a  30   31   30   33   30   30   30   30   30   30   30   31    46  42   CR   LF
            #command =[0x3a,0x30,0x31,0x30,0x33,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x31,0x46,0x42,0x0d,0x0a]

            #             Read Input Registers     :01 04 00 00 00 01  F (CRLF)
            #            :   0    1    0    4   0     0   0     0   0    0     0    1    F    A    
            # \CMD 1 || 3a  30   31   30   34   30   30   30   30   30   30   30   31    46  41   CR   LF
            command =[0x3a,0x30,0x31,0x30,0x34,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x31,0x46,0x41,0x0d,0x0a]

            #             Write single Registers     :01 06 00 00 00 01  F8 (CRLF)
            #            :   0    1    0    6   0     0   0     0   0    0     0    1    F    8   
            # \CMD 1 || 3a  30   31   30   36   30   30   30   30   30   30   30   31    46  38   CR   LF
            #command =[0x3a,0x30,0x31,0x30,0x36,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x31,0x46,0x38,0x0d,0x0a]

            print(f'command Read: {command}')
            x = self.ser.write(command)
        else:
            self.SerialConnect()


    def OnPress3(self, event):
        if self.connected: 
            #             Write single Registers     :01 06 00 00 00 01  F8 (CRLF)
            #            :   0    1    0    6   0     0   0     0   0    0     0    1    F    8   
            # \CMD 2 || 3a  30   31   30   36   30   30   30   30   30   30   30   31    46  38   CR   LF
            command =[0x3a,0x30,0x31,0x30,0x36,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x31,0x46,0x38,0x0d,0x0a]
            print(f'command Write: {command}')
            x = self.ser.write(command)
        else:
            self.SerialConnect()


if __name__ == '__main__':
    app = wx.App()
    frame = MyFrame()
    frame.ShowDialog("Select available COM and write it down")
    frame.SerialConnect()

    app.MainLoop()

#import minimalmodbus
#import serial

#command = 1 #"\x01\x03\x02\x00\x01\x25\xCA"
#ser = serial.Serial('COM6', 115200)  # open serial port
#ser.write(command)     # write a string
#x = ser.read(7)
#print(x)
 # port name, slave address (in decimal)
#instrument = minimalmodbus.Instrument('COM6', 1, debug=True) 

#instrument.serial.baudrate = 115200         # Baud
#instrument.serial.bytesize = 8
# instrument.serial.parity   = serial.PARITY_NONE
#instrument.serial.stopbits = 1
#instrument.serial.timeout  = 0.1          # seconds

## Read temperature (PV = ProcessValue) ##
# Registernumber, number of decimals
#temperature = instrument.read_long(registeraddress = 1,functioncode=3)  
#print(temperature)

## Change temperature setpoint (SP) ##
# NEW_TEMPERATURE = 95
# Registernumber, value, number of decimals for storage
# instrument.write_register(24, NEW_TEMPERATURE, 1)  