import { Router, type IRouter } from "express";
import { db } from "@workspace/db";
import { telemetryReadingsTable } from "@workspace/db";
import { CreateTelemetryBody, ListTelemetryQueryParams } from "@workspace/api-zod";
import { desc, eq } from "drizzle-orm";

const router: IRouter = Router();

router.get("/telemetry", async (req, res) => {
  const parsed = ListTelemetryQueryParams.safeParse(req.query);
  if (!parsed.success) {
    res.status(400).json({ error: "Invalid query params" });
    return;
  }
  const limit = parsed.data.limit ?? 100;
  const sessionId = parsed.data.sessionId ?? null;

  const readings = await db.query.telemetryReadingsTable.findMany({
    where: sessionId ? eq(telemetryReadingsTable.sessionId, sessionId) : undefined,
    orderBy: [desc(telemetryReadingsTable.createdAt)],
    limit,
  });
  res.json(readings);
});

router.post("/telemetry", async (req, res) => {
  const parsed = CreateTelemetryBody.safeParse(req.body);
  if (!parsed.success) {
    res.status(400).json({ error: "Invalid telemetry data" });
    return;
  }
  const [reading] = await db.insert(telemetryReadingsTable).values(parsed.data).returning();
  res.status(201).json(reading);
});

export default router;
