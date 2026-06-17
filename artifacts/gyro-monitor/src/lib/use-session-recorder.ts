import { useState, useRef, useCallback } from 'react';
import { useCreateTelemetry } from '@workspace/api-client-react';
import type { TelemetryInput } from '@workspace/api-client-react';

export type RecorderStatus = 'idle' | 'recording' | 'stopped';

export function useSessionRecorder(sessionId: number | null) {
  const [status, setStatus] = useState<RecorderStatus>('idle');
  const [recordedCount, setRecordedCount] = useState(0);
  const lastRecordTime = useRef(0);
  const createTelemetry = useCreateTelemetry();

  const startRecording = useCallback(() => {
    setStatus('recording');
    setRecordedCount(0);
    lastRecordTime.current = 0;
  }, []);

  const stopRecording = useCallback(() => {
    setStatus('stopped');
  }, []);

  const record = useCallback((telemetry: TelemetryInput | null) => {
    if (status !== 'recording' || !telemetry || !sessionId) return;

    const now = Date.now();
    // Record every 500ms to avoid overwhelming the DB
    if (now - lastRecordTime.current < 500) return;
    lastRecordTime.current = now;

    createTelemetry.mutate(
      {
        data: {
          sessionId,
          rpm: telemetry.rpm ?? 0,
          tiltX: telemetry.tiltX ?? 0,
          tiltY: telemetry.tiltY ?? 0,
          tiltZ: telemetry.tiltZ ?? 0,
          rotationZ: telemetry.rotationZ ?? 0,
          avgTiltMag: telemetry.avgTiltMag ?? 0,
          temp: telemetry.temp ?? 0,
          throttle: telemetry.throttle ?? 0,
          pwmPct: telemetry.pwmPct ?? 0,
          vibration: telemetry.vibration ?? 0,
          autoMode: telemetry.autoMode ?? false,
          pidError: telemetry.pidError ?? null,
          pidOutput: telemetry.pidOutput ?? null,
          pidIntegral: telemetry.pidIntegral ?? null,
        },
      },
      {
        onSuccess: () => {
          setRecordedCount((c) => c + 1);
        },
      }
    );
  }, [status, sessionId, createTelemetry]);

  return {
    status,
    recordedCount,
    startRecording,
    stopRecording,
    record,
  };
}
