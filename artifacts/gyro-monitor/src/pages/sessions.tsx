import { useState } from 'react';
import { useLocation, Link } from 'wouter';
import { useListSessions, useGetSession, useUpdateSession, getListSessionsQueryKey, getGetSessionQueryKey } from '@workspace/api-client-react';
import { useQueryClient } from '@tanstack/react-query';
import { Card } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Badge } from '@/components/ui/badge';
import { useToast } from '@/hooks/use-toast';
import { ArrowLeft, Play, Square, Trash2 } from 'lucide-react';

function statusColor(status: string) {
  switch (status) {
    case 'running': return 'bg-primary text-primary-foreground';
    case 'completed': return 'bg-secondary text-secondary-foreground';
    case 'error': return 'bg-destructive text-destructive-foreground';
    default: return 'bg-muted text-muted-foreground';
  }
}

export default function Sessions() {
  const { data: sessions, isLoading } = useListSessions();
  const [selectedId, setSelectedId] = useState<number | null>(null);
  const { data: selectedSession } = useGetSession(
    selectedId ?? 0,
    { query: { enabled: !!selectedId, queryKey: getGetSessionQueryKey(selectedId ?? 0) } }
  );
  const updateSession = useUpdateSession();
  const queryClient = useQueryClient();
  const { toast } = useToast();
  const [, setLocation] = useLocation();

  const handleStatus = (id: number, status: 'running' | 'completed' | 'error' | 'idle') => {
    updateSession.mutate(
      { id, data: { status } },
      {
        onSuccess: () => {
          queryClient.invalidateQueries({ queryKey: getListSessionsQueryKey() });
          if (selectedId) {
            queryClient.invalidateQueries({ queryKey: getGetSessionQueryKey(selectedId) });
          }
          toast({ title: 'Session Updated', description: `Status changed to ${status}` });
        },
      }
    );
  };

  return (
    <div className="p-6 space-y-6">
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-4">
          <Button
            variant="ghost"
            size="icon"
            onClick={() => setLocation('/')}
            data-testid="btn-back"
          >
            <ArrowLeft className="h-5 w-5" />
          </Button>
          <h1 className="text-2xl font-bold tracking-tight uppercase">Sessions</h1>
        </div>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Session List */}
        <div className="lg:col-span-2 space-y-4">
          {isLoading && (
            <div className="text-muted-foreground">Loading sessions...</div>
          )}
          {sessions && sessions.length === 0 && (
            <Card className="p-8 text-center">
              <div className="text-muted-foreground">No sessions recorded yet.</div>
              <div className="text-xs text-muted-foreground mt-2">
                Go to the dashboard and create a new session to start recording telemetry.
              </div>
            </Card>
          )}
          {sessions?.map((session) => (
            <Card
              key={session.id}
              className={`p-4 cursor-pointer transition-colors ${
                selectedId === session.id ? 'border-primary' : 'hover:border-muted-foreground/30'
              }`}
              onClick={() => setSelectedId(session.id)}
              data-testid={`session-card-${session.id}`}
            >
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-4">
                  <Badge className={statusColor(session.status)}>{session.status}</Badge>
                  <div>
                    <div className="font-bold">{session.name}</div>
                    <div className="text-xs text-muted-foreground">
                      Target {session.targetRpm} RPM / {session.duration}h
                    </div>
                  </div>
                </div>
                <div className="flex items-center gap-2">
                  <div className="text-xs text-muted-foreground">
                    {new Date(session.createdAt).toLocaleDateString()}
                  </div>
                </div>
              </div>
            </Card>
          ))}
        </div>

        {/* Session Detail */}
        <div className="lg:col-span-1">
          {selectedSession ? (
            <Card className="p-4 space-y-4">
              <div>
                <h2 className="text-lg font-bold uppercase">{selectedSession.name}</h2>
                <div className="text-xs text-muted-foreground mt-1">
                  ID: {selectedSession.id}
                </div>
              </div>

              <div className="space-y-2">
                <div className="flex justify-between text-sm">
                  <span className="text-muted-foreground">Status</span>
                  <Badge className={statusColor(selectedSession.status)}>
                    {selectedSession.status}
                  </Badge>
                </div>
                <div className="flex justify-between text-sm">
                  <span className="text-muted-foreground">Target RPM</span>
                  <span className="font-bold">{selectedSession.targetRpm}</span>
                </div>
                <div className="flex justify-between text-sm">
                  <span className="text-muted-foreground">Duration</span>
                  <span className="font-bold">{selectedSession.duration}h</span>
                </div>
                <div className="flex justify-between text-sm">
                  <span className="text-muted-foreground">Created</span>
                  <span className="font-bold">{new Date(selectedSession.createdAt).toLocaleString()}</span>
                </div>
                {selectedSession.startedAt && (
                  <div className="flex justify-between text-sm">
                    <span className="text-muted-foreground">Started</span>
                    <span className="font-bold">{new Date(selectedSession.startedAt).toLocaleString()}</span>
                  </div>
                )}
                {selectedSession.endedAt && (
                  <div className="flex justify-between text-sm">
                    <span className="text-muted-foreground">Ended</span>
                    <span className="font-bold">{new Date(selectedSession.endedAt).toLocaleString()}</span>
                  </div>
                )}
              </div>

              <div className="grid grid-cols-2 gap-2">
                <Button
                  onClick={() => handleStatus(selectedSession.id, 'running')}
                  variant="outline"
                  data-testid="btn-session-run"
                  className="w-full"
                  disabled={updateSession.isPending}
                >
                  <Play className="h-4 w-4 mr-1" />
                  RUN
                </Button>
                <Button
                  onClick={() => handleStatus(selectedSession.id, 'completed')}
                  variant="outline"
                  data-testid="btn-session-complete"
                  className="w-full"
                  disabled={updateSession.isPending}
                >
                  <Square className="h-4 w-4 mr-1" />
                  COMPLETE
                </Button>
                <Button
                  onClick={() => handleStatus(selectedSession.id, 'error')}
                  variant="outline"
                  data-testid="btn-session-error"
                  className="w-full col-span-2"
                  disabled={updateSession.isPending}
                >
                  <Trash2 className="h-4 w-4 mr-1" />
                  MARK ERROR
                </Button>
              </div>
            </Card>
          ) : (
            <Card className="p-8 text-center">
              <div className="text-muted-foreground">Select a session to view details</div>
            </Card>
          )}
        </div>
      </div>
    </div>
  );
}
