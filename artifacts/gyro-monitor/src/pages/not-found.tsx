import { Card, CardContent } from "@/components/ui/card";
import { AlertCircle } from "lucide-react";
import { Link } from "wouter";

export default function NotFound() {
  return (
    <div className="min-h-screen w-full flex items-center justify-center bg-background">
      <Card className="w-full max-w-md mx-4 border-destructive/50">
        <CardContent className="pt-6">
          <div className="flex mb-4 gap-2">
            <AlertCircle className="h-8 w-8 text-destructive" />
            <h1 className="text-2xl font-bold text-foreground">404 Not Found</h1>
          </div>

          <p className="mt-4 text-sm text-muted-foreground">
            The requested resource does not exist.
          </p>
          <div className="mt-4">
            <Link href="/" className="text-primary hover:underline text-sm font-bold uppercase tracking-wider">
              Return to Dashboard
            </Link>
          </div>
        </CardContent>
      </Card>
    </div>
  );
}
