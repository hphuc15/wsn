CREATE DATABASE IF NOT EXISTS wsn_db;
USE wsn_db;

CREATE TABLE device (
    device_id   INT AUTO_INCREMENT PRIMARY KEY,
    name        VARCHAR(64)
);

CREATE TABLE telemetry (
    id          BIGINT AUTO_INCREMENT PRIMARY KEY,
    device_id   INT NOT NULL,
    ts          TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    temp        FLOAT,
    humi        FLOAT,
    soil_humi   FLOAT,
    FOREIGN KEY (device_id) REFERENCES device(device_id)
);

CREATE INDEX idx_telemetry_device_ts ON telemetry(device_id, ts DESC);

INSERT INTO device (device_id, name) VALUES (1, 'esp32_1');