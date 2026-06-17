import { Router, type IRouter } from "express";
import { db } from "@workspace/db";
import { sessionsTable } from "@workspace/db";
import { CreateSessionBody, UpdateSessionBody, GetSessionParams } from "@workspace/api-zod";
import { eq } from "drizzle-orm";

const router: IRouter = Router();

router.get("/sessions", async (_req, res) => {
  const sessions = await db.query.sessionsTable.findMany({
    orderBy: (sessionsTable, { desc }) => [desc(sessionsTable.createdAt)],
  });
  res.json(sessions);
});

router.post("/sessions", async (req, res) => {
  const parsed = CreateSessionBody.safeParse(req.body);
  if (!parsed.success) {
    res.status(400).json({ error: "Invalid session data" });
    return;
  }
  const [session] = await db.insert(sessionsTable).values(parsed.data).returning();
  res.status(201).json(session);
});

router.get("/sessions/:id", async (req, res) => {
  const parsed = GetSessionParams.safeParse(req.params);
  if (!parsed.success) {
    res.status(400).json({ error: "Invalid ID" });
    return;
  }
  const session = await db.query.sessionsTable.findFirst({
    where: eq(sessionsTable.id, parsed.data.id),
  });
  if (!session) {
    res.status(404).json({ error: "Session not found" });
    return;
  }
  res.json(session);
});

router.patch("/sessions/:id", async (req, res) => {
  const paramsParsed = GetSessionParams.safeParse(req.params);
  const bodyParsed = UpdateSessionBody.safeParse(req.body);
  if (!paramsParsed.success || !bodyParsed.success) {
    res.status(400).json({ error: "Invalid data" });
    return;
  }
  const [session] = await db
    .update(sessionsTable)
    .set(bodyParsed.data)
    .where(eq(sessionsTable.id, paramsParsed.data.id))
    .returning();
  if (!session) {
    res.status(404).json({ error: "Session not found" });
    return;
  }
  res.json(session);
});

export default router;
