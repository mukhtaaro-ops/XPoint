# X4 Pro+ Factory Recovery

## Purpose

Factory recovery must remain independent of X4 Pro+. The recovery path may not depend on the custom firmware booting, mounting the SD card or exposing a USB menu.

## Required backup

Use the verified full 16 MiB factory image created from this exact XTEINK X4 Pro.

Before any custom flash:

1. Confirm the file size is exactly **16,777,216 bytes**.
2. Recalculate its SHA-256.
3. Compare it with the SHA-256 recorded when the backup was originally verified.
4. Keep at least two copies of the backup in separate locations.

The repository deliberately does not store the user's factory image or its private backup hash.

## ROM-download requirement

Before the first X4 Pro+ physical flash, demonstrate a repeatable way to force this exact X4 Pro into the ESP32-S3 ROM download loader even when the application firmware is completely unusable.

Do not substitute an in-firmware USB/download option for this test. The recovery path exists specifically for the case where no application can boot.

The exact physical button/pad sequence remains **unverified** for this device and is therefore a release blocker.

## Factory restore command

Once the X4 Pro is confirmed to be in the ESP32-S3 ROM download loader and the correct serial port is known, the verified full factory image is restored from address:

`0x00000000`

Use the same known-good esptool workflow that was used when the backup was successfully restored previously. Do not write the full 16 MiB factory image at the X4 Pro+ application offset.

## Offsets are not interchangeable

- Full 16 MiB factory image restore: **0x00000000**
- X4 Pro+ application image: **0x00010000**

A factory image is a complete flash snapshot. An application image is only one partition. Treating them as interchangeable can leave the device unbootable.

## Post-restore verification

After a factory restore:

- boot the device normally;
- confirm the stock home screen appears;
- open a known book;
- test page turns and sleep/wake;
- confirm the SD card is readable.

Only after this procedure has been physically demonstrated should the recovery gate in `X4_PRO_PLUS_RELEASE_CHECKLIST.md` be checked.
