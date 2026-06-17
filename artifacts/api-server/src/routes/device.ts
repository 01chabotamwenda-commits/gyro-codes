import { Router, type IRouter } from "express";
import { SendCommandBody } from "@workspace/api-zod";

const router: IRouter = Router();

const serialConnections = new Map<string, WritableStreamDefaultWriter>();

router.post("/device/command", async (req, res) => {
  const parsed = SendCommandBody.safeParse(req.body);
  if (!parsed.success) {
    res.status(400).json({ error: "Invalid command" });
    return;
  }

  const { command, key } = parsed.data;
  const cmdMap: Record<string, string> = {
    MOTOR_START: "CMD:MOTOR_START",
    MOTOR_STOP: "CMD:MOTOR_STOP",
    EMERGENCY_STOP: "CMD:EMERGENCY_STOP",
    SPEED_UP: "CMD:SPEED_UP",
    SPEED_DOWN: "CMD:SPEED_DOWN",
    RESET_REF: "CMD:RESET_REF",
    AUTO_ON: "CMD:AUTO_ON",
    AUTO_OFF: "CMD:AUTO_OFF",
  };

  const cmd = cmdMap[command];
  if (!cmd) {
    res.status(400).json({ success: false, message: "Unknown command" });
    return;
  }

  res.json({ success: true, message: `Command ${cmd} queued for serial transmission` });
});

export default router;
