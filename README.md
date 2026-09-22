# AwsIoTRng


# ESP32 → AWS IoT Core (MQTT over TLS)

A minimal but complete example of an ESP32 connecting to AWS IoT Core over
MQTT with mutual TLS authentication, publishing sensor-style readings to
the cloud in real time.

## What this does

- Connects the ESP32 to WiFi
- Authenticates to AWS IoT Core using a unique X.509 certificate and
  private key (device identity, not a shared password)
- Publishes a JSON payload every 5 seconds over an encrypted MQTT
  connection
- Readings are visible live in the AWS IoT Core console, or by any
  other service subscribed to the same topic

Currently publishes a placeholder value; swapping in a real sensor
reading is a small, contained change once a sensor is wired in, the
cloud connectivity itself is the part that's done.

## Stack

| Layer | Tool |
|---|---|
| Hardware | Generic ESP-WROOM-32 dev board |
| Firmware framework | Arduino (via PlatformIO) |
| TLS / networking | WiFiClientSecure |
| MQTT client | [PubSubClient](https://github.com/knolleary/pubsubclient) |
| Cloud | AWS IoT Core |

## AWS setup

Resources are created via the AWS CLI rather than the console, so the
setup is scriptable and repeatable:

```bash
# Policy scoped to a specific publish topic, not wildcard-everything
aws iot create-policy --policy-name esp32-test-policy --policy-document file://iot-policy.json

# Certificate and keys (AWS only returns the private key once, at creation)
aws iot create-keys-and-certificate --set-as-active \
  --certificate-pem-outfile certificate.pem.crt \
  --public-key-outfile public.pem.key \
  --private-key-outfile private.pem.key

# The device identity itself
aws iot create-thing --thing-name esp32-test-device

# Link everything together
aws iot attach-thing-principal --thing-name esp32-test-device --principal <certificateArn>
aws iot attach-policy --policy-name esp32-test-policy --target <certificateArn>

# The account-specific endpoint the device connects to
aws iot describe-endpoint --endpoint-type iot:Data-ATS
```

## Firmware setup

1. Install [PlatformIO](https://platformio.org/)
2. Clone this repo, `cd` into it
3. Download the [Amazon Root CA1](https://www.amazontrust.com/repository/AmazonRootCA1.pem)
   certificate
4. In `src/main.cpp`, fill in:
   - Your WiFi SSID and password
   - Your AWS IoT endpoint (from `describe-endpoint` above)
   - The contents of `AmazonRootCA1.pem`, `certificate.pem.crt`, and
     `private.pem.key` into their respective `R"EOF(...)EOF"` blocks
5. Build and upload:
   ```
   pio run -t upload
   pio device monitor
   ```

**Never commit your certificate or private key files.** They're
excluded via `.gitignore`; treat them the same as any other credential.

## Verifying it works

AWS Console → IoT Core → MQTT test client → subscribe to
`esp32/test/data`. Readings published by the device appear live.

## Notes from getting this working

A few non-obvious issues came up building this, worth documenting
since they're easy to hit again:

- **DNS resolution can fail intermittently on ESP32** depending on
  what DNS server the router hands out via DHCP. Fixed by explicitly
  setting a public DNS server (`WiFi.config(...)`) before connecting.
- **Most ESP32 boards have no real-time clock.** AWS IoT Core's TLS
  handshake validates certificate validity dates, which fails silently
  if the device thinks it's 1970. Syncing time over NTP before
  attempting the AWS connection is required, not optional.
- **`WiFiClientSecure::lastError()` can return misleading text** in
  some library versions (a documented upstream issue), don't trust its
  output literally when debugging a connection failure.
- **A certificate can be valid and correctly formatted but still fail
  to connect** if it isn't actually attached to both the thing and a
  policy in AWS. `aws iot list-attached-policies --target <certArn>`
  is the fastest way to confirm this is wired up correctly.

## Next steps

- Replace the placeholder reading with a real sensor value
- Subscribe to a command topic to control the device from the cloud
  side, not just publish outward
