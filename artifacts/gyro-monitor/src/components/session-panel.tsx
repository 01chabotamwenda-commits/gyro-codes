import { useState } from 'react';
import { Card } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { useCreateSession, useListSessions, getListSessionsQueryKey } from '@workspace/api-client-react';
import { useQueryClient } from '@tanstack/react-query';
import { useLocation } from 'wouter';
import { useToast } from '@/hooks/use-toast';

export function SessionPanel({
  currentSessionId,
  onSessionChange,
  isRecording,
  recordedCount,
  onStartRecording,
  onStopRecording,
}: {
  currentSessionId: number | null;
  onSessionChange: (id: number | null) => void;
  isRecording: boolean;
  recordedCount: number;
  onStartRecording: () => void;
  onStopRecording: () => void;
}) {
  const [name, setName] = useState('');
  const [targetRpm, setTargetRpm] = useState('10000');
  const [duration, setDuration] = useState('24');
  const [isOpen, setIsOpen] = useState(false);
  const createSession = useCreateSession();
  const { data: sessions } = useListSessions();
  const queryClient = useQueryClient();
  const [, setLocation] = useLocation();
  const { toast } = useToast();

  const handleCreate = () => {
    createSession.mutate(
      {
        data: {
          name: name || 'Untitled Session',
          targetRpm: parseFloat(targetRpm) || 10000,
          duration: parseFloat(duration) || 24,
        },
      },
      {
        onSuccess: (session) => {
          onSessionChange(session.id);
          queryClient.invalidateQueries({ queryKey: getListSessionsQueryKey() });
          setIsOpen(false);
          setName('');
          toast({
            title: 'Session Created',
            description: `Recording session "${session.name}" ready.`,
          });
        },
      }
    );
  };

  return (
    <Card className="p-4">
      <div className="flex items-center justify-between mb-4">
        <h2 className="text-lg font-bold uppercase">Session</h2>
        {currentSessionId && (
          <div className="text-xs text-muted-foreground">
            ID: {currentSessionId}
          </div>
        )}
      </div>

      <div className="space-y-3">
        {!isOpen && (
          <div className="space-y-2">
            <div className="flex items-center gap-2">
              <span className="text-sm text-muted-foreground">Current:</span>
              <span className="font-bold text-sm">
                {currentSessionId
                  ? sessions?.find((s) => s.id === currentSessionId)?.name || `Session #${currentSessionId}`
                  : 'None selected'}
              </span>
            </div>
            <div className="flex items-center gap-2">
              <span className="text-sm text-muted-foreground">Status:</span>
              <span className={`font-bold text-sm ${isRecording ? 'text-primary' : 'text-muted-foreground'}`}>
                {isRecording ? 'Recording' : 'Idle'}
              </span>
            </div>
            {isRecording && (
              <div className="text-xs text-muted-foreground">
                Recorded {recordedCount} data points
              </div>
            )}
          </div>
        )}

        {isOpen && (
          <div className="space-y-2">
            <Input
              placeholder="Session name"
              value={name}
              onChange={(e) => setName(e.target.value)}
              data-testid="input-session-name"
            />
            <div className="grid grid-cols-2 gap-2">
              <Input
                type="number"
                placeholder="Target RPM"
                value={targetRpm}
                onChange={(e) => setTargetRpm(e.target.value)}
                data-testid="input-target-rpm"
              />
              <Input
                type="number"
                placeholder="Duration (hrs)"
                value={duration}
                onChange={(e) => setDuration(e.target.value)}
                data-testid="input-duration"
              />
            </div>
          </div>
        )}

        <div className="grid grid-cols-2 gap-2">
          {!isOpen ? (
            <>
              <Button
                onClick={() => setIsOpen(true)}
                variant="outline"
                data-testid="btn-new-session"
                className="w-full"
              >
                NEW SESSION
              </Button>
              <Button
                onClick={() => setLocation('/sessions')}
                variant="outline"
                data-testid="btn-view-sessions"
                className="w-full"
              >
                VIEW ALL
              </Button>
              <Button
                onClick={isRecording ? onStopRecording : onStartRecording}
                variant={isRecording ? 'destructive' : 'default'}
                data-testid="btn-toggle-record"
                className="w-full col-span-2"
                disabled={!currentSessionId}
              >
                {isRecording ? 'STOP RECORDING' : 'START RECORDING'}
              </Button>
            </>
          ) : (
            <>
              <Button
                onClick={handleCreate}
                variant="default"
                data-testid="btn-create-session"
                className="w-full"
                disabled={createSession.isPending}
              >
                {createSession.isPending ? 'CREATING...' : 'CREATE'}
              </Button>
              <Button
                onClick={() => setIsOpen(false)}
                variant="secondary"
                data-testid="btn-cancel-session"
                className="w-full"
              >
                CANCEL
              </Button>
            </>
          )}
        </div>
      </div>
    </Card>
  );
}
