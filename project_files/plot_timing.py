"""Plot actual AVR emulator input and GPIO events; no invented timing values."""
import json
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
ROOT=Path(__file__).resolve().parents[1]
folder=ROOT/'test_evidence'
events=json.loads((folder/'timing_events.json').read_text())
plt.rcParams.update({'font.family':'DejaVu Sans','font.size':10,'axes.spines.top':False,'axes.spines.right':False})
faults=[]
for n,e in enumerate(events):
    if e['event']=='input step' and e['load_D8']==1 and (250<e['voltage_V']<300 or e['current_mA']>5000):
        edge=next(x for x in events[n+1:] if x['event']=='D8 edge' and x['load_D8']==0)
        faults.append((e,edge))
fig,axes=plt.subplots(len(faults),2,figsize=(11,6),sharex='row',layout='constrained')
for row,(fault,edge) in enumerate(faults):
    kind='Over-voltage' if fault['voltage_V']>250 else 'Over-current'
    key='voltage_V' if kind=='Over-voltage' else 'current_mA'
    unit='V' if key=='voltage_V' else 'A'
    scale=1 if unit=='V' else 1000
    limit=250 if unit=='V' else 3
    prev=events[events.index(fault)-1]
    dt=(edge['time_us']-fault['time_us'])/1000
    x=[-2,0,0,dt,dt,10]
    axes[row,0].plot(x,[prev[key]/scale,prev[key]/scale,fault[key]/scale,fault[key]/scale,fault[key]/scale,fault[key]/scale],color='#c76b12',lw=2)
    axes[row,0].axhline(limit,color='#bd3345',ls='--',label=f'Limit: {limit} {unit}')
    axes[row,0].set_ylabel(f'Input ({unit})');axes[row,0].legend(loc='lower right');axes[row,0].set_title(kind+' input step')
    axes[row,1].plot(x,[1,1,1,1,0,0],color='#007c91',lw=2)
    axes[row,1].annotate(f'D8 LOW after {dt:.3f} ms',xy=(dt,0),xytext=(dt+1,.65),arrowprops={'arrowstyle':'->','color':'#223047'})
    axes[row,1].set_ylim(-.15,1.25);axes[row,1].set_yticks([0,1],['OFF','ON']);axes[row,1].set_title('Actual simulated load GPIO')
    for ax in axes[row]:
        ax.axvline(0,color='#52677d',ls=':',lw=1);ax.grid(alpha=.18);ax.set_xlabel('Time from fault injection (ms)');ax.set_xlim(-2,10)
fig.suptitle('Arduino Uno fault response | compiled AVR firmware',fontsize=15,fontweight='bold')
fig.savefig(folder/'timing_fault_response.png',dpi=180)
fig.savefig(folder/'timing_fault_response.svg')
plt.close(fig)
fig,ax=plt.subplots(figsize=(11,3.8),layout='constrained')
t=[e['time_us']/1e6 for e in events];y=[e['load_D8'] for e in events]
ax.step(t,y,where='post',color='#007c91',lw=2);ax.set_yticks([0,1],['OFF','ON']);ax.set_ylim(-.15,1.25)
ax.set_xlabel('Simulated time (s)');ax.set_ylabel('Load output D8');ax.grid(alpha=.2)
ax.set_title('Full integration run: startup, trips, settings and safe restart',fontweight='bold')
fig.savefig(folder/'timing_full_run.png',dpi=180);fig.savefig(folder/'timing_full_run.svg');plt.close(fig)
print('Timing plots generated from',len(events),'recorded events.')
print([(round((edge['time_us']-fault['time_us'])/1000,3)) for fault,edge in faults])
