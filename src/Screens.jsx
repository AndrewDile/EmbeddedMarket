import React, {useState,useEffect,useRef} from 'react';
import {displays,parts} from './data.js';

const constraints = [['Power budget','≤ 200 mA @ 3.3 V'],['Allowed interfaces','I²C / SPI / GPIO only'],['Controller clock','≥ 48 MHz'],['Minimum memory','256 KB flash / 64 KB RAM']];
const pinMap = {U1:'20 usable · 9 assigned', U2:'U4 CH0 · SDA / SCL', U3:'PB0 SCK / PB1 MOSI / PB2 MISO / PB3 CS / PB4 DC', C1:'PC0 LED_A / PC1 LED_B', U4:'PA0 SDA / PA1 SCL'};
function Title({step,children}) {return <div className="title"><small>{step}</small><h1>{children}</h1></div>;}
function Cards({rows}) {return <div className="cards">{rows.map(([k,v,h])=><section key={k}><aside>{k}</aside><strong>{v}</strong><aside>{h}</aside></section>)}</div>;}
function Badge({tone='',children}) {return <small className={tone}>{children}</small>;}
function Editor({bom,save,cancel}) {
  const [display,setDisplay]=useState(bom.display), [current,setCurrent]=useState(bom.ledCurrent??'');
  return <form onSubmit={e=>{e.preventDefault();save(display,current===''?null:Number(current));}}>
    <h2>Revise BOM</h2><label>U3 display <select value={display} onChange={e=>setDisplay(e.target.value)}>{Object.keys(displays).map(id=><option key={id}>{id}</option>)}</select></label>
    <label>C1 maximum current (mA) <input type="number" min="0" step="any" placeholder="Unknown" value={current} onChange={e=>setCurrent(e.target.value)}/></label>
    <button className="primary">Save revision</button><button type="button" onClick={cancel}>Cancel</button></form>;
}
export function BomPage({bom,editing,busy,error,save,revise,runCheck,cancel}) {
  const d=displays[bom.display], initial=bom.revision===5;
  const rows=[
    ['Microcontroller','EX-MCU32 · U1','3.3 V · 48 MHz','256 KB flash / 64 KB RAM','Project controller','40 mA allocation','20 usable','9 assigned','✓ OK','Controller checks passed'],
    ['Temperature sensor','EX-TEMP33 · U2','3.3 V logic · 10 mA','I²C address 0x48','U4 CH0 → U2','SDA / SCL','0 extra MCU pins','','N/A','Connection changed — recheck required'],
    ['Display module',bom.display+' · U3',d.voltage+' V MISO output · '+d.current+' mA','SPI, full duplex','SPI1 → U1','PB0–PB4','5 pins','','× ERROR','MISO exceeds input limit'],
    ['Status-light circuit','USER-LED2 · C1','3.3 V · 2 LED outputs','Maximum current: '+(bom.ledCurrent===null?'unknown':bom.ledCurrent+' mA'),'GPIO → U1','PC0 / PC1','2 pins','','⚠ WARN','Current check incomplete'],
    ['I²C multiplexer','EX-I2CMUX8 · U4','3.3 V · 2 mA max','8 channels · address 0x70','I²C1 → U1','PA0 SDA / PA1 SCL','2 MCU pins','','N/A','Newly selected — not yet checked']
  ];
  return <><Title step="01 / CONFIGURE">BOM Page</Title><p>Field node · 1 controller + 3 peripherals + 1 custom circuit · {initial?'Last check: r04':'Revision awaiting recheck'}</p>
    <section><div className="bar"><h2>Project-wide constraints</h2><button disabled title="Fixed demo constraints">Edit</button></div><div className="constraints">{constraints.map(([k,v])=><div key={k}><aside>{k}</aside><b>{v}</b></div>)}</div></section>
    <div className="bar"><h2>Components</h2><button disabled title="Fixed five-component demo">＋ Add Component　⌄</button></div>
    <div className="scroll"><table><thead><tr>{['Component / part number','Key specs & constraints','Connection / allocation','MCU pins','Last check status'].map(h=><th key={h}>{h}</th>)}</tr></thead><tbody>{rows.map(([name,id,a,b,c,d,p,q,status,note],i)=><tr key={id}><td><b>{name}</b><aside>{id}</aside><button className="link" disabled={busy||![2,3].includes(i)} onClick={revise}>Edit part / connections</button></td><td>{a}<br/>{b}</td><td>{c}<br/>{d}</td><td>{p}<br/>{q}</td><td><Badge tone={initial?['ok','','bad','warn',''][i]:''}>{initial?status:'N/A'}</Badge><aside>{initial?note:'Recheck required'}</aside></td></tr>)}</tbody></table></div>
    <Cards rows={ [['MCU pin allocation','9 / 20','11 free · shared I²C1 pins reused'],['Power estimate',(bom.ledCurrent===null?'≥ ':'')+(52+d.current+(bom.ledCurrent??0))+' / 200 mA',bom.ledCurrent===null?'Known subtotal + C1 current unknown':'All current inputs entered'],['Check coverage',initial?'6 of 11 passed':'Recheck required',initial?'1 failed · 4 incomplete · rerouted sensor needs recheck':'Component inputs changed']] }/>
    {editing?<Editor {...{bom,save,cancel}}/>:<div className="actions"><button className="primary wide" disabled={busy} onClick={runCheck}>{busy?'Checking…':'Check Compatibility →'}</button><aside>✓ OK = completed check · ⚠ WARN = incomplete or needs review<br/>× ERROR = failed rule · — Not checked = pending / changed input</aside></div>}{error&&<p role="alert">{error}</p>}</>;
}

function ConflictDetail({bom,close,revise}) {
  const closeButton=useRef(null);
  useEffect(()=>{closeButton.current.focus();},[]);
  return <article className="panel" aria-label="Conflict Detail Side Panel" onKeyDown={e=>{if(e.key==='Escape')close();}}>
    <div className="bar"><h2>Conflict Detail Side Panel</h2><button ref={closeButton} onClick={close}>Close ×</button></div>
    <aside>E01 · U3 / {bom.display} → U1 / EX-MCU32 · Rule V-01</aside>
    <section className="banner"><Badge tone="bad">× ERROR / input-voltage limit</Badge><h3>5.0 V &gt; 3.3 V · exceeds limit by 1.7 V</h3><p>The display’s MISO signal drives MCU pin PB2 above its configured maximum input.</p></section>
    <h2>Specifications used in this run</h2><table><thead><tr><th>Parameter</th><th>U1 · controller</th><th>U3 · display</th></tr></thead><tbody>{[['Signal level','Max input: 3.3 V','MISO output: 5.0 V'],['Interface','SPI1 supported','SPI required · OK'],['Pins','PB0–PB4 reserved','5 signals · OK']].map(row=><tr key={row[0]}>{row.map(cell=><td key={cell}>{cell}</td>)}</tr>)}</tbody></table>
    <section><h2>Source &amp; provenance</h2><p>U1 input limit and U3 output level come from manually entered example project records.</p><aside>Datasheet used: none attached.</aside><button disabled>Attach datasheet / URL</button><button disabled>Open source</button></section>
    <h2>Review or correct specifications</h2><p>Revise the component inputs and rerun the compatibility check.</p><button onClick={revise}>Review / override spec…</button>
    <section className="banner"><b>Recommended: use a 3.3 V logic display</b><p>EX-DISP33 · 3.3 V MISO · SPI · 5 pins · 50 mA</p><button className="primary" onClick={revise}>Browse compatible alternatives →</button></section></article>;
}

export function ResultsPage({bom,result:r,run,revise,approve}) {
  const [open,setOpen]=useState(false);
  const trigger=useRef(null);
  function close(){setOpen(false);requestAnimationFrame(()=>trigger.current?.focus());}
  const errors=Number(r.conflict)+Number(r.overBudget), warnings=Number(!r.known)*(r.overBudget?1:2);
  const count=errors+warnings;
  const findings=[
    [r.conflict,'E01','bad','U3 · '+bom.display+' — Logic voltage mismatch','5.0 V MISO output > U1 3.3 V maximum input. Direct connection fails the input-voltage rule.'],
    [!r.known,'W01','warn','C1 · USER-LED2 — Current data missing','Maximum circuit current is unknown. Voltage, GPIO and 2-pin allocation pass; current check is incomplete.'],
    [r.overBudget||!r.known,r.overBudget?'E02':'W02',r.overBudget?'bad':'warn',r.overBudget?'Project — Current budget exceeded':'Project — Power-budget check incomplete','Known allocations total '+r.total+' mA'+(r.known?'.':' + unknown C1 draw.')+' Project limit: 200 mA.']
  ].filter(row=>row[0]);
  const actions=<><button onClick={revise}>← Revise BOM</button><button disabled={!r.ready} onClick={approve}>Review &amp; approve BOM</button></>;
  if(open)return <div className="split"><div className="summary"><small>03 / INSPECT</small><h1>Results Page</h1><aside>Field node · Run {run}</aside><section><h2>Not ready to finalize</h2>{errors} error · {warnings} warnings</section><section className="selected"><Badge tone="bad">E01 / × ERROR</Badge><h2>U3 · {bom.display}</h2><p>Logic voltage mismatch</p><button onClick={close}>View conflict — selected</button></section><section>{findings.slice(1).map(f=><p key={f[1]}><b>{f[1]} · {f[3]}</b></p>)}</section><p>Choose an alternative or update the reviewed specifications, then recheck the BOM.</p>{actions}</div><ConflictDetail {...{bom,close,revise}}/></div>;
  return <><Title step="02 / CHECK">Results Page</Title><section className="banner"><div className="bar"><h2>{r.ready?'Ready to finalize':'Not ready to finalize'}</h2><Badge tone={r.ready?'ok':'bad'}>{errors} ERROR / {warnings} WARNINGS</Badge></div>{r.ready?'Review and approve this passing run.':'Resolve open findings, then rerun the same BOM.'}</section>
    <Cards rows={ [['Rule coverage',(11-count)+' / 11 passed',errors+' failed · '+warnings+' incomplete'],['MCU signals','9 / 20 pins','Pin allocation verified against example inputs'],['Project power',r.total+' mA'+(r.known?'':' + ?'),'Budget ≤ 200 mA']] }/>
    <div className="bar"><h2>All errors &amp; warnings</h2><aside>{count} findings · no hidden issues</aside></div>{findings.map(([,id,tone,title,desc])=><section className="finding" key={id}><Badge tone={tone}>{id} / {tone==='bad'?'× ERROR':'⚠ WARN'}</Badge>　<b>{title}</b><p>{desc}</p>{id==='E01'&&<button ref={trigger} onClick={()=>setOpen(true)}>View conflict →</button>}<button onClick={revise}>{id==='E01'?'Browse compatible displays':id==='W01'?'Complete circuit data':'Review power inputs'}</button></section>)}
    <section><h2>Completed checks</h2><p>U1 · EX-MCU32: clock and memory meet project minimums. U2 · EX-TEMP33: modeled voltage, I²C address and pins OK.</p><aside>U3 SPI and pin allocation match. C1 uses two GPIO outputs.</aside></section>{actions} <aside className="inline">Approval unlocks only after a complete passing run.</aside></>;
}

export function FinalizedPage({bom,result,run,revise}) {
  const rows=parts(bom);
  const download='data:application/json;charset=utf-8,'+encodeURIComponent(JSON.stringify({bom,components:rows,pinMap,result,run},null,2));
  return <><Title step="04 / FINALIZE">Finalized BOM Page</Title><section className="banner actions"><Badge tone="ok">✓ PASSED + APPROVED</Badge><div><b>Field node · Frozen snapshot r{String(bom.revision).padStart(2,'0')}</b><p>11/11 checks complete · 0 errors · 0 warnings · User reviewed and approved Run {String(run).padStart(2,'0')}</p></div></section>
    <Cards rows={ [['Pin allocation','9 / 20 pins','11 available · I²C1 reused by multiplexer'],['Power allocation',result.total+' / 200 mA',(200-result.total)+' mA margin · all maximum draws entered'],['Project constraints','All satisfied','48 MHz · 256 KB flash / 64 KB RAM']] }/>
    <div className="bar"><h2>Connection tree</h2><aside>Signal connections · power / ground excluded from GPIO counts</aside></div>
    <div className="tree">{rows.map(([ref,id,name,spec,connection,current])=><div className={'node '+ref} key={ref}><div><b>{ref==='U1'?ref+' · '+id:name}</b><aside>{ref==='U1'?'Root controller':ref+' / '+id}</aside></div><div><b>{spec}</b><aside>{connection} · {pinMap[ref]}</aside></div><div>{current} mA<aside>✓ All scoped checks OK</aside></div></div>)}</div>
    <section><h2>What changed before approval</h2><p>✓ E01 resolved: EX-DISP50 replaced by {bom.display}. SPI retains five signals.</p><p>✓ W01 resolved: USER-LED2 maximum current recorded as {bom.ledCurrent} mA. W02 rechecked: {result.total} mA ≤ 200 mA.</p></section>
    <a className="primary" download="approved-bom-and-pins.json" href={download}>Export BOM &amp; pin map</a> <button onClick={revise}>Edit as new revision</button></>;
}
