import { useState, useEffect, useCallback, useRef } from 'react';
import type { TelemetryInput } from '@workspace/api-client-react';

export type ConnectionStatus = 'disconnected' | 'connecting' | 'connected' | 'error';

export function useSerial() {
  const [status, setStatus] = useState<ConnectionStatus>('disconnected');
  const [error, setError] = useState<string | null>(null);
  const [isSimulated, setIsSimulated] = useState(false);
  const [telemetry, setTelemetry] = useState<TelemetryInput | null>(null);

  const portRef = useRef<any>(null);
  const readerRef = useRef<any>(null);
  const writerRef = useRef<any>(null);
  const simIntervalRef = useRef<number | null>(null);

  const connect = useCallback(async () => {
    try {
      if (!('serial' in navigator)) {
        throw new Error('Web Serial API not supported in this browser. Use simulation mode.');
      }
      
      setStatus('connecting');
      setError(null);
      setIsSimulated(false);

      const port = await (navigator as any).serial.requestPort({ filters: [{ usbVendorId: 0x10C4 }] });
      await port.open({ baudRate: 115200 });
      portRef.current = port;
      setStatus('connected');

      const textDecoder = new TextDecoderStream();
      const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
      readerRef.current = textDecoder.readable.getReader();
      
      const textEncoder = new TextEncoderStream();
      const writableStreamClosed = textEncoder.readable.pipeTo(port.writable);
      writerRef.current = textEncoder.writable.getWriter();

      // Read loop
      (async () => {
        let buffer = "";
        try {
          while (true) {
            const { value, done } = await readerRef.current.read();
            if (done) break;
            if (value) {
              buffer += value;
              const lines = buffer.split('\n');
              buffer = lines.pop() || '';
              
              for (const line of lines) {
                if (line.trim().startsWith('{')) {
                  try {
                    const data = JSON.parse(line.trim());
                    setTelemetry(data as TelemetryInput);
                  } catch (e) {
                    console.error("Failed to parse telemetry line", line);
                  }
                }
              }
            }
          }
        } catch (error: any) {
          console.error("Serial read error:", error);
          setError(error.message);
          setStatus('error');
        }
      })();

    } catch (err: any) {
      setError(err.message);
      setStatus('error');
    }
  }, []);

  const startSimulation = useCallback(() => {
    setStatus('connected');
    setIsSimulated(true);
    setError(null);

    let t = 0;
    simIntervalRef.current = window.setInterval(() => {
      t += 0.1;
      setTelemetry({
        rpm: 3000 + Math.sin(t) * 100,
        tiltX: Math.sin(t * 0.5) * 5,
        tiltY: Math.cos(t * 0.3) * 3,
        tiltZ: 0,
        rotationZ: Math.sin(t * 0.1) * 2,
        avgTiltMag: Math.abs(Math.sin(t * 0.5) * 5),
        temp: 35 + Math.sin(t * 0.05) * 5,
        throttle: 1500 + Math.sin(t) * 100,
        pwmPct: 50 + Math.sin(t) * 10,
        vibration: Math.random() * 0.5,
        autoMode: true,
        pidError: Math.sin(t) * 2,
        pidOutput: Math.cos(t) * 10,
        pidIntegral: Math.sin(t * 0.5) * 5
      });
    }, 100);
  }, []);

  const disconnect = useCallback(async () => {
    if (simIntervalRef.current) {
      clearInterval(simIntervalRef.current);
      simIntervalRef.current = null;
    }
    
    if (readerRef.current) {
      await readerRef.current.cancel();
      readerRef.current = null;
    }
    if (writerRef.current) {
      writerRef.current.releaseLock();
      writerRef.current = null;
    }
    if (portRef.current) {
      await portRef.current.close();
      portRef.current = null;
    }
    
    setStatus('disconnected');
    setIsSimulated(false);
  }, []);

  const sendCommand = useCallback(async (cmd: string) => {
    if (isSimulated) {
      console.log(`[SIM] Sent command: ${cmd}`);
      return;
    }
    if (writerRef.current) {
      await writerRef.current.write(cmd + '\n');
    }
  }, [isSimulated]);

  return {
    status,
    error,
    isSimulated,
    telemetry,
    connect,
    startSimulation,
    disconnect,
    sendCommand
  };
}
