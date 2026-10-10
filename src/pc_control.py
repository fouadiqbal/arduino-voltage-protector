"""Optional real-Arduino USB serial panel. Install: python -m pip install pyserial"""
import time
import tkinter as tk
from tkinter import ttk, messagebox
try:
    import serial
    from serial.tools import list_ports
except ImportError:
    raise SystemExit('Install the optional dependency: python -m pip install pyserial')

class Panel:
    def __init__(self, root):
        self.root, self.port, self.buffer = root, None, bytearray()
        self.last_rx = 0
        root.title('Arduino Uno | Voltage Protector')
        root.geometry('820x580')
        box = ttk.Frame(root, padding=22); box.pack(fill='both', expand=True)
        ttk.Label(box, text='Voltage Protector', font=('Segoe UI', 23, 'bold')).pack(anchor='w')
        ttk.Label(box, text='USB serial control - low-voltage demonstrator only').pack(anchor='w', pady=(0,15))
        bar = ttk.Frame(box); bar.pack(fill='x')
        self.port_name = tk.StringVar()
        self.ports = ttk.Combobox(bar, textvariable=self.port_name, width=18)
        self.ports.pack(side='left')
        ttk.Button(bar, text='Refresh ports', command=self.refresh).pack(side='left', padx=5)
        ttk.Button(bar, text='Connect', command=self.connect).pack(side='left')
        ttk.Button(bar, text='Disconnect', command=self.disconnect).pack(side='left', padx=5)
        self.status = tk.StringVar(value='DISCONNECTED - load state unknown')
        ttk.Label(box, textvariable=self.status, font=('Segoe UI',14,'bold')).pack(anchor='w', pady=20)
        self.values={}
        for key, caption, default in [('VMIN','Minimum voltage (V)',180),('VMAX','Maximum voltage (V)',250),
                                       ('IMAX','Current limit (mA)',5000),('DELAY','Restart delay (ms)',3000)]:
            row=ttk.Frame(box);row.pack(fill='x', pady=3)
            ttk.Label(row,text=caption,width=30).pack(side='left')
            value=tk.StringVar(value=str(default)); self.values[key]=value
            ttk.Entry(row,textvariable=value,width=12).pack(side='left')
            ttk.Button(row,text='Apply',command=lambda k=key:self.send('SET '+k+' '+self.values[k].get())).pack(side='left',padx=8)
        self.settings=tk.StringVar(value='Device settings: waiting for telemetry')
        ttk.Label(box,textvariable=self.settings).pack(anchor='w',pady=10)
        row=ttk.Frame(box);row.pack(fill='x')
        ttk.Button(row,text='GET status',command=lambda:self.send('GET')).pack(side='left')
        ttk.Button(row,text='Safe RESET',command=lambda:self.send('RESET')).pack(side='left',padx=8)
        self.log=tk.Text(box,height=8,font=('Consolas',10));self.log.pack(fill='both',expand=True,pady=10)
        ttk.Label(box,text='Settings are held in RAM and revert to defaults after Arduino reset.').pack(anchor='w')
        self.refresh();root.protocol('WM_DELETE_WINDOW',self.close);root.after(50,self.poll)
    def refresh(self):
        names=[p.device for p in list_ports.comports()];self.ports['values']=names
        if names and not self.port_name.get():self.port_name.set(names[0])
    def write_log(self,s):
        self.log.insert('end',s+'\n');self.log.see('end')
        if int(self.log.index('end-1c').split('.')[0])>150:self.log.delete('1.0','50.0')
    def connect(self):
        self.disconnect()
        try:
            self.port=serial.Serial(self.port_name.get(),115200,timeout=0,write_timeout=.2)
            self.last_rx=time.monotonic();self.status.set('Connected; waiting for fresh telemetry')
            self.write_log('Opening USB serial may reset the Uno. Output will start OFF.')
        except (serial.SerialException,ValueError) as e:messagebox.showerror('Connection',str(e))
    def disconnect(self):
        if self.port:self.port.close()
        self.port=None;self.buffer.clear();self.status.set('DISCONNECTED - load state unknown')
    def send(self,text):
        if not self.port:return messagebox.showinfo('Connect first','Select the Arduino COM port and connect.')
        if '\n' in text or '\r' in text or len(text)>46:return
        try:self.port.write((text+'\n').encode('ascii'));self.write_log('> '+text)
        except (serial.SerialException,UnicodeError) as e:self.write_log(str(e));self.disconnect()
    def poll(self):
        if self.port:
            try:
                self.buffer.extend(self.port.read(4096))
                while b'\n' in self.buffer:
                    raw,_,tail=self.buffer.partition(b'\n');self.buffer=bytearray(tail)
                    line=raw.decode('ascii',errors='replace').strip();self.write_log(line)
                    fields=line.split()
                    if len(fields)==8 and fields[0]=='S':
                        try:
                            v,i,lo,hi,limit,delay=map(int,fields[2:])
                            self.status.set(f'{fields[1]} | {v} V | {i/1000:.2f} A')
                            self.settings.set(f'Device settings: {lo}-{hi} V | {limit} mA | delay {delay} ms')
                            self.last_rx=time.monotonic()
                        except ValueError:pass
                if len(self.buffer)>8192:self.buffer.clear()
                if time.monotonic()-self.last_rx>2:self.status.set('STALE TELEMETRY - load state unknown')
            except serial.SerialException as e:self.write_log(str(e));self.disconnect()
        self.root.after(50,self.poll)
    def close(self):self.disconnect();self.root.destroy()

if __name__=='__main__':
    root=tk.Tk();Panel(root);root.mainloop()
