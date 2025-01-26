# Smart Plug

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html

## Notes:

**Publish a connected state with ephemeral data to the topic.**

```bash
mosquitto_pub \
  -h localhost \
  -p 1883 \
  -t "devices/1d83e896-b059-47af-974b-1a9eb6230189/state" \
  -m '{"connected": true, "ephemeralData": { "brightness": 50, "poweredOn": true }}' \
  -r \
  --will-topic "devices/1d83e896-b059-47af-974b-1a9eb6230189/state" \
  --will-payload '{"connected": false}'
```

**Publish a disconnected state to the topic.**

```bash
mosquitto_pub \
  -h localhost \
  -p 1883 \
  -t "devices/1d83e896-b059-47af-974b-1a9eb6230189/state" \
  -m '{"connected": false }' \
  -r \
  --will-topic "devices/1d83e896-b059-47af-974b-1a9eb6230189/state" \
  --will-payload '{"connected": false}'
```

**Subscribe to a topic**

```
mosquitto_sub -h localhost -p 1883 -t "devices/+/requested_state"
```

**Publish a message to the requested_state topic**

```bash
mosquitto_pub \
  -h localhost \
  -p 1883 \
  -t "device/1234/requested_state" \
  -m '{"poweredOn": true, "brightness": 55 }'
```
