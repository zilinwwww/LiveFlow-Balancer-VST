-- Custom Verification Codes Table

CREATE TABLE IF NOT EXISTS verification_codes (
  email      TEXT PRIMARY KEY,
  code       TEXT NOT NULL,
  expires_at INTEGER NOT NULL
);
