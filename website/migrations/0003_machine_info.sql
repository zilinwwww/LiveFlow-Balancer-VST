-- Add Telemetry Data to Licenses
ALTER TABLE licenses ADD COLUMN machine_name TEXT;
ALTER TABLE licenses ADD COLUMN machine_info TEXT;
