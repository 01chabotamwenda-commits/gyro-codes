import { Router, type IRouter } from "express";
import healthRouter from "./health";
import telemetryRouter from "./telemetry";
import sessionsRouter from "./sessions";
import deviceRouter from "./device";

const router: IRouter = Router();

router.use(healthRouter);
router.use(telemetryRouter);
router.use(sessionsRouter);
router.use(deviceRouter);

export default router;
