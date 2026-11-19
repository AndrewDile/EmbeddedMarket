import {parts, displays} from './data.js';

// This fixture runs only when the team has not configured an API endpoint.
export function sampleResponse(bom) {
  const conflict = displays[bom.display].voltage > 3.3;
  const known = bom.ledCurrent !== null;
  const total = parts(bom).reduce((sum, row) => sum + (row[5] ?? 0), 0);
  const overBudget = total > 200;
  const conflicts = Number(conflict) + Number(overBudget), warnings = known ? 0 : 2;
  const checks = { total: 11, passed: 11 - conflicts - warnings, warnings, conflicts };
  return { conflict, known, total, overBudget, ready: !conflict && known && !overBudget, checks };
}

export async function checkCompatibility(bom, endpoint = import.meta.env?.VITE_CHECK_URL) {
  if (!endpoint) return sampleResponse(bom);
  const response = await fetch(endpoint, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(bom)
  });
  if (!response.ok) throw new Error('Check failed');
  const result = await response.json();
  const flags = ['conflict','known','overBudget','ready'];
  if (!result || !result.checks || !Number.isFinite(result.total) || flags.some(key => typeof result[key] !== 'boolean')) {
    throw new Error('Incomplete check response');
  }
  return result;
}
