import React, { useState, useEffect, useRef, useCallback } from 'react';
import './index.css';
import { getText, getHelpSections, type Language } from './i18n';

declare global {
  interface Window {
    __JUCE__?: {
      backend?: {
        juceSetParameter: (id: string, value: number) => Promise<void>;
        juceRequestState: () => Promise<void>;
      };
    };
    updateDSPState?: (state: any) => void;
    updateDSPParams?: (params: any) => void;
  }
}

const MIN_LUFS = -60.0;
const MAX_LUFS = -5.0;

// ============================================================================
// Rotary Knob (Canvas-rendered arc, matching old JUCE LookAndFeel)
// ============================================================================
const RotaryKnob = ({
  label, accentColor, min, max, val, defaultVal, suffix, onChange, disabled, formatValue,
  overlay
}: {
  label: string, accentColor: string, min: number, max: number, val: number, defaultVal: number,
  suffix: string, onChange: (v: number) => void, disabled?: boolean,
  formatValue?: (v: number) => string, overlay?: React.ReactNode
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [isEditing, setIsEditing] = useState(false);
  const [editString, setEditString] = useState('');
  const dragRef = useRef<{ y: number, val: number } | null>(null);

  const norm = (val - min) / (max - min);

  // Draw the rotary arc
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;
    const w = canvas.width, h = canvas.height;
    ctx.clearRect(0, 0, w, h);

    const cx = w / 2, cy = h / 2;
    const radius = Math.min(w, h) * 0.38;
    const startAngle = Math.PI * 0.75;
    const endAngle = Math.PI * 2.25;
    const activeAngle = startAngle + norm * (endAngle - startAngle);

    // Parse accent color
    const ac = accentColor;

    // Outer track ring
    ctx.beginPath();
    ctx.arc(cx, cy, radius, startAngle, endAngle);
    ctx.strokeStyle = 'rgba(255,255,255,0.06)';
    ctx.lineWidth = 6;
    ctx.lineCap = 'round';
    ctx.stroke();

    // Active value ring glow
    if (norm > 0.001) {
      ctx.beginPath();
      ctx.arc(cx, cy, radius, startAngle, activeAngle);
      ctx.strokeStyle = ac + '40'; // 25% alpha glow
      ctx.lineWidth = 12;
      ctx.lineCap = 'round';
      ctx.stroke();

      // Solid active arc
      ctx.beginPath();
      ctx.arc(cx, cy, radius, startAngle, activeAngle);
      ctx.strokeStyle = ac;
      ctx.lineWidth = 6;
      ctx.lineCap = 'round';
      ctx.stroke();
    }

    // Inner dome
    const innerR = radius - 12;
    const grad = ctx.createLinearGradient(cx - innerR, cy - innerR, cx + innerR, cy + innerR);
    grad.addColorStop(0, '#1f2d40');
    grad.addColorStop(1, '#0a1320');
    ctx.beginPath();
    ctx.arc(cx, cy, innerR, 0, Math.PI * 2);
    ctx.fillStyle = grad;
    ctx.fill();

    // Dome bevel
    ctx.beginPath();
    ctx.arc(cx, cy, innerR - 0.5, 0, Math.PI * 2);
    ctx.strokeStyle = 'rgba(255,255,255,0.16)';
    ctx.lineWidth = 1;
    ctx.stroke();

    // Pointer indicator
    const pointerLen = radius * 0.35;
    const px = cx + Math.cos(activeAngle - Math.PI / 2) * (radius * 0.6);
    const py = cy + Math.sin(activeAngle - Math.PI / 2) * (radius * 0.6);
    const px2 = cx + Math.cos(activeAngle - Math.PI / 2) * (radius * 0.6 - pointerLen);
    const py2 = cy + Math.sin(activeAngle - Math.PI / 2) * (radius * 0.6 - pointerLen);
    ctx.beginPath();
    ctx.moveTo(px2, py2);
    ctx.lineTo(px, py);
    ctx.strokeStyle = disabled ? 'rgba(255,255,255,0.3)' : '#ffffff';
    ctx.lineWidth = 3;
    ctx.lineCap = 'round';
    ctx.stroke();
  }, [val, min, max, accentColor, disabled, norm]);

  const onPointerDown = (e: React.PointerEvent<HTMLDivElement>) => {
    if (disabled) return;
    e.currentTarget.setPointerCapture(e.pointerId);
    dragRef.current = { y: e.clientY, val };
  };
  const onPointerMove = (e: React.PointerEvent<HTMLDivElement>) => {
    if (!dragRef.current || disabled) return;
    const dy = dragRef.current.y - e.clientY;
    let nv = dragRef.current.val + (dy / 200) * (max - min);
    onChange(Math.max(min, Math.min(max, nv)));
  };
  const onPointerUp = (e: React.PointerEvent<HTMLDivElement>) => {
    if (dragRef.current) {
      dragRef.current = null;
      e.currentTarget.releasePointerCapture(e.pointerId);
    }
  };
  const onDblClick = () => { if (!disabled) onChange(defaultVal); };
  const onValueDblClick = (e: React.MouseEvent) => {
    if (disabled) return;
    e.stopPropagation();
    setIsEditing(true);
    setEditString(val.toFixed(1));
  };
  const submitEdit = () => {
    setIsEditing(false);
    const p = parseFloat(editString);
    if (!isNaN(p)) onChange(Math.max(min, Math.min(max, p)));
  };

  const displayVal = formatValue ? formatValue(val) : `${val.toFixed(1)} ${suffix}`;

  return (
    <div className={`knob-card ${disabled ? 'disabled' : ''}`}>
      <div className="knob-label">{label}</div>
      {overlay && <div className="knob-overlay">{overlay}</div>}
      <div
        className="knob-area"
        onPointerDown={onPointerDown} onPointerMove={onPointerMove}
        onPointerUp={onPointerUp} onPointerCancel={onPointerUp}
        onDoubleClick={onDblClick}
        style={{ cursor: disabled ? 'default' : 'ns-resize' }}
      >
        <canvas ref={canvasRef} width={80} height={80} style={{ width: 60, height: 60 }} />
      </div>
      <div className="knob-value-container" onDoubleClick={onValueDblClick}>
        {isEditing ? (
          <input type="text" className="knob-value-input" value={editString}
            onChange={e => setEditString(e.target.value)}
            onBlur={submitEdit} onKeyDown={e => { if (e.key === 'Enter') submitEdit(); }}
            autoFocus />
        ) : (
          <span className="knob-value">{displayVal}</span>
        )}
      </div>
    </div>
  );
};

// ============================================================================
// Presence Indicator (arc gauge matching old PresenceIndicator.h)
// ============================================================================
const PresenceIndicator = ({ presence }: { presence: number }) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  useEffect(() => {
    const c = canvasRef.current;
    if (!c) return;
    const ctx = c.getContext('2d');
    if (!ctx) return;
    const w = c.width, h = c.height;
    ctx.clearRect(0, 0, w, h);

    // Background card
    ctx.fillStyle = 'rgba(255,255,255,0.05)';
    ctx.beginPath();
    ctx.roundRect(0, 0, w, h, 10);
    ctx.fill();
    ctx.strokeStyle = 'rgba(255,255,255,0.08)';
    ctx.lineWidth = 1;
    ctx.stroke();

    // Title
    ctx.fillStyle = '#89a0ba';
    ctx.font = 'bold 9px Inter';
    ctx.textAlign = 'center';
    ctx.fillText('VOICE', w / 2, 14);

    // Arc meter
    const cx = w / 2, cy = h / 2 + 4;
    const radius = Math.min(w, h) * 0.28;
    const startAngle = -Math.PI * 0.75;
    const endAngle = Math.PI * 0.75;
    const activeAngle = startAngle + presence * (endAngle - startAngle);

    // Background arc
    ctx.beginPath();
    ctx.arc(cx, cy, radius, startAngle + Math.PI / 2, endAngle + Math.PI / 2);
    ctx.strokeStyle = 'rgba(255,255,255,0.1)';
    ctx.lineWidth = 5;
    ctx.lineCap = 'round';
    ctx.stroke();

    // Active arc
    if (presence > 0.01) {
      ctx.beginPath();
      ctx.arc(cx, cy, radius, startAngle + Math.PI / 2, activeAngle + Math.PI / 2);
      const arcColor = presence > 0.5 ? '#3ecfd5' : '#56d4ff';
      ctx.strokeStyle = arcColor;
      ctx.lineWidth = 5.5;
      ctx.lineCap = 'round';
      ctx.stroke();

      // Glow tip dot
      const tipX = cx + radius * Math.cos(activeAngle);
      const tipY = cy + radius * Math.sin(activeAngle);
      ctx.fillStyle = arcColor + '80';
      ctx.beginPath(); ctx.arc(tipX, tipY, 4, 0, Math.PI * 2); ctx.fill();
      ctx.fillStyle = '#f3f7fb';
      ctx.beginPath(); ctx.arc(tipX, tipY, 2.5, 0, Math.PI * 2); ctx.fill();
    }

    // Value text
    const presColor = presence > 0.7 ? '#56d4ff' : presence > 0.3 ? '#89a0ba' : '#5a7088';
    ctx.fillStyle = presColor;
    ctx.font = 'bold 11px Inter';
    ctx.textAlign = 'center';
    ctx.fillText(presence.toFixed(2), w / 2, h - 5);
  }, [presence]);

  return <canvas ref={canvasRef} width={64} height={52} style={{ width: 64, height: 52 }} />;
};

// ============================================================================
// Gain Timeline (scrolling history strip matching old GainTimelineComponent.h)
// ============================================================================
const GainTimeline = ({ gainHistory, gainHeadIndex, currentGainDb, voicePresence }: {
  gainHistory: number[], gainHeadIndex: number, currentGainDb: number, voicePresence: number
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const c = canvasRef.current;
    if (!c) return;
    const ctx = c.getContext('2d');
    if (!ctx) return;
    const w = c.width, h = c.height;
    ctx.clearRect(0, 0, w, h);

    // Background
    ctx.fillStyle = 'rgba(255,255,255,0.05)';
    ctx.beginPath(); ctx.roundRect(0, 0, w, h, 8); ctx.fill();

    const readoutW = 38;
    const plotW = w - readoutW - 4;
    const plotH = h - 6;
    const plotX = 2, plotY = 3;
    const centerY = plotY + plotH / 2;
    const maxGain = 12;
    const count = gainHistory.length;

    // Presence background
    const presAlpha = Math.min(0.18, voicePresence * 0.18);
    ctx.fillStyle = `rgba(86,212,255,${presAlpha})`;
    ctx.beginPath(); ctx.roundRect(plotX, plotY, plotW, plotH, 6); ctx.fill();

    // Center 0dB line
    ctx.strokeStyle = 'rgba(255,255,255,0.12)';
    ctx.lineWidth = 1;
    ctx.beginPath(); ctx.moveTo(plotX, centerY); ctx.lineTo(plotX + plotW, centerY); ctx.stroke();

    // Draw gain history
    if (count > 0) {
      ctx.beginPath();
      let boostStarted = false;
      for (let i = 0; i < count; i++) {
        const idx = (gainHeadIndex + i) % count;
        const g = gainHistory[idx];
        const x = plotX + (i / (count - 1)) * plotW;
        const normG = Math.max(-maxGain, Math.min(maxGain, g)) / maxGain;
        const halfH = plotH * 0.48;
        if (g >= 0) {
          const y = centerY - halfH * normG;
          if (!boostStarted) { ctx.moveTo(x, centerY); boostStarted = true; }
          ctx.lineTo(x, y);
        }
      }
      if (boostStarted) {
        ctx.lineTo(plotX + plotW, centerY);
        ctx.closePath();
        ctx.fillStyle = 'rgba(129,230,157,0.35)';
        ctx.fill();
        ctx.strokeStyle = 'rgba(129,230,157,0.7)';
        ctx.lineWidth = 1;
        ctx.stroke();
      }

      ctx.beginPath();
      let duckStarted = false;
      for (let i = 0; i < count; i++) {
        const idx = (gainHeadIndex + i) % count;
        const g = gainHistory[idx];
        const x = plotX + (i / (count - 1)) * plotW;
        const normG = Math.max(-maxGain, Math.min(maxGain, g)) / maxGain;
        const halfH = plotH * 0.48;
        if (g <= 0) {
          const y = centerY - halfH * normG;
          if (!duckStarted) { ctx.moveTo(x, centerY); duckStarted = true; }
          ctx.lineTo(x, y);
        }
      }
      if (duckStarted) {
        ctx.lineTo(plotX + plotW, centerY);
        ctx.closePath();
        ctx.fillStyle = 'rgba(255,175,110,0.35)';
        ctx.fill();
        ctx.strokeStyle = 'rgba(255,175,110,0.7)';
        ctx.lineWidth = 1;
        ctx.stroke();
      }
    }

    // Readout
    const gc = currentGainDb < -0.1 ? '#ffaf6e' : currentGainDb > 0.1 ? '#81e69d' : '#89a0ba';
    ctx.fillStyle = gc;
    ctx.font = 'bold 10px Inter';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    const prefix = currentGainDb > 0.05 ? '+' : '';
    ctx.fillText(prefix + currentGainDb.toFixed(1), w - readoutW / 2 - 2, h / 2);
  }, [gainHistory, gainHeadIndex, currentGainDb, voicePresence]);

  return <canvas ref={canvasRef} width={700} height={52} style={{ width: '100%', height: 52 }} />;
};

// ============================================================================
// LUFS Scatter Plot Visualizer (1:1 from VisualizerComponent.cpp)
// ============================================================================
const VisualizerCanvas = ({ params, dspState, onChangeParam }: {
  params: any, dspState: any, onChangeParam: (k: string, v: number) => void
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const interactionRef = useRef<{
    mode: 'none' | 'balance' | 'anchor' | 'noiseGate',
    startX: number, startY: number, startVal: number
  }>({ mode: 'none', startX: 0, startY: 0, startVal: 0 });

  const mapX = (v: number, w: number, pad: number[]) => {
    const actW = w - pad[0] - pad[2];
    return pad[0] + Math.max(0, Math.min(1, (v - MIN_LUFS) / (MAX_LUFS - MIN_LUFS))) * actW;
  };
  const mapY = (v: number, h: number, pad: number[]) => {
    const actH = h - pad[1] - pad[3];
    return h - pad[3] - Math.max(0, Math.min(1, (v - MIN_LUFS) / (MAX_LUFS - MIN_LUFS))) * actH;
  };
  const unmapX = (x: number, w: number, pad: number[]) => {
    return MIN_LUFS + ((x - pad[0]) / (w - pad[0] - pad[2])) * (MAX_LUFS - MIN_LUFS);
  };

  const computeTarget = useCallback((vLufs: number, p: any) => {
    return vLufs - (p.balance + (p.dynamics * 0.030) * (vLufs - p.anchorManual));
  }, []);

  useEffect(() => {
    let animId: number;
    const render = () => {
      const canvas = canvasRef.current;
      if (!canvas) return;
      const ctx = canvas.getContext('2d');
      if (!ctx) return;
      const w = canvas.width, h = canvas.height;
      const pad = [46, 30, 14, 34]; // L, T, R, B
      ctx.clearRect(0, 0, w, h);

      // Background
      const bgGrad = ctx.createLinearGradient(0, 0, w, h);
      bgGrad.addColorStop(0, '#0b1420');
      bgGrad.addColorStop(1, '#131f32');
      ctx.fillStyle = bgGrad;
      ctx.beginPath(); ctx.roundRect(0, 0, w, h, 14); ctx.fill();

      // Border
      ctx.strokeStyle = 'rgba(255,255,255,0.09)';
      ctx.lineWidth = 1;
      ctx.beginPath(); ctx.roundRect(1, 1, w - 2, h - 2, 13); ctx.stroke();

      const p = params;
      const s = dspState;

      // Minor grid (5 LUFS)
      for (let l = -55; l <= -10; l += 5) {
        const y = mapY(l, h, pad), x = mapX(l, w, pad);
        ctx.strokeStyle = 'rgba(255,255,255,0.03)';
        ctx.lineWidth = 1;
        ctx.beginPath(); ctx.moveTo(pad[0], y); ctx.lineTo(w - pad[2], y); ctx.stroke();
        ctx.beginPath(); ctx.moveTo(x, pad[1]); ctx.lineTo(x, h - pad[3]); ctx.stroke();
      }

      // Major grid (10 LUFS) + labels
      ctx.fillStyle = '#5a7088';
      ctx.font = '9px Inter';
      for (let l = -50; l <= -10; l += 10) {
        const y = mapY(l, h, pad), x = mapX(l, w, pad);
        ctx.strokeStyle = 'rgba(255,255,255,0.07)';
        ctx.lineWidth = 1;
        ctx.beginPath(); ctx.moveTo(pad[0], y); ctx.lineTo(w - pad[2], y); ctx.stroke();
        ctx.beginPath(); ctx.moveTo(x, pad[1]); ctx.lineTo(x, h - pad[3]); ctx.stroke();

        ctx.textAlign = 'right'; ctx.textBaseline = 'middle';
        ctx.fillText(l.toString(), pad[0] - 6, y);
        ctx.textAlign = 'center'; ctx.textBaseline = 'top';
        ctx.fillText(l.toString(), x, h - pad[3] + 5);
      }

      // Axis titles
      ctx.fillStyle = '#89a0ba';
      ctx.font = 'bold 9px Inter';
      ctx.textAlign = 'center'; ctx.textBaseline = 'top';
      ctx.fillText('VOCAL  LUFS  →', (pad[0] + w - pad[2]) / 2, h - pad[3] + 18);
      // Rotated Y-axis title
      ctx.save();
      ctx.translate(12, (pad[1] + h - pad[3]) / 2);
      ctx.rotate(-Math.PI / 2);
      ctx.fillText('BACKING  LUFS  →', 0, 0);
      ctx.restore();

      // 45° reference line (dashed)
      ctx.setLineDash([6, 4]);
      ctx.strokeStyle = 'rgba(255,255,255,0.09)';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(mapX(MIN_LUFS, w, pad), mapY(MIN_LUFS, h, pad));
      ctx.lineTo(mapX(MAX_LUFS, w, pad), mapY(MAX_LUFS, h, pad));
      ctx.stroke();
      ctx.setLineDash([]);

      // Noise floor region
      const noiseX = mapX(p.noiseGate, w, pad);
      if (noiseX > pad[0]) {
        const ng = ctx.createLinearGradient(pad[0], 0, noiseX, 0);
        ng.addColorStop(0, 'rgba(0,0,0,0.18)');
        ng.addColorStop(1, 'rgba(0,0,0,0.30)');
        ctx.fillStyle = ng;
        ctx.fillRect(pad[0], pad[1], noiseX - pad[0], h - pad[1] - pad[3]);

        const isGateHover = interactionRef.current.mode === 'noiseGate';
        ctx.setLineDash([4, 3]);
        ctx.strokeStyle = isGateHover ? 'rgba(255,255,255,0.5)' : 'rgba(255,255,255,0.25)';
        ctx.lineWidth = isGateHover ? 2 : 1;
        ctx.beginPath(); ctx.moveTo(noiseX, pad[1]); ctx.lineTo(noiseX, h - pad[3]); ctx.stroke();
        ctx.setLineDash([]);

        // "NOISE" label
        ctx.fillStyle = 'rgba(255,255,255,0.35)';
        ctx.font = '8px Inter';
        ctx.textAlign = 'right'; ctx.textBaseline = 'top';
        ctx.fillText('NOISE', noiseX - 4, pad[1] + 4);
      }

      // Range region polygons
      const isExpand = p.expansionMode > 0;
      const upperLimit = isExpand ? Math.min(p.boostCeiling, p.range) : Math.min(p.duckFloor, p.range);
      const lowerLimit = isExpand ? Math.min(p.duckFloor, p.range) : Math.min(p.boostCeiling, p.range);

      const drawRangeRegion = (limit: number, dir: 1 | -1, color: string) => {
        ctx.fillStyle = color;
        ctx.beginPath();
        let started = false;
        const noiseFloor = Math.max(p.noiseGate as number, MIN_LUFS);
        for (let l = noiseFloor; l <= MAX_LUFS; l += 0.5) {
          const tb = computeTarget(l, p);
          const px = mapX(l, w, pad), py = mapY(tb, h, pad);
          if (!started) { ctx.moveTo(px, py); started = true; } else ctx.lineTo(px, py);
        }
        for (let l = MAX_LUFS; l >= noiseFloor; l -= 0.5) {
          const tb = computeTarget(l, p);
          ctx.lineTo(mapX(l, w, pad), mapY(tb + limit * dir, h, pad));
        }
        ctx.closePath(); ctx.fill();
      };
      drawRangeRegion(upperLimit, 1, isExpand ? 'rgba(129,230,157,0.1)' : 'rgba(255,175,110,0.1)');
      drawRangeRegion(lowerLimit, -1, isExpand ? 'rgba(255,175,110,0.1)' : 'rgba(129,230,157,0.1)');

      // Target curve
      ctx.beginPath();
      let curveStarted = false;
      const noiseFloor = Math.max(p.noiseGate as number, MIN_LUFS);
      for (let l = noiseFloor; l <= MAX_LUFS; l += 0.5) {
        const tb = computeTarget(l, p);
        const px = mapX(l, w, pad), py = mapY(tb, h, pad);
        if (!curveStarted) { ctx.moveTo(px, py); curveStarted = true; } else ctx.lineTo(px, py);
      }
      if (curveStarted) {
        // Glow
        ctx.strokeStyle = 'rgba(62,207,213,0.18)';
        ctx.lineWidth = 8; ctx.lineCap = 'round'; ctx.lineJoin = 'round'; ctx.stroke();
        // Gradient line
        const cg = ctx.createLinearGradient(pad[0], 0, w - pad[2], 0);
        cg.addColorStop(0, '#3ecfd5'); cg.addColorStop(1, '#6eedf3');
        ctx.strokeStyle = cg; ctx.lineWidth = 2.5; ctx.stroke();
      }

      // Anchor point
      const anchorX = mapX(p.anchorManual, w, pad);
      const anchorY = mapY(computeTarget(p.anchorManual, p), h, pad);
      if (anchorX > pad[0] && anchorX < w - pad[2]) {
        const isAnchorHv = interactionRef.current.mode === 'anchor';
        const r = isAnchorHv ? 7 : 5;
        ctx.fillStyle = 'rgba(62,207,213,0.2)'; ctx.beginPath(); ctx.arc(anchorX, anchorY, r + 8, 0, Math.PI * 2); ctx.fill();
        ctx.fillStyle = 'rgba(62,207,213,0.4)'; ctx.beginPath(); ctx.arc(anchorX, anchorY, r + 4, 0, Math.PI * 2); ctx.fill();
        ctx.fillStyle = '#3ecfd5'; ctx.beginPath(); ctx.arc(anchorX, anchorY, r, 0, Math.PI * 2); ctx.fill();
        ctx.fillStyle = '#ddffffff'; ctx.beginPath(); ctx.arc(anchorX, anchorY, r - 2, 0, Math.PI * 2); ctx.fill();
      }

      // Trail (from ring buffer)
      const trail: number[][] = s.trail || [];
      const trailHead = s.trailHeadIndex || 0;
      const trailCount = trail.length;
      if (trailCount > 0) {
        for (let i = 0; i < trailCount; i++) {
          const oi = (trailHead + i) % trailCount;
          const pt = trail[oi];
          if (!pt || pt[0] < MIN_LUFS + 1) continue;
          const px = mapX(pt[0], w, pad), py = mapY(pt[1], h, pad);
          if (px < pad[0] || px > w - pad[2] || py < pad[1] || py > h - pad[3]) continue;
          const age = (trailCount - 1 - i) / (trailCount - 1);
          const alpha = Math.min(0.4, (1 - age) * 0.4);
          const size = 2 + (1 - age) * 2;
          ctx.fillStyle = `rgba(255,255,255,${alpha})`;
          ctx.beginPath(); ctx.arc(px, py, size, 0, Math.PI * 2); ctx.fill();
        }
      }

      // State point + Gain vector
      if (s.sidechainConnected && s.voicePresence > 0.05) {
        const tBack = computeTarget(s.vocalLufs, p);
        const spx = mapX(s.vocalLufs, w, pad);
        const spy = mapY(s.backingLufs, h, pad);
        const tpy = mapY(tBack, h, pad);

        let pc: string;
        if (s.totalGainDb < -0.5) pc = `rgba(255,175,110,${0.6 * s.voicePresence})`;
        else if (s.totalGainDb > 0.5) pc = `rgba(129,230,157,${0.6 * s.voicePresence})`;
        else pc = `rgba(62,207,213,${0.6 * s.voicePresence})`;

        // Gain vector line
        if (Math.abs(s.totalGainDb) > 0.3 && spx >= pad[0] && spx <= w - pad[2]) {
          ctx.strokeStyle = pc; ctx.lineWidth = 2;
          ctx.beginPath(); ctx.moveTo(spx, spy); ctx.lineTo(spx, tpy); ctx.stroke();
          ctx.fillStyle = pc;
          ctx.beginPath(); ctx.arc(spx, tpy, 3, 0, Math.PI * 2); ctx.fill();
        }

        // Multi-ring glow state point
        if (spx >= pad[0] && spx <= w - pad[2]) {
          const dotColor = s.totalGainDb < -0.5 ? '#ffaf6e' : s.totalGainDb > 0.5 ? '#81e69d' : '#3ecfd5';
          ctx.fillStyle = dotColor + '26'; ctx.beginPath(); ctx.arc(spx, spy, 14, 0, Math.PI * 2); ctx.fill();
          ctx.fillStyle = dotColor + '4d'; ctx.beginPath(); ctx.arc(spx, spy, 8, 0, Math.PI * 2); ctx.fill();
          ctx.fillStyle = dotColor; ctx.beginPath(); ctx.arc(spx, spy, 5, 0, Math.PI * 2); ctx.fill();
          ctx.fillStyle = 'rgba(255,255,255,0.8)'; ctx.beginPath(); ctx.arc(spx, spy, 2.5, 0, Math.PI * 2); ctx.fill();
        }
      }

      // Info overlay badges (top-right)
      const badgeY = 10;
      // SC badge
      const scText = s.sidechainConnected ? 'SC ACTIVE' : 'SC OFFLINE';
      const scColor = s.sidechainConnected ? '#35d1c7' : '#7f95ab';
      const scBadgeX = w - pad[2] - 80;
      ctx.fillStyle = scColor + '1f'; ctx.beginPath(); ctx.roundRect(scBadgeX, badgeY, 76, 20, 10); ctx.fill();
      ctx.strokeStyle = scColor + '85'; ctx.lineWidth = 1; ctx.beginPath(); ctx.roundRect(scBadgeX, badgeY, 76, 20, 10); ctx.stroke();
      ctx.fillStyle = scColor; ctx.font = 'bold 10px Inter'; ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
      ctx.fillText(scText, scBadgeX + 38, badgeY + 10);

      // Gain badge
      const gainBadgeX = scBadgeX - 74;
      const gc = s.totalGainDb < -0.3 ? '#ffaa73' : s.totalGainDb > 0.3 ? '#7ee6aa' : '#89a0ba';
      const gPrefix = s.totalGainDb > 0.05 ? '+' : '';
      const gText = gPrefix + (s.totalGainDb || 0).toFixed(1) + ' dB';
      ctx.fillStyle = 'rgba(0,0,0,0.07)'; ctx.beginPath(); ctx.roundRect(gainBadgeX, badgeY, 68, 20, 10); ctx.fill();
      ctx.strokeStyle = gc + '80'; ctx.lineWidth = 1; ctx.beginPath(); ctx.roundRect(gainBadgeX, badgeY, 68, 20, 10); ctx.stroke();
      ctx.fillStyle = gc; ctx.font = 'bold 10px Inter'; ctx.textAlign = 'center';
      ctx.fillText(gText, gainBadgeX + 34, badgeY + 10);

      animId = requestAnimationFrame(render);
    };
    animId = requestAnimationFrame(render);
    return () => cancelAnimationFrame(animId);
  }, [params, dspState, computeTarget]);

  // Hit-test for interactions
  const hitTest = (e: React.PointerEvent<HTMLCanvasElement>) => {
    const rect = e.currentTarget.getBoundingClientRect();
    const x = (e.clientX - rect.left) * (e.currentTarget.width / rect.width);
    const y = (e.clientY - rect.top) * (e.currentTarget.height / rect.height);
    const w = e.currentTarget.width, h = e.currentTarget.height;
    const pad = [46, 30, 14, 34];
    const mx = unmapX(x, w, pad);

    const ax = mapX(params.anchorManual, w, pad);
    const ay = mapY(computeTarget(params.anchorManual, params), h, pad);
    if (Math.hypot(x - ax, y - ay) < 15) return 'anchor';

    const nx = mapX(params.noiseGate, w, pad);
    if (Math.abs(x - nx) < 10) return 'noiseGate';

    if (mx > params.noiseGate) {
      const cy = mapY(computeTarget(mx, params), h, pad);
      if (Math.abs(y - cy) < 12) return 'balance';
    }
    return 'none';
  };

  const onPointerDown = (e: React.PointerEvent<HTMLCanvasElement>) => {
    const mode = hitTest(e);
    if (mode === 'none') return;
    if (mode === 'anchor' && params.anchorAuto > 0) return;
    e.currentTarget.setPointerCapture(e.pointerId);
    const startVal = mode === 'anchor' ? params.anchorManual : mode === 'noiseGate' ? params.noiseGate : params.balance;
    interactionRef.current = { mode, startX: e.clientX, startY: e.clientY, startVal };
  };

  const onPointerMove = (e: React.PointerEvent<HTMLCanvasElement>) => {
    if (interactionRef.current.mode !== 'none') {
      const { mode, startX, startY, startVal } = interactionRef.current;
      const rect = e.currentTarget.getBoundingClientRect();
      const h = rect.height, w = rect.width;
      if (mode === 'balance') {
        const delta = ((startY - e.clientY) / h) * (MAX_LUFS - MIN_LUFS);
        onChangeParam('balance', startVal + delta);
      } else if (mode === 'noiseGate') {
        const delta = ((e.clientX - startX) / w) * (MAX_LUFS - MIN_LUFS);
        onChangeParam('noiseGate', startVal + delta);
      } else if (mode === 'anchor') {
        const delta = ((e.clientX - startX) / w) * (MAX_LUFS - MIN_LUFS);
        onChangeParam('anchorManual', startVal + delta);
      }
    } else {
      const ht = hitTest(e);
      e.currentTarget.style.cursor = ht === 'none' ? 'default' : ht === 'balance' ? 'ns-resize' :
        (ht === 'anchor' && params.anchorAuto > 0) ? 'default' : 'ew-resize';
    }
  };

  const onPointerUp = (e: React.PointerEvent<HTMLCanvasElement>) => {
    e.currentTarget.releasePointerCapture(e.pointerId);
    interactionRef.current.mode = 'none';
  };

  const onWheel = (e: React.WheelEvent<HTMLCanvasElement>) => {
    const delta = -e.deltaY * 0.5;
    onChangeParam('dynamics', params.dynamics + delta);
  };

  const onDoubleClick = (e: React.MouseEvent<HTMLCanvasElement>) => {
    // Reset hovered parameter to default
    const rect = e.currentTarget.getBoundingClientRect();
    const x = (e.clientX - rect.left) * (e.currentTarget.width / rect.width);
    const y = (e.clientY - rect.top) * (e.currentTarget.height / rect.height);
    const w = e.currentTarget.width, h = e.currentTarget.height;
    const pad = [46, 30, 14, 34];
    const mx = unmapX(x, w, pad);

    const ax = mapX(params.anchorManual, w, pad);
    const ay = mapY(computeTarget(params.anchorManual, params), h, pad);
    if (Math.hypot(x - ax, y - ay) < 15) {
      // Toggle anchor auto
      onChangeParam('anchorAuto', params.anchorAuto > 0 ? 0 : 1);
      return;
    }
    const nx = mapX(params.noiseGate, w, pad);
    if (Math.abs(x - nx) < 10) { onChangeParam('noiseGate', -45); return; }
    if (mx > params.noiseGate) {
      const cy = mapY(computeTarget(mx, params), h, pad);
      if (Math.abs(y - cy) < 15) { onChangeParam('balance', 3); return; }
    }
  };

  return (
    <canvas ref={canvasRef} width={820} height={380}
      style={{ width: '100%', height: '100%', display: 'block', touchAction: 'none' }}
      onPointerDown={onPointerDown} onPointerMove={onPointerMove}
      onPointerUp={onPointerUp} onPointerLeave={onPointerUp} onPointerCancel={onPointerUp}
      onWheel={onWheel} onDoubleClick={onDoubleClick}
    />
  );
};

// ============================================================================
// Help Overlay (scrollable card matching old HelpOverlay.cpp)
// ============================================================================
const HelpOverlayComponent = ({ lang, onClose }: { lang: Language, onClose: () => void }) => {
  const sections = getHelpSections(lang);
  return (
    <div className="modal-backdrop" onClick={onClose}>
      <div className="help-card" onClick={e => e.stopPropagation()}>
        <div className="help-header">
          <span>{getText('Header_Title', lang)}</span>
          <div className="modal-close" onClick={onClose}>✕</div>
        </div>
        <div className="help-scroll">
          {sections.map((sec, i) => (
            <div key={i} className="help-section">
              <div className="help-title">{sec.title}</div>
              <div className="help-desc">{sec.description}</div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};

// ============================================================================
// Main Application
// ============================================================================
function App() {
  const [params, setParams] = useState({
    balance: 3, dynamics: 0, speed: 120, range: 8, anchorManual: -24,
    noiseGate: -45, boostCeiling: 6, duckFloor: 10, expansionMode: 0, anchorAuto: 1,
    lookAhead: 0, presenceRelease: 80
  });

  const [dspState, setDspState] = useState({
    totalGainDb: 0, vocalLufs: -70, backingLufs: -70, voicePresence: 0,
    heldVocalLevel: -60, anchorPoint: -24, sidechainConnected: false,
    trail: [] as number[][], trailHeadIndex: 0,
    gainHistory: [] as number[], gainHeadIndex: 0
  });

  const [expertMode, setExpertMode] = useState(false);
  const [modalMode, setModalMode] = useState<'none' | 'about' | 'help'>('none');
  const [lang, setLang] = useState<Language>(
    navigator.language.startsWith('zh') ? 'zh' : 'en'
  );

  useEffect(() => {
    window.updateDSPState = (s) => setDspState(s);
    window.updateDSPParams = (p) => setParams(prev => ({ ...prev, ...p }));
    if (window.__JUCE__?.backend?.juceRequestState) {
      window.__JUCE__.backend.juceRequestState();
    }
    return () => { delete window.updateDSPState; delete window.updateDSPParams; };
  }, []);

  const handleParamChange = (id: string, value: number) => {
    setParams(prev => ({ ...prev, [id]: value }));
    if (window.__JUCE__?.backend?.juceSetParameter) {
      window.__JUCE__.backend.juceSetParameter(id, value);
    }
  };

  const t = (key: string) => getText(key, lang);

  // Knob configurations matching old accent colors exactly
  const coreAccents = ['#3ecfd5', '#ffb76d', '#8ae4a9', '#9fcfff', '#56d4ff'];
  const expertAccents = ['#79e28e', '#ff9d68', '#7fd8e6', '#f7d57a', '#9ce8c8'];

  return (
    <div className="app-container">
      {/* Scanline overlay */}
      <div className="scanline-overlay" />

      <header className="header">
        <div className="title-area">
          <span className="title-main">LiveFlow</span>
          <span className="title-sub">Balancer</span>
          <span className="header-subtitle">{t('Header_Sub')}</span>
        </div>
        <div className="header-actions">
          <div className="header-btn lang-btn" onClick={() => setLang(lang === 'en' ? 'zh' : 'en')}>
            {t('Btn_ToggleLang')}
          </div>
          <div className="header-btn" onClick={() => setModalMode('help')}>{t('Btn_Help')}</div>
          <div className="header-btn" onClick={() => setModalMode('about')}>{t('Btn_About')}</div>
          <div className={`header-btn expert-btn ${expertMode ? 'active' : ''}`}
            onClick={() => setExpertMode(!expertMode)}>
            {t('Btn_Expert')}
          </div>
        </div>
      </header>

      <main className="canvas-container">
        <VisualizerCanvas params={params} dspState={dspState} onChangeParam={handleParamChange} />
      </main>

      <section className="middle-strip">
        <PresenceIndicator presence={dspState.voicePresence} />
        <div className="timeline-container">
          <GainTimeline
            gainHistory={dspState.gainHistory || []}
            gainHeadIndex={dspState.gainHeadIndex || 0}
            currentGainDb={dspState.totalGainDb || 0}
            voicePresence={dspState.voicePresence || 0}
          />
        </div>
      </section>

      <section className="controls-row">
        <RotaryKnob label={t('Label_Balance')} accentColor={coreAccents[0]}
          min={-12} max={12} defaultVal={3} val={params.balance} suffix="dB"
          formatValue={v => (v > 0.05 ? '+' : '') + v.toFixed(1) + ' dB'}
          onChange={v => handleParamChange('balance', v)} />

        <RotaryKnob label={t('Label_Dynamics')} accentColor={coreAccents[1]}
          min={-100} max={100} defaultVal={0} val={params.dynamics} suffix="%"
          formatValue={v => v.toFixed(0) + '%'}
          onChange={v => handleParamChange('dynamics', v)}
          overlay={
            <div className={`toggle-pill ${params.expansionMode > 0 ? 'on' : ''}`}
              onClick={() => handleParamChange('expansionMode', params.expansionMode > 0 ? 0 : 1)}>
              {params.expansionMode > 0 ? t('Toggle_Expand') : t('Toggle_Compress')}
            </div>
          } />

        <RotaryKnob label={t('Label_Speed')} accentColor={coreAccents[2]}
          min={5} max={500} defaultVal={120} val={params.speed} suffix="ms"
          formatValue={v => v.toFixed(0) + ' ms'}
          onChange={v => handleParamChange('speed', v)} />

        <RotaryKnob label={t('Label_Range')} accentColor={coreAccents[3]}
          min={0} max={24} defaultVal={8} val={params.range} suffix="dB"
          onChange={v => handleParamChange('range', v)} />

        <RotaryKnob label={t('Label_Anchor')} accentColor={coreAccents[4]}
          min={-60} max={0} defaultVal={-24} val={params.anchorManual} suffix="dB"
          disabled={params.anchorAuto > 0}
          onChange={v => handleParamChange('anchorManual', v)}
          overlay={
            <div className={`toggle-pill ${params.anchorAuto > 0 ? 'on' : ''}`}
              onClick={() => handleParamChange('anchorAuto', params.anchorAuto > 0 ? 0 : 1)}>
              {t('Toggle_Auto')}
            </div>
          } />
      </section>

      {expertMode && (
        <section className="controls-row expert-row">
          <RotaryKnob label={t('Label_BoostCeiling')} accentColor={expertAccents[0]}
            min={0} max={24} defaultVal={6} val={params.boostCeiling} suffix="dB"
            onChange={v => handleParamChange('boostCeiling', v)} />
          <RotaryKnob label={t('Label_DuckFloor')} accentColor={expertAccents[1]}
            min={0} max={24} defaultVal={10} val={params.duckFloor} suffix="dB"
            onChange={v => handleParamChange('duckFloor', v)} />
          <RotaryKnob label={t('Label_NoiseGate')} accentColor={expertAccents[2]}
            min={-60} max={-20} defaultVal={-45} val={params.noiseGate} suffix="dB"
            onChange={v => handleParamChange('noiseGate', v)} />
          <RotaryKnob label={t('Label_LookAhead')} accentColor={expertAccents[3]}
            min={0} max={20} defaultVal={0} val={params.lookAhead} suffix="ms"
            formatValue={v => v.toFixed(0) + ' ms'}
            onChange={v => handleParamChange('lookAhead', v)} />
          <RotaryKnob label={t('Label_ReleaseHyst')} accentColor={expertAccents[4]}
            min={10} max={300} defaultVal={80} val={params.presenceRelease} suffix="ms"
            formatValue={v => v.toFixed(0) + ' ms'}
            onChange={v => handleParamChange('presenceRelease', v)} />
        </section>
      )}

      {modalMode === 'help' && <HelpOverlayComponent lang={lang} onClose={() => setModalMode('none')} />}

      {modalMode === 'about' && (
        <div className="modal-backdrop" onClick={() => setModalMode('none')}>
          <div className="modal-content" onClick={e => e.stopPropagation()}>
            <div className="modal-close" onClick={() => setModalMode('none')}>✕</div>
            <h2>{t('About_Title')}</h2>
            <div className="modal-body">
              <p style={{ whiteSpace: 'pre-line' }}>{t('About_Desc')}</p>
              <p style={{ color: '#5a7088', marginTop: 12 }}>Version v1.0.2604</p>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

export default App;
