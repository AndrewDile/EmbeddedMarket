import React, {useState} from 'react';
import {initialBom} from './data.js';
import {checkCompatibility} from './api.js';
import {BomPage, ResultsPage, FinalizedPage} from './Screens.jsx';

export default function App() {
  const [bom, setBom] = useState(initialBom);
  const [page, setPage] = useState('bom');
  const [editing, setEditing] = useState(false);
  const [result, setResult] = useState(null);
  const [run, setRun] = useState(0);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState('');

  function revise() {
    setPage('bom');
    setEditing(true);
  }
  function save(display, ledCurrent) {
    if (display !== bom.display || ledCurrent !== bom.ledCurrent) {
      setBom({display, ledCurrent, revision: bom.revision + 1});
      setResult(null);
    }
    setEditing(false);
    setError('');
  }
  async function runCheck() {
    setBusy(true);
    setError('');
    try {
      const checked = await checkCompatibility(bom);
      setResult(checked);
      setRun(n => n + 1);
      setPage('results');
    } catch {
      setError('Could not retrieve compatibility results. Check the connection and try again.');
    } finally {
      setBusy(false);
    }
  }
  return <>
    <header><strong>HARDWARE / COMPATIBILITY WORKBENCH</strong><span>FIELD NODE / BOM r{String(bom.revision).padStart(2,'0')} / Run {String(run || 1).padStart(2,'0')}</span></header>
    <main>
      {page === 'bom' && <BomPage {...{bom, editing, busy, error, save, revise, runCheck}} cancel={() => setEditing(false)} />}
      {page === 'results' && <ResultsPage {...{bom, result, run, revise}} approve={() => setPage('final')} />}
      {page === 'final' && <FinalizedPage {...{bom, result, run, revise}} />}
      <footer>Example project data only. “OK” means checked against these inputs, not hardware certification. EX-* parts are fictional; no real datasheet has been verified.</footer>
    </main>
  </>;
}
