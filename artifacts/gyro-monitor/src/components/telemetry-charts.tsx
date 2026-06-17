import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
  Area,
  AreaChart,
} from 'recharts';
import { Card } from '@/components/ui/card';
import type { TelemetryPoint } from '@/lib/use-telemetry-history';

function timeLabel(ts: number) {
  const d = new Date(ts);
  return `${d.getMinutes()}:${d.getSeconds().toString().padStart(2, '0')}.${Math.floor(d.getMilliseconds() / 100)}`;
}

const ChartTooltip = ({ active, payload, label }: any) => {
  if (active && payload && payload.length) {
    return (
      <div className="bg-card border border-border rounded p-2 text-xs shadow-lg">
        <div className="text-muted-foreground mb-1">{timeLabel(label)}</div>
        {payload.map((entry: any) => (
          <div key={entry.name} style={{ color: entry.color }}>
            {entry.name}: {typeof entry.value === 'number' ? entry.value.toFixed(2) : entry.value}
          </div>
        ))}
      </div>
    );
  }
  return null;
};

export function RPMChart({ data }: { data: TelemetryPoint[] }) {
  return (
    <Card className="p-4">
      <h3 className="text-sm uppercase tracking-wider font-bold mb-3 text-muted-foreground">RPM History</h3>
      <div className="h-48">
        <ResponsiveContainer width="100%" height="100%">
          <AreaChart data={data}>
            <CartesianGrid strokeDasharray="3 3" stroke="hsl(var(--border))" opacity={0.3} />
            <XAxis dataKey="timestamp" tickFormatter={timeLabel} tick={{ fontSize: 10, fill: 'hsl(var(--muted-foreground))' }} />
            <YAxis tick={{ fontSize: 10, fill: 'hsl(var(--muted-foreground))' }} domain={['auto', 'auto']} />
            <Tooltip content={<ChartTooltip />} />
            <Area type="monotone" dataKey="rpm" stroke="hsl(var(--chart-1))" fill="hsl(var(--chart-1))" fillOpacity={0.15} strokeWidth={2} />
          </AreaChart>
        </ResponsiveContainer>
      </div>
    </Card>
  );
}

export function TiltChart({ data }: { data: TelemetryPoint[] }) {
  return (
    <Card className="p-4">
      <h3 className="text-sm uppercase tracking-wider font-bold mb-3 text-muted-foreground">Tilt Magnitude</h3>
      <div className="h-48">
        <ResponsiveContainer width="100%" height="100%">
          <AreaChart data={data}>
            <CartesianGrid strokeDasharray="3 3" stroke="hsl(var(--border))" opacity={0.3} />
            <XAxis dataKey="timestamp" tickFormatter={timeLabel} tick={{ fontSize: 10, fill: 'hsl(var(--muted-foreground))' }} />
            <YAxis tick={{ fontSize: 10, fill: 'hsl(var(--muted-foreground))' }} domain={['auto', 'auto']} />
            <Tooltip content={<ChartTooltip />} />
            <Area type="monotone" dataKey="tiltMag" stroke="hsl(var(--chart-2))" fill="hsl(var(--chart-2))" fillOpacity={0.15} strokeWidth={2} />
          </AreaChart>
        </ResponsiveContainer>
      </div>
    </Card>
  );
}

export function ThrottleChart({ data }: { data: TelemetryPoint[] }) {
  return (
    <Card className="p-4">
      <h3 className="text-sm uppercase tracking-wider font-bold mb-3 text-muted-foreground">Throttle / PWM</h3>
      <div className="h-48">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={data}>
            <CartesianGrid strokeDasharray="3 3" stroke="hsl(var(--border))" opacity={0.3} />
            <XAxis dataKey="timestamp" tickFormatter={timeLabel} tick={{ fontSize: 10, fill: 'hsl(var(--muted-foreground))' }} />
            <YAxis tick={{ fontSize: 10, fill: 'hsl(var(--muted-foreground))' }} domain={['auto', 'auto']} />
            <Tooltip content={<ChartTooltip />} />
            <Line type="monotone" dataKey="throttle" name="Throttle (us)" stroke="hsl(var(--chart-3))" strokeWidth={2} dot={false} />
            <Line type="monotone" dataKey="pwmPct" name="PWM %" stroke="hsl(var(--chart-4))" strokeWidth={2} dot={false} />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </Card>
  );
}

export function PIDChart({ data }: { data: TelemetryPoint[] }) {
  return (
    <Card className="p-4">
      <h3 className="text-sm uppercase tracking-wider font-bold mb-3 text-muted-foreground">PID Response</h3>
      <div className="h-48">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={data}>
            <CartesianGrid strokeDasharray="3 3" stroke="hsl(var(--border))" opacity={0.3} />
            <XAxis dataKey="timestamp" tickFormatter={timeLabel} tick={{ fontSize: 10, fill: 'hsl(var(--muted-foreground))' }} />
            <YAxis tick={{ fontSize: 10, fill: 'hsl(var(--muted-foreground))' }} domain={['auto', 'auto']} />
            <Tooltip content={<ChartTooltip />} />
            <Line type="monotone" dataKey="pidError" name="Error" stroke="hsl(var(--chart-5))" strokeWidth={2} dot={false} />
            <Line type="monotone" dataKey="pidOutput" name="Output" stroke="hsl(var(--chart-1))" strokeWidth={2} dot={false} />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </Card>
  );
}
