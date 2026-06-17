import { useEffect, useState } from 'react';
import { useSerial } from '@/lib/use-serial';
import { useTelemetryHistory } from '@/lib/use-telemetry-history';
import { useSessionRecorder } from '@/lib/use-session-recorder';
import { Card } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { RPMChart, TiltChart, ThrottleChart, PIDChart } from '@/components/telemetry-charts';
import { SessionPanel } from '@/components/session-panel';

export default function Dashboard() {
  const serial = useSerial();
  const { history, addPoint, clearHistory } = useTelemetryHistory(200);
  const [sessionId, setSessionId] = useState<number | null>(null);
  const recorder = useSessionRecorder(sessionId);

  useEffect(() => {
    const interval = setInterval(() => {
      addPoint(serial.telemetry);
      recorder.record(serial.telemetry);
    }, 100);
    return () => clearInterval(interval);
  }, [serial.telemetry, addPoint, recorder]);

  useEffect(() => {
    if (serial.status === 'disconnected') {
      clearHistory();
    }
  }, [serial.status, clearHistory]);

  return (
    <div className="p-6 space-y-6">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-bold tracking-tight uppercase">Dashboard</h1>
        <div className="flex items-center gap-4">
          <div
            className={`px-3 py-1 rounded text-xs uppercase tracking-wider font-bold ${
              serial.status === 'connected'
                ? 'bg-primary text-primary-foreground'
                : serial.status === 'error'
                  ? 'bg-destructive text-destructive-foreground'
                  : 'bg-muted text-muted-foreground'
            }`}
            data-testid="status-connection"
          >
            {serial.status} {serial.isSimulated && '(SIM)'}
          </div>
          {serial.status === 'disconnected' || serial.status === 'error' ? (
            <>
              <Button
                onClick={serial.connect}
                data-testid="btn-connect"
                variant="default"
              >
                CONNECT USB
              </Button>
              <Button
                onClick={serial.startSimulation}
                data-testid="btn-sim"
                variant="outline"
              >
                SIMULATION
              </Button>
            </>
          ) : (
            <Button
              onClick={serial.disconnect}
              data-testid="btn-disconnect"
              variant="destructive"
            >
              DISCONNECT
            </Button>
          )}
        </div>
      </div>

      {/* Telemetry Cards */}
      <div className="grid grid-cols-1 lg:grid-cols-4 gap-6">
        <Card className="p-4 flex flex-col gap-2">
          <div className="text-sm text-muted-foreground uppercase">RPM</div>
          <div className="text-4xl font-bold" data-testid="value-rpm">
            {serial.telemetry?.rpm?.toFixed(0) ?? '0'}
          </div>
        </Card>
        <Card className="p-4 flex flex-col gap-2">
          <div className="text-sm text-muted-foreground uppercase">Tilt X</div>
          <div className="text-4xl font-bold" data-testid="value-tilt-x">
            {serial.telemetry?.tiltX?.toFixed(2) ?? '0.00'}°
          </div>
        </Card>
        <Card className="p-4 flex flex-col gap-2">
          <div className="text-sm text-muted-foreground uppercase">Tilt Y</div>
          <div className="text-4xl font-bold" data-testid="value-tilt-y">
            {serial.telemetry?.tiltY?.toFixed(2) ?? '0.00'}°
          </div>
        </Card>
        <Card className="p-4 flex flex-col gap-2">
          <div className="text-sm text-muted-foreground uppercase">Temp</div>
          <div className="text-4xl font-bold" data-testid="value-temp">
            {serial.telemetry?.temp?.toFixed(1) ?? '0.0'}°C
          </div>
        </Card>
      </div>

      {/* More telemetry */}
      <div className="grid grid-cols-1 lg:grid-cols-4 gap-6">
        <Card className="p-4 flex flex-col gap-2">
          <div className="text-sm text-muted-foreground uppercase">Tilt Z</div>
          <div className="text-4xl font-bold" data-testid="value-tilt-z">
            {serial.telemetry?.tiltZ?.toFixed(2) ?? '0.00'}°
          </div>
        </Card>
        <Card className="p-4 flex flex-col gap-2">
          <div className="text-sm text-muted-foreground uppercase">Rotation Z</div>
          <div className="text-4xl font-bold" data-testid="value-rotation-z">
            {serial.telemetry?.rotationZ?.toFixed(2) ?? '0.00'}°
          </div>
        </Card>
        <Card className="p-4 flex flex-col gap-2">
          <div className="text-sm text-muted-foreground uppercase">Throttle</div>
          <div className="text-4xl font-bold" data-testid="value-throttle">
            {serial.telemetry?.throttle?.toFixed(0) ?? '0'} µs
          </div>
        </Card>
        <Card className="p-4 flex flex-col gap-2">
          <div className="text-sm text-muted-foreground uppercase">PWM %</div>
          <div className="text-4xl font-bold" data-testid="value-pwm">
            {serial.telemetry?.pwmPct?.toFixed(1) ?? '0.0'}%
          </div>
        </Card>
      </div>

      {/* Charts */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <RPMChart data={history} />
        <TiltChart data={history} />
        <ThrottleChart data={history} />
        <PIDChart data={history} />
      </div>

      {/* Controls & Session */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        <Card className="p-4">
          <h2 className="text-lg font-bold mb-4 uppercase">Motor Controls</h2>
          <div className="grid grid-cols-2 gap-4">
            <Button
              onClick={() => serial.sendCommand('CMD:START')}
              data-testid="btn-motor-start"
              className="w-full"
            >
              START MOTOR
            </Button>
            <Button
              onClick={() => serial.sendCommand('CMD:STOP')}
              data-testid="btn-motor-stop"
              variant="secondary"
              className="w-full"
            >
              STOP MOTOR
            </Button>
            <Button
              onClick={() => serial.sendCommand('CMD:SPEED_UP')}
              data-testid="btn-speed-up"
              variant="outline"
              className="w-full"
            >
              SPEED UP
            </Button>
            <Button
              onClick={() => serial.sendCommand('CMD:SPEED_DOWN')}
              data-testid="btn-speed-down"
              variant="outline"
              className="w-full"
            >
              SPEED DOWN
            </Button>
            <Button
              onClick={() => serial.sendCommand('CMD:AUTO_ON')}
              data-testid="btn-auto-on"
              variant="outline"
              className="w-full"
            >
              AUTO ON
            </Button>
            <Button
              onClick={() => serial.sendCommand('CMD:AUTO_OFF')}
              data-testid="btn-auto-off"
              variant="outline"
              className="w-full"
            >
              AUTO OFF
            </Button>
            <Button
              onClick={() => serial.sendCommand('CMD:EMERGENCY_STOP')}
              data-testid="btn-estop"
              variant="destructive"
              className="w-full col-span-2"
            >
              EMERGENCY STOP
            </Button>
          </div>
        </Card>

        <SessionPanel
          currentSessionId={sessionId}
          onSessionChange={setSessionId}
          isRecording={recorder.status === 'recording'}
          recordedCount={recorder.recordedCount}
          onStartRecording={recorder.startRecording}
          onStopRecording={recorder.stopRecording}
        />
      </div>

      {/* PID Status */}
      <Card className="p-4">
        <h2 className="text-lg font-bold mb-4 uppercase">PID Status</h2>
        <div className="grid grid-cols-3 gap-4">
          <div className="flex justify-between border-b pb-2">
            <span className="text-muted-foreground">Error</span>
            <span className="font-bold" data-testid="value-pid-error">
              {serial.telemetry?.pidError?.toFixed(4) ?? '0.0000'}
            </span>
          </div>
          <div className="flex justify-between border-b pb-2">
            <span className="text-muted-foreground">Output</span>
            <span className="font-bold" data-testid="value-pid-output">
              {serial.telemetry?.pidOutput?.toFixed(4) ?? '0.0000'}
            </span>
          </div>
          <div className="flex justify-between border-b pb-2">
            <span className="text-muted-foreground">Integral</span>
            <span className="font-bold" data-testid="value-pid-integral">
              {serial.telemetry?.pidIntegral?.toFixed(4) ?? '0.0000'}
            </span>
          </div>
        </div>
        <div className="mt-3 flex items-center gap-4">
          <span className="text-sm text-muted-foreground uppercase">Auto Mode:</span>
          <span
            className={`font-bold ${serial.telemetry?.autoMode ? 'text-primary' : 'text-muted-foreground'}`}
            data-testid="value-auto-mode"
          >
            {serial.telemetry?.autoMode ? 'ACTIVE' : 'OFF'}
          </span>
          <span className="text-sm text-muted-foreground uppercase">Vibration:</span>
          <span className="font-bold" data-testid="value-vibration">
            {serial.telemetry?.vibration?.toFixed(3) ?? '0.000'}
          </span>
        </div>
      </Card>

      {/* Error display */}
      {serial.error && (
        <Card className="p-4 border-destructive">
          <div className="text-sm text-destructive font-bold" data-testid="text-serial-error">
            ERROR: {serial.error}
          </div>
        </Card>
      )}
    </div>
  );
}
