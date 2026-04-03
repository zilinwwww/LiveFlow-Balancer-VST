-- LiveFlow Activation System — Initial Schema

CREATE TABLE IF NOT EXISTS users (
  id         TEXT PRIMARY KEY,
  email      TEXT UNIQUE NOT NULL,
  password   TEXT NOT NULL,
  name       TEXT,
  created_at TEXT DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS licenses (
  id          TEXT PRIMARY KEY,
  user_id     TEXT NOT NULL REFERENCES users(id),
  product     TEXT NOT NULL,
  key         TEXT UNIQUE NOT NULL,
  status      TEXT NOT NULL DEFAULT 'available',
  machine_id  TEXT,
  bound_at    TEXT,
  created_at  TEXT DEFAULT (datetime('now'))
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_licenses_user_product ON licenses(user_id, product);
CREATE INDEX IF NOT EXISTS idx_licenses_key ON licenses(key);
