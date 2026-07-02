# Lazy Watch German

A Pebble watchface that displays the time as spoken German phrases, using a large custom font with smooth slide animations on every minute change.

## Screenshots

| Pebble Time 2 (emery) | Pebble Time (basalt) | Pebble Classic (aplite) |
|:---:|:---:|:---:|
| ![halb Elf](screenshot_emery.png) | ![Zehn Nach Zehn](screenshots/basalt_0.6.2_20260702-104735.png) | ![Neun Uhr Acht Und Dreissig](screenshots/aplite.png) |

## Features

**German fuzzy time** — time is spoken in natural German rather than shown as digits:

| Time | Display |
|------|---------|
| 3:00 | `drei` |
| 12:00 | `zwölf` |
| 0:00 | `null` |
| 3:05 / 3:10 | `fünf nach drei` / `zehn nach drei` |
| 3:15 | `viertel nach drei` |
| 3:30 | `halb vier` *(German half-hour references the next hour)* |
| 3:45 | `viertel vor vier` |
| 3:50 / 3:55 | `zehn vor vier` / `fünf vor vier` |
| 3:22 | `drei uhr zwei und zwanzig` |

**Adaptive font sizing** — automatically picks from three font sizes (small, medium, large) so the text always fills the screen as large as possible without overflow.

**Slide animation** — on every minute tick, the outgoing text slides left off-screen while the new text slides in from the right.

**Configurable colors** — background and text color are independently configurable via the Pebble app. Settings persist across reboots.

**Timeline Quick View** — the layout adapts when Pebble's timeline peek overlays the watchface.

## Supported platforms

- **Aplite** — Pebble / Pebble Steel
- **Basalt** — Pebble Time / Pebble Time Steel
- **Diorite** — Pebble 2
- **Emery** — Pebble Time 2

## Building

```sh
pebble build
```

Requires the [Pebble SDK](https://developer.rebble.io/developer.pebble.com/sdk/index.html). The compiled `.pbw` is written to `build/`.

## License

MIT — see [LICENSE](LICENSE).
