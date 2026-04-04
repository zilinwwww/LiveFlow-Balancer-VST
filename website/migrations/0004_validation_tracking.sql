ALTER TABLE licenses ADD COLUMN validation_count INTEGER DEFAULT 0;
ALTER TABLE licenses ADD COLUMN last_validated_at TEXT;
