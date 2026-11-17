# Licence notes — plain language

**Licence: MIT** (full text in [`LICENSE`](LICENSE)).

---

## Resumen en claro (ES)

- Cualquiera puede usar, copiar, modificar y **vender** este código, incluso
  dentro de un producto cerrado.
- La única obligación de quien lo use es incluir el aviso de copyright y el
  texto de la licencia MIT.
- No hay garantía de ningún tipo: el software se entrega "tal cual".
- **Consecuencia que hay que aceptar:** alguien podría usar el motor DSP en un
  plugin comercial sin pagarte nada. Si en el futuro eso no se quiere, se puede
  relicenciar; lo ya publicado bajo MIT se queda bajo MIT.

## Why MIT here (EN)

- Matches how the formats themselves are licensed now: VST3 moved to MIT in
  2025 and CLAP has been MIT from the start, so the permissive choice is the
  consistent one for this codebase.
- Nothing about the licence blocks a later change: permissive → copyleft is
  possible for future versions, copyleft → permissive is not.
- Only the source is published in v1.0.0, so no commercial framework licence
  is triggered by the release (see below).

## Binaries and JUCE — the one real condition

| Component | Dependency | Licence situation |
|---|---|---|
| `src/dsp/` | none | MIT, no conditions |
| `src/plugin/`, `src/ui/` | JUCE 7.0.12 | JUCE is dual-licensed: **GPLv3 or paid commercial licence** |

- **Publishing source does not require a JUCE licence.**
- **Distributing binaries does**: you must either publish them under GPLv3 or
  buy a commercial JUCE licence.
- Do not casually upgrade to JUCE 8: it is **AGPLv3** or commercial, which is
  stricter than the GPLv3 option in JUCE 7.
- Decide this before shipping installers, not before pushing source.

## How to change the licence later

1. Replace `LICENSE` with the new text.
2. Update the SPDX identifier in the README and in any file headers.
3. Note the change in `CHANGELOG.md` with the version it takes effect from.

Previous releases remain available under the licence they were published with.

## Not legal advice

These notes are a practical summary for the maintainer, not legal advice. If
money or third-party code enters the picture, have a qualified professional
review it.
