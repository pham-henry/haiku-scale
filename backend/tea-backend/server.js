const express = require("express");
const cors = require("cors");
const sqlite3 = require("sqlite3").verbose();

const app = express();
const PORT = 3000;

// Middleware
app.use(cors());
app.use(express.json());

// SQLite setup
const db = new sqlite3.Database("./tea.db", (err) => {
  if (err) {
    console.error("Failed to open SQLite DB:", err.message);
    process.exit(1);
  }
  console.log("Connected to SQLite database.");
});

db.serialize(() => {
  db.run(`
    CREATE TABLE IF NOT EXISTS tea_measurements (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      device_id TEXT NOT NULL,
      unix_time INTEGER,
      weight_g REAL NOT NULL,
      state TEXT NOT NULL,
      signal_ok INTEGER NOT NULL,
      buffered INTEGER NOT NULL,
      created_at DATETIME DEFAULT CURRENT_TIMESTAMP
    )
  `);

  db.run(`
    CREATE INDEX IF NOT EXISTS idx_tea_measurements_device_time
    ON tea_measurements(device_id, unix_time)
  `);
});

// Health check
app.get("/health", (req, res) => {
  res.json({
    ok: true,
    service: "tea-backend",
    port: PORT
  });
});

// Accept ESP32 measurements
app.post("/api/tea-weight", (req, res) => {
  const {
    device_id,
    unix_time,
    weight_g,
    state,
    signal_ok,
    buffered
  } = req.body;

  if (
    typeof device_id !== "string" ||
    typeof weight_g !== "number" ||
    typeof state !== "string" ||
    typeof signal_ok !== "boolean" ||
    typeof buffered !== "boolean"
  ) {
    return res.status(400).json({
      error: "Invalid payload",
      expected: {
        device_id: "string",
        unix_time: "number|null",
        weight_g: "number",
        state: "string",
        signal_ok: "boolean",
        buffered: "boolean"
      }
    });
  }

  const sql = `
    INSERT INTO tea_measurements
    (device_id, unix_time, weight_g, state, signal_ok, buffered)
    VALUES (?, ?, ?, ?, ?, ?)
  `;

  db.run(
    sql,
    [
      device_id,
      Number.isFinite(unix_time) ? unix_time : null,
      weight_g,
      state,
      signal_ok ? 1 : 0,
      buffered ? 1 : 0
    ],
    function (err) {
      if (err) {
        console.error("Insert failed:", err.message);
        return res.status(500).json({ error: "Database insert failed" });
      }

      res.status(201).json({
        ok: true,
        id: this.lastID
      });
    }
  );
});

// Get recent measurements
app.get("/api/tea-weight", (req, res) => {
  const limit = Math.min(Number(req.query.limit) || 50, 500);

  db.all(
    `
    SELECT
      id,
      device_id,
      unix_time,
      weight_g,
      state,
      signal_ok,
      buffered,
      created_at
    FROM tea_measurements
    ORDER BY id DESC
    LIMIT ?
    `,
    [limit],
    (err, rows) => {
      if (err) {
        console.error("Query failed:", err.message);
        return res.status(500).json({ error: "Database query failed" });
      }

      res.json(rows);
    }
  );
});

// Get latest measurement
app.get("/api/tea-weight/latest", (req, res) => {
  db.get(
    `
    SELECT
      id,
      device_id,
      unix_time,
      weight_g,
      state,
      signal_ok,
      buffered,
      created_at
    FROM tea_measurements
    ORDER BY id DESC
    LIMIT 1
    `,
    [],
    (err, row) => {
      if (err) {
        console.error("Query failed:", err.message);
        return res.status(500).json({ error: "Database query failed" });
      }

      res.json(row || null);
    }
  );
});

// Optional: filter by device_id
app.get("/api/tea-weight/device/:deviceId", (req, res) => {
  const { deviceId } = req.params;
  const limit = Math.min(Number(req.query.limit) || 50, 500);

  db.all(
    `
    SELECT
      id,
      device_id,
      unix_time,
      weight_g,
      state,
      signal_ok,
      buffered,
      created_at
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

app.listen(PORT, "0.0.0.0", () => {
  console.log(`Tea backend listening on http://0.0.0.0:${PORT}`);
});