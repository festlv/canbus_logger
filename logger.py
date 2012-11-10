import wx
import serial_interface

import process
import multiprocessing
import Queue

from ObjectListView import ObjectListView, ColumnDefn, BatchedUpdate


class MainWindow(wx.Frame):
    _process = None
    _paused = False
    _paused_batch = []
    _ignore = False
    _ignored_messages = []
    _message_occurences = {}

    def __init__(self, parent, title):
        wx.Frame.__init__(self, parent, title=title, size=(200,100))
        self.serial_interface = serial_interface.SerialInterface()

        self.CreateStatusBar() # A Statusbar in the bottom of the window
        

        self.main_sizer = wx.BoxSizer(wx.VERTICAL)
        self.create_tools()
        self.create_filters()

        self.message_list = ObjectListView(self, style=wx.LC_REPORT|wx.SUNKEN_BORDER)
        self.batched_message_list = BatchedUpdate(self.message_list, 1)
        self.message_list.SetFilter(self.filter)
        self.main_sizer.Add(self.message_list, 1, flag=wx.EXPAND|wx.ALL, border=5)


        self.SetSizerAndFit(self.main_sizer)
        self.SetAutoLayout(True)
        self.Layout()
        
        
        self.init_message_list()
        self.message_list.SortBy(0, False)
        self.Bind (wx.EVT_IDLE, self.on_idle)
        self.Bind(wx.EVT_CLOSE, self.on_close)

        self.Show()

    def create_tools(self):
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

        self.pause_button = wx.Button(self, label="Pause")
        tools_sizer.Add(self.pause_button, flag=wx.EXPAND)
        
        self.save_button = wx.Button(self, label="Save")
        tools_sizer.Add(self.save_button, flag=wx.EXPAND)
        self.main_sizer.Add(tools_sizer, 0)

        self.update_port_selection()

        self.Bind(wx.EVT_BUTTON, self.on_connect, self.connect_button)
        self.Bind(wx.EVT_BUTTON, self.on_save, self.save_button)
        self.Bind(wx.EVT_BUTTON, self.on_pause, self.pause_button)

    def create_filters(self):
        b = 5
        self.filter_sizer = wx.BoxSizer(wx.HORIZONTAL)
        self.filter_sizer.Add(wx.StaticText(self, label="Filter (ID or data):", pos=(10, 5)), flag=wx.EXPAND|wx.ALL, border=b)
        self.filter_textbox = wx.TextCtrl(self)
        self.filter_sizer.Add(self.filter_textbox, flag=wx.EXPAND)
        self.main_sizer.Add(self.filter_sizer)

        self.ignore_button = wx.Button(self, label="Start ignore")
        self.filter_sizer.Add(self.ignore_button, flag=wx.EXPAND)
        self.Bind(wx.EVT_BUTTON, self.on_ignore, self.ignore_button)

        self.clear_ignore_button = wx.Button(self, label="Clear ignore list")
        self.filter_sizer.Add(self.clear_ignore_button, flag=wx.EXPAND)
        self.Bind(wx.EVT_BUTTON, self.on_clear_ignore, self.clear_ignore_button)

        self.Bind(wx.EVT_TEXT, self.on_filter_update, self.filter_textbox)

        self.filter_sizer.Add(wx.StaticText(self, label="Number of occurences:", pos=(10, 5)), flag=wx.EXPAND|wx.ALL, border=b)
        self.filter_sizer.Add(wx.StaticText(self, label=">=", pos=(10, 5)), flag=wx.EXPAND|wx.ALL, border=b)
        self.gte_occurences_text = wx.TextCtrl(self)
        self.filter_sizer.Add(self.gte_occurences_text, flag=wx.EXPAND)
        self.Bind(wx.EVT_TEXT, self.on_filter_update, self.gte_occurences_text)

        self.filter_sizer.Add(wx.StaticText(self, label="<=", pos=(10, 5)), flag=wx.EXPAND|wx.ALL, border=b)
        
        self.lte_occurences_text = wx.TextCtrl(self)
        self.filter_sizer.Add(self.lte_occurences_text, flag=wx.EXPAND)
        self.Bind(wx.EVT_TEXT, self.on_filter_update, self.lte_occurences_text)



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


    def on_ignore(self, event):
        if self._ignore:
            self._ignore = False
            self.ignore_button.SetLabel("Start ignore")
        else:
            self._ignore = True
            self.ignore_button.SetLabel("Stop ignore")

    def on_clear_ignore(self, event):
        self._ignored_messages = []


    def on_filter_update(self, event):
        self.batched_message_list.RepopulateList()

    def filter(self, object_list):
        val = self.filter_textbox.GetValue()
        try:
            gte = int(self.gte_occurences_text.GetValue())
        except ValueError:
            gte = 0

        try:
            lte = int(self.lte_occurences_text.GetValue())
        except ValueError:
            lte = 0

        res = []
        for m in object_list:
            mv = m.id + m.data
            if mv in self._ignored_messages:
                continue
            matches_filter = len(val)==0 or (m.id.startswith(val) or m.data.startswith(val))

            matches_gte = (mv not in self._message_occurences) or self._message_occurences[mv]>=gte
            matches_lte = (mv not in self._message_occurences) or lte>0 or self._message_occurences[mv]<=lte
            matches_occurences = matches_gte and matches_lte

            if matches_filter and matches_occurences:
                res.append(m)

        return res

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
                    self._process = process.SerialProcess(serial_port, bitrate, self._queue)
                    self.connect_button.SetLabel("Disconnect")
                except serial_interface.SerialException, e:
                    wx.MessageBox("Cannot open serial port! "+e.message, "Error", wx.OK | wx.ICON_ERROR)
            else:
                wx.MessageBox("Please select serial port and bitrate.", "Error", wx.OK | wx.ICON_ERROR)
        else:
            self._queue.put("stop")
            self.connect_button.SetLabel("Connect")
            self._process = None

    def on_pause(self, event):
        if self._paused:
            self._paused = False
            self.batched_message_list.AddObjects(self._paused_batch)
            self._paused_batch = []
            self.pause_button.SetLabel("Pause")
        else:
            self.pause_button.SetLabel("Play")
            self._paused = True

    def on_idle(self, event):
        if hasattr(self, '_queue'):
            try:
                message = self._queue.get(False)
                if hasattr(message, 'timestamp'):
                    message_val = message.id + message.data

                    if self._ignore:
                        self._ignored_messages.append(message_val)

                    if message_val in self._message_occurences:
                        self._message_occurences[message_val]+=1
                    else:
                        self._message_occurences[message_val]=1


                    if self._paused:
                        self._paused_batch.append(message)
                    else:
                        self.batched_message_list.AddObject(message)

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