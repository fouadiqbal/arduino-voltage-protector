// Executes real ATmega328P machine code; no reimplementation of protection rules.
const fs = require('fs');
const avr = require(process.env.AVR8JS_PATH || 'avr8js');
function loadHex(path) {
  const bytes = new Uint8Array(32768);
  for (const line of fs.readFileSync(path,'utf8').trim().split(/\r?\n/)) {
    if (line[0] !== ':') throw Error('Invalid HEX');
    const n = parseInt(line.slice(1,3),16), addr = parseInt(line.slice(3,7),16);
    const type = parseInt(line.slice(7,9),16);
    let sum=0;
    for(let i=1;i<line.length;i+=2) sum+=parseInt(line.slice(i,i+2),16);
    if(sum & 255) throw Error('HEX checksum mismatch');
    if(type===0) for(let i=0;i<n;i++) bytes[addr+i]=parseInt(line.slice(9+2*i,11+2*i),16);
    else if(type!==1 && type!==3) throw Error('Unsupported HEX record '+type);
  }
  return new Uint16Array(bytes.buffer);
}
function machine(path) {
  const cpu=new avr.CPU(loadHex(path));
  const timer=new avr.AVRTimer(cpu,avr.timer0Config);
  const uart=new avr.AVRUSART(cpu,avr.usart0Config,16000000);
  const b=new avr.AVRIOPort(cpu,avr.portBConfig);
  const c=new avr.AVRIOPort(cpu,avr.portCConfig);
  const d=new avr.AVRIOPort(cpu,avr.portDConfig);
  const adc=new avr.AVRADC(cpu,avr.adcConfig);
  // ACK the LCD backpack at 0x27. Model bus byte time at the configured SCL
  // rate, but not LCD pixels; Wokwi provides the visual LCD verification.
  const twi=new avr.AVRTWI(cpu,avr.twiConfig,16000000);
  const later=(bits,fn)=>cpu.addClockEvent(fn,Math.ceil(bits*16000000/twi.sclFrequency));
  twi.eventHandler={
    start:()=>later(1,()=>twi.completeStart()),
    stop:()=>later(1,()=>twi.completeStop()),
    connectToSlave:addr=>later(9,()=>twi.completeConnect(addr===0x27)),
    writeByte:()=>later(9,()=>twi.completeWrite(true)),
    readByte:()=>later(9,()=>twi.completeRead(255))
  };
  const step=()=>{avr.avrInstruction(cpu); cpu.tick();};
  const runMs=ms=>{const end=cpu.cycles+ms*16000; while(cpu.cycles<end)step();};
  return {cpu,timer,uart,b,c,d,adc,twi,step,runMs};
}
if(require.main===module) {
  const m=machine(process.argv[2]); let line='',done=false,failed=true;
  m.uart.onByteTransmit=byte=>{
    if(byte===10) {
      console.log(line);
      if(line.startsWith('RESULT ')){done=true;failed=!/failed=0\s*$/.test(line);}
      line='';
    } else if(byte!==13) line+=String.fromCharCode(byte);
  };
  while(!done && m.cpu.cycles<16000000*10) m.step();
  if(!done) console.error('FAIL: simulation timed out');
  process.exitCode=failed?1:0;
}
module.exports={machine};
