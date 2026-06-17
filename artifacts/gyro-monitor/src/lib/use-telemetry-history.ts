import { useState, useRef, useCallback } from 'react';
import type { TelemetryInput } from '@workspace/api-client-react';

export interface TelemetryPoint {
  timestamp: number;
  rpm: number;
  tiltMag: number;
  throttle: number;
  pwmPct: number;
  pidError: number;
  pidOutput: number;
  temp: number;
}

export function useTelemetryHistory(maxPoints = 200) {
  const [history, setHistory] = useState<TelemetryPoint[]>([]);
  const historyRef = useRef<TelemetryPoint[]>([]);

  const addPoint = useCallback((telemetry: TelemetryInput | null) => {
    if (!telemetry) return;
    const point: TelemetryPoint = {
      timestamp: Date.now(),
      rpm: telemetry.rpm ?? 0,
      tiltMag: telemetry.avgTiltMag ?? Math.sqrt((telemetry.tiltX ?? 0) ** 2 + (telemetry.tiltY ?? 0) ** 2),
      throttle: telemetry.throttle ?? 0,
      pwmPct: telemetry.pwmPct ?? 0,
      pidError: telemetry.pidError ?? 0,
      pidOutput: telemetry.pidOutput ?? 0,
      temp: telemetry.temp ?? 0,
    };

    const next = [...historyRef.current, point];
    if (next.length > maxPoints) {
      next.shift();
    }
    historyRef.current = next;
    setHistory(next);
  }, [maxPoints]);

  const clearHistory = useCallback(() => {
    historyRef.current = [];
    setHistory([]);
  }, []);

  return { history, addPoint, clearHistory };
}
