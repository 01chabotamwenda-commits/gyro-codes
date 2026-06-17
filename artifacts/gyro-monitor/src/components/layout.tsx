import { Link } from "wouter";

export function Sidebar() {
  return (
    <div className="w-64 border-r border-border bg-card flex flex-col h-full">
      <div className="p-4 border-b border-border">
        <h1 className="text-xl font-bold tracking-widest text-primary uppercase">GYRO_OS</h1>
        <p className="text-xs text-muted-foreground uppercase mt-1">Terminal V1.0</p>
      </div>
      <nav className="flex-1 p-4 space-y-2">
        <Link href="/" className="block px-4 py-2 rounded bg-muted text-foreground hover:bg-secondary font-bold uppercase tracking-wider text-sm transition-colors">
          Telemetry
        </Link>
        <Link href="/sessions" className="block px-4 py-2 rounded text-muted-foreground hover:bg-secondary hover:text-foreground font-bold uppercase tracking-wider text-sm transition-colors">
          Sessions
        </Link>
      </nav>
      <div className="p-4 border-t border-border text-xs text-muted-foreground uppercase">
        SYSTEM: ONLINE<br/>
        BAUD: 115200
      </div>
    </div>
  );
}
