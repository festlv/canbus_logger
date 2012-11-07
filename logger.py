import wx
import serial_interface

import process
import multiprocessing
import Queue

from ObjectListView import ObjectListView, ColumnDefn


class MainWindow(wx.Frame):
    _process = None

    def __init__(self, parent, title):
        wx.Frame.__init__(self, parent, title=title, size=(200,100))
        self.serial_interface = serial_interface.SerialInterface()

        self.CreateStatusBar() # A Statusbar in the bottom of the window
        

        tools_sizer = wx.BoxSizer(wx.HORIZONTAL)
        b=5
        tools_sizer.Add(wx.StaticText(self, label="Serial port:", pos=(10, 5)), flag=wx.EXPAND|wx.ALL, border=b)
        
        self.serial_combobox = wx.ComboBox(self, style=wx.CB_DROPDOWN)
        tools_sizer.Add(self.serial_combobox)

        tools_sizer.Add(wx.StaticText(self, label="CAN baud rate (kbps):"), flag=wx.EXPAND|wx.ALL, border=b)
        self.bitrate_combobox = wx.ComboBox(self, style=wx.CB_DROPDOWN, 
            choices=[str(i) for i in self.serial_interface.supported_can_bitrates])
        
        tools_sizer.Add(self.bitrate_combobox, flag=wx.EXPAND)

        self.connect_button = wx.Button(self, label="Connect")
        tools_sizer.Add(self.connect_button, flag=wx.EXPAND)
        
        self.save_button = wx.Button(self, label="Save")
        tools_sizer.Add(self.save_button, flag=wx.EXPAND)

        self.main_sizer = wx.BoxSizer(wx.VERTICAL)
        self.main_sizer.Add(tools_sizer, 0)


        self.message_list = ObjectListView(self, style=wx.LC_REPORT|wx.SUNKEN_BORDER)

        self.main_sizer.Add(self.message_list, 1, flag=wx.EXPAND|wx.ALL, border=5)


        self.SetSizerAndFit(self.main_sizer)
        self.SetAutoLayout(True)
        self.Layout()
        
        self.update_port_selection()
        self.init_message_list()

        self.Bind (wx.EVT_IDLE, self.on_idle)
        self.Bind(wx.EVT_CLOSE, self.on_close)
        self.Bind(wx.EVT_BUTTON, self.on_connect, self.connect_button)
        self.Bind(wx.EVT_BUTTON, self.on_save, self.save_button)
        self.Show()

    def init_message_list(self):
        self.message_list.SetColumns([
            ColumnDefn("Timestamp", "left", 300, "timestamp"),
            ColumnDefn("ID", "left", 200, "id"),
            ColumnDefn("Data", "left", 250, "data", minimumWidth=250),
            ]

            )

    def update_port_selection(self):
        """
        Updates serial port selection with available serial ports
        """
    	self.serial_combobox.Clear()
    	self.serial_combobox.AppendItems(self.serial_interface.scan())

    def on_connect(self, event):
        """Executed on click of Connect button. 
        Spawns a new thread which listens for messages on serial port, if no connection is present.
        If already connected, stops existing thread (disconnects).
        """

        if not self._process:
            serial_port = self.serial_combobox.GetStringSelection()
            bitrate = self.bitrate_combobox.GetStringSelection()

            if serial_port and bitrate:
                try:
                    self._queue = multiprocessing.Queue()
                    self._process = process.SerialProcess(self, serial_port, bitrate, self._queue)
                    self.connect_button.SetLabel("Disconnect")
                except serial_interface.SerialException, e:
                    wx.MessageBox("Cannot open serial port! "+e.message, "Error", wx.OK | wx.ICON_ERROR)
            else:
                wx.MessageBox("Please select serial port and bitrate.", "Error", wx.OK | wx.ICON_ERROR)
        else:
            self._queue.put("stop")
            self.connect_button.SetLabel("Connect")
            self._process = None


    def on_idle(self, event):
        if hasattr(self, '_queue'):
            try:
                message = self._queue.get(False)
                if hasattr(message, 'timestamp'):
                    self.message_list.AddObject(message)

                elif message=="stop":
                    self._queue.put("stop")
            except Queue.Empty:
                pass
            event.RequestMore()
        

    def on_close(self, event):
        if self._process:
            self._queue.put("stop")
            self._process.join()
        self.Destroy()


    def on_save(self, event):
        dlg = wx.FileDialog(
            self, message="Save file as ...",
            defaultFile="", style=wx.SAVE, wildcard="All files (*.*)|*.*"
            )
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            try:
                f = open(path,'w')
                f.write("#Timestamp, #ID, #Data\n")
                for o in self.message_list.GetObjects():
                    f.write("%s, %s, %s\n" % (o.timestamp, o.id, o.data))
                f.close()
            except IOError:
                wx.MessageBox("Cannot save file! \n"+e.message, "Error", wx.OK | wx.ICON_ERROR)
        dlg.Destroy()

class MyApp(wx.App):
    def OnInit(self):
        frame = MainWindow(None, "CANBus logger")
        frame.Show(True)
        self.SetTopWindow(frame)
        return True



if __name__ == '__main__':
    app = MyApp(0)
    app.MainLoop()