"""Readable I2C layout; every wire remains electrically connected and visible."""
import json
from pathlib import Path
p=[]; c=[]
def part(t,id,x,y,rotate=0,**attrs): p.append(dict(type=t,id=id,left=x,top=y,rotate=rotate,attrs=attrs))
def wire(a,b,color,route=None): c.append([a,b,color,route or []])
part('wokwi-arduino-uno','uno',0,70)
part('wokwi-lcd1602','lcd',450,-110,pins='i2c',i2cAddress='0x27')
part('wokwi-potentiometer','voltage',-250,60,value='785')
part('wokwi-potentiometer','current',-250,230,value='102')
part('wokwi-text','title',-250,-210,text='ARDUINO UNO VOLTAGE PROTECTOR')
part('wokwi-text','vlabel',-285,15,text='VOLTAGE | A0 | 0-300 V')
part('wokwi-text','ilabel',-285,185,text='CURRENT | A1 | 0-20 A')
part('wokwi-text','lcdlabel',450,-160,text='I2C LCD | SDA: A4, SCL: A5')
part('wokwi-text','legend',-250,450,text='5 V simulation | Red: +5 V | Black: GND | Orange: analog | Blue: controls')
for n,(name,pin,x) in enumerate([('MENU','10',20),('PLUS','9',115),('MINUS','7',210),('RESET','6',305)]):
    part('wokwi-pushbutton',name,x,-110,color='blue',label=name)
    wire(name+':1.l','uno:'+pin,'blue',['v'+str(35+n*12),'*','v'+str(-25-n*12)])
    if n: wire(name+':2.l',['MENU','PLUS','MINUS'][n-1]+':2.l','black',['v-35','*','v-35'])
wire('MENU:2.l','uno:GND.3','black',['h-65','v130','*','v-15'])
wire('voltage:SIG','uno:A0','orange',['v45','h120','v215','*','v40'])
wire('current:SIG','uno:A1','orange',['v45','h140','v65','*','v60'])
wire('voltage:VCC','current:VCC','red',['h-30','*','h-30'])
wire('voltage:GND','current:GND','black',['h-45','*','h-45'])
wire('current:VCC','uno:5V','red',['v90','*','v90'])
wire('current:GND','uno:GND.1','black',['v110','*','v110'])
wire('uno:A4','lcd:SDA','purple',['v40','h80','*','h-45'])
wire('uno:A5','lcd:SCL','purple',['v60','h95','*','h-25'])
wire('uno:5V','lcd:VCC','red',['v90','h240','*','h-65'])
wire('uno:GND.2','lcd:GND','black',['v110','h200','*','h-85'])
for name,pin,color,x in [('load','8','green',490),('fault','13','red',640)]:
    part('wokwi-led',name,x,260,color=color,label=name.upper())
    part('wokwi-resistor',name+'R',x-45,325,value='330')
    wire('uno:'+pin,name+'R:1',color,['v-30','h'+('210' if name=='load' else '365'),'v260','*','h-20'])
    wire(name+'R:2',name+':A',color,['h20'])
wire('load:C','fault:C','black',['v105','*','v105'])
wire('fault:C','uno:GND.2','black',['v105','*','v110'])
data=dict(version=1,author='Fouad Iqbal',editor='wokwi',parts=p,connections=c,
          serialMonitor=dict(display='always',newline='lf',convertEol=True))
out=Path(__file__).parent/'wokwi/diagram.json'
out.write_text(json.dumps(data,indent=2)+'\n')
print(out)
