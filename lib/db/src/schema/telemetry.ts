import { pgTable, serial, integer, real, boolean, timestamp } from "drizzle-orm/pg-core";
import { createInsertSchema } from "drizzle-zod";
import { z } from "zod/v4";

export const telemetryReadingsTable = pgTable("telemetry_readings", {
  id: serial("id").primaryKey(),
  sessionId: integer("session_id"),
  rpm: real("rpm").notNull().default(0),
  tiltX: real("tilt_x").notNull().default(0),
  tiltY: real("tilt_y").notNull().default(0),
  tiltZ: real("tilt_z").notNull().default(0),
  rotationZ: real("rotation_z").notNull().default(0),
  avgTiltMag: real("avg_tilt_mag").notNull().default(0),
  temp: real("temp").notNull().default(0),
  throttle: real("throttle").notNull().default(0),
  pwmPct: real("pwm_pct").notNull().default(0),
  vibration: real("vibration").notNull().default(0),
  autoMode: boolean("auto_mode").notNull().default(false),
  pidError: real("pid_error"),
  pidOutput: real("pid_output"),
  pidIntegral: real("pid_integral"),
  createdAt: timestamp("created_at", { withTimezone: true }).defaultNow().notNull(),
});

export const insertTelemetryReadingSchema = createInsertSchema(telemetryReadingsTable).omit({ id: true, createdAt: true });
export type InsertTelemetryReading = z.infer<typeof insertTelemetryReadingSchema>;
export type TelemetryReading = typeof telemetryReadingsTable.$inferSelect;
