const {machine}=require('./run_avr.cjs');
const fs=require('fs');
const m=machine(process.argv[2]);
const log=[]; let line='',passed=0,failed=0;
const trace=[];let inputV=0,inputI=0;
function capture(event){trace.push({time_us:Math.round(m.cpu.cycles/16),voltage_V:inputV,current_mA:inputI,load_D8:on()?1:0,event});}
m.b.addListener((value,old)=>{if((value^old)&1)capture('D8 edge');});
const stamp=()=> (m.cpu.cycles/16000).toFixed(3);
function record(s){console.log(s);log.push(s);}
m.uart.onByteTransmit=byte=>{
  if(byte===10){record(`${stamp()} ms UART ${line}`);line='';}
  else if(byte!==13)line+=String.fromCharCode(byte);
};
function check(ok,name){record(`${ok?'PASS':'FAIL'} ${name}`);ok?passed++:failed++;}
function input(v,i){inputV=v;inputI=i;m.adc.channelValues[0]=v/300*5;m.adc.channelValues[1]=i/20000*5;capture('input step');}
function on(){return !!(m.cpu.data[0x25]&1);}
function send(text){for(const b of Buffer.from(text+'\n')){while(!m.uart.writeByte(b))m.runMs(.1);m.runMs(.2);}m.runMs(80);}
function button(port,pin){port.setPin(pin,false);m.runMs(65);port.setPin(pin,true);m.runMs(65);}
// D6/D7 are port D, D9/D10 are port B. All buttons released.
m.d.setPin(6,true);m.d.setPin(7,true);m.b.setPin(1,true);m.b.setPin(2,true);
input(230,2000);m.runMs(100);check(!on(),'boot output OFF');
m.runMs(4300);check(on(),'safe boot connects after LCD initialization and delay');
input(270,2000);const ovStart=m.cpu.cycles;while(on()&&m.cpu.cycles-ovStart<16000*15)m.step();
const ovLatency=(m.cpu.cycles-ovStart)/16000;
check(!on()&&ovLatency>0&&ovLatency<15,`OV output disconnect ${ovLatency.toFixed(3)} ms (<15 ms emulator budget)`);
send('RESET');check(!on(),'unsafe serial RESET does not reconnect');
input(230,2000);m.runMs(4000);check(!on(),'recovery remains latched');
send('RESET');m.runMs(2900);check(!on(),'serial RESET observes full delay');
m.runMs(200);check(on(),'serial RESET eventually reconnects');
// Menu once selects VMIN; plus once updates 180 -> 181.
button(m.b,2);button(m.b,1);check(!on(),'physical button setting change opens output');
send('GET');check(log.some(s=>/UART S WAIT \d+ \d+ 181 250 5000 3000/.test(s)),'local button changes VMIN to 181');
send('SET IMAX 3000');send('GET');check(log.some(s=>/181 250 3000 3000/.test(s)),'PC settings visible in telemetry');
m.runMs(3100);check(on(),'new settings rearm after delay');
input(230,6000);const ocStart=m.cpu.cycles;while(on()&&m.cpu.cycles-ocStart<16000*15)m.step();
const ocLatency=(m.cpu.cycles-ocStart)/16000;
check(!on()&&ocLatency>0&&ocLatency<15,`OC output disconnect ${ocLatency.toFixed(3)} ms (<15 ms emulator budget)`);
input(230,1000);m.runMs(20);button(m.d,6);m.runMs(3100);check(on(),'physical RESET button recovers safely');
send('SET VMAX 0');send('GET');check(log.some(s=>s.includes('ERR RANGE')),'invalid setting rejected');
send('X'.repeat(70));check(log.some(s=>s.includes('ERR LENGTH')),'overlong serial frame rejected');
send('GET');check(on(),'parser recovers after overlong frame');
input(170,1000);m.runMs(15);check(!on(),'UV output disconnect');
input(230,1000);m.runMs(20);send('RESET');m.runMs(3100);
input(300,1000);m.runMs(15);check(!on(),'ADC rail saturation disconnect');
send('GET');check(log.some(s=>s.includes('UART S SENSOR')),'saturation reported as sensor fault');
record(`RESULT integration passed=${passed} failed=${failed}`);
if(process.argv[3])fs.writeFileSync(process.argv[3],log.join('\n')+'\n');
if(process.argv[3]){
  const path=require('path'),dir=path.dirname(process.argv[3]);capture('end');
  fs.writeFileSync(path.join(dir,'timing_events.json'),JSON.stringify(trace,null,2)+'\n');
  fs.writeFileSync(path.join(dir,'timing_capture.csv'),'time_us,voltage_V,current_mA,load_D8,event\n'+trace.map(r=>Object.values(r).join(',')).join('\n')+'\n');
  let vcd='$timescale 1 us $end\n$scope module protector $end\n$var wire 9 v voltage_V $end\n$var wire 15 i current_mA $end\n$var wire 1 l load_D8 $end\n$upscope $end\n$enddefinitions $end\n';
  for(const r of trace)vcd+=`#${r.time_us}\nb${r.voltage_V.toString(2)} v\nb${r.current_mA.toString(2)} i\n${r.load_D8}l\n`;
  fs.writeFileSync(path.join(dir,'timing_capture.vcd'),vcd);
}
process.exitCode=failed?1:0;
