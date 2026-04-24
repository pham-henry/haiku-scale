const express = require("express");
const cors = require("cors");
const sqlite3 = require("sqlite3").verbose();

const app = express();
const PORT = Number(process.env.PORT) || 3000;
const DB_PATH = process.env.DB_PATH || "./tea.db";

app.use(cors());
app.use(express.json());

const db = new sqlite3.Database(DB_PATH, (err) => {
  if (err) {
    console.error("Failed to open SQLite DB:", err.message);
    process.exit(1);
  }
  console.log(`Connected to SQLite database at ${DB_PATH}.`);
});

function addColumnIfMissing(table, column, definition) {
  db.all(`PRAGMA table_info(${table})`, (err, rows) => {
    if (err) {
      console.error(`Failed to inspect ${table}:`, err.message);
      return;
    }

    if (!rows.some((row) => row.name === column)) {
      db.run(`ALTER TABLE ${table} ADD COLUMN ${column} ${definition}`, (alterErr) => {
        if (alterErr) {
          console.error(`Failed to add ${column}:`, alterErr.message);
        }
      });
    }
  });
}

db.serialize(() => {
  db.run(`
    CREATE TABLE IF NOT EXISTS tea_measurements (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      device_id TEXT NOT NULL,
      unix_time INTEGER,
      uptime_ms INTEGER,
      firmware_version TEXT,
      raw INTEGER,
      median_g REAL,
      weight_g REAL NOT NULL,
      variance_g2 REAL,
      state TEXT NOT NULL,
      signal_ok INTEGER NOT NULL,
      stable INTEGER,
      outlier_rejected INTEGER,
      buffered INTEGER NOT NULL,
      report_reason TEXT,
      created_at DATETIME DEFAULT CURRENT_TIMESTAMP
    )
  `);

  db.run(`
    CREATE INDEX IF NOT EXISTS idx_tea_measurements_device_time
    ON tea_measurements(device_id, unix_time)
  `);

  addColumnIfMissing("tea_measurements", "uptime_ms", "INTEGER");
  addColumnIfMissing("tea_measurements", "firmware_version", "TEXT");
  addColumnIfMissing("tea_measurements", "raw", "INTEGER");
  addColumnIfMissing("tea_measurements", "median_g", "REAL");
  addColumnIfMissing("tea_measurements", "variance_g2", "REAL");
  addColumnIfMissing("tea_measurements", "stable", "INTEGER");
  addColumnIfMissing("tea_measurements", "outlier_rejected", "INTEGER");
  addColumnIfMissing("tea_measurements", "report_reason", "TEXT");
});

app.get("/health", (req, res) => {
  res.json({
    ok: true,
    service: "tea-backend",
    port: PORT,
    db_path: DB_PATH
  });
});

app.post("/api/tea-weight", (req, res) => {
  const {
    device_id,
    unix_time,
    uptime_ms,
    firmware_version,
    raw,
    median_g,
    weight_g,
    variance_g2,
    state,
    signal_ok,
    stable,
    outlier_rejected,
    buffered,
    report_reason
  } = req.body;

  if (
    typeof device_id !== "string" ||
    typeof weight_g !== "number" ||
    typeof state !== "string" ||
    typeof signal_ok !== "boolean" ||
    typeof buffered !== "boolean" ||
    (stable !== undefined && typeof stable !== "boolean") ||
    (outlier_rejected !== undefined && typeof outlier_rejected !== "boolean")
  ) {
    return res.status(400).json({
      error: "Invalid payload",
      expected: {
        device_id: "string",
        unix_time: "number|null",
        uptime_ms: "number|null",
        weight_g: "number",
        state: "string",
        signal_ok: "boolean",
        stable: "boolean",
        buffered: "boolean",
        report_reason: "string"
      }
    });
  }

  const sql = `
    INSERT INTO tea_measurements
    (
      device_id,
      unix_time,
      uptime_ms,
      firmware_version,
      raw,
      median_g,
      weight_g,
      variance_g2,
      state,
      signal_ok,
      stable,
      outlier_rejected,
      buffered,
      report_reason
    )
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
  `;

  db.run(
    sql,
    [
      device_id,
      Number.isFinite(unix_time) ? unix_time : null,
      Number.isFinite(uptime_ms) ? uptime_ms : null,
      typeof firmware_version === "string" ? firmware_version : null,
      Number.isFinite(raw) ? raw : null,
      Number.isFinite(median_g) ? median_g : null,
      weight_g,
      Number.isFinite(variance_g2) ? variance_g2 : null,
      state,
      signal_ok ? 1 : 0,
      stable ? 1 : 0,
      outlier_rejected ? 1 : 0,
      buffered ? 1 : 0,
      typeof report_reason === "string" ? report_reason : null
    ],
    function (err) {
      if (err) {
        console.error("Insert failed:", err.message);
        return res.status(500).json({ error: "Database insert failed" });
      }

      console.log(
        `[POST OK] id=${this.lastID} device=${device_id} weight=${weight_g.toFixed(1)}g state=${state} reason=${report_reason || "unknown"} stable=${Boolean(stable)} buffered=${buffered}`
      );

      res.status(201).json({
        ok: true,
        id: this.lastID
      });
    }
  );
});

app.get("/api/tea-weight", (req, res) => {
  const limit = Math.min(Number(req.query.limit) || 50, 500);
  const deviceId = req.query.device_id;
  const params = [];
  let where = "";

  if (typeof deviceId === "string" && deviceId.length > 0) {
    where = "WHERE device_id = ?";
    params.push(deviceId);
  }

  params.push(limit);

  db.all(
    `
    SELECT *
    FROM tea_measurements
    ${where}
    ORDER BY id DESC
    LIMIT ?
    `,
    params,
    (err, rows) => {
      if (err) {
        console.error("Query failed:", err.message);
        return res.status(500).json({ error: "Database query failed" });
      }

      res.json(rows);
    }
  );
});

app.get("/api/tea-weight/latest", (req, res) => {
  const deviceId = req.query.device_id;
  const params = [];
  let where = "";

  if (typeof deviceId === "string" && deviceId.length > 0) {
    where = "WHERE device_id = ?";
    params.push(deviceId);
  }

  db.get(
    `
    SELECT *
    FROM tea_measurements
    ${where}
    ORDER BY id DESC
    LIMIT 1
    `,
    params,
    (err, row) => {
      if (err) {
        console.error("Query failed:", err.message);
        return res.status(500).json({ error: "Database query failed" });
      }

      res.json(row || null);
    }
  );
});

app.get("/api/tea-weight/device/:deviceId", (req, res) => {
  const { deviceId } = req.params;
  const limit = Math.min(Number(req.query.limit) || 50, 500);

  db.all(
    `
    SELECT *
    FROM tea_measurements
    WHERE device_id = ?
    ORDER BY id DESC
    LIMIT ?
    `,
    [deviceId, limit],
    (err, rows) => {
      if (err) {
        console.error("Query failed:", err.message);
        return res.status(500).json({ error: "Database query failed" });
      }

      res.json(rows);
    }
  );
});

app.get("/api/devices", (req, res) => {
  db.all(
    `
    SELECT
      device_id,
      COUNT(*) AS samples,
      MAX(created_at) AS last_seen,
      MAX(id) AS latest_id
    FROM tea_measurements
    GROUP BY device_id
    ORDER BY last_seen DESC
    `,
    [],
    (err, rows) => {
      if (err) {
        console.error("Device query failed:", err.message);
        return res.status(500).json({ error: "Database query failed" });
      }

      res.json(rows);
    }
  );
});

app.listen(PORT, "0.0.0.0", () => {
  console.log(`Tea backend listening on http://0.0.0.0:${PORT}`);
});
