# TalkBox Reference 1.2.0 — UI e implementación

Fecha: 2026-11-17 · Tag: `v1.2.0` · Instalador: `TalkBoxReference-Setup-1.2.0.exe`

---

## 1. Lo que estaba mal (diagnóstico)

Medido, no supuesto:

| Problema | Magnitud |
|---|---|
| Parámetros declarados | 44 |
| Parámetros con control en la UI | **0** |
| Pestañas | 5, **todas vacías** |
| Parámetros que no llegaban al DSP | **6 de 44** |
| Bug: la posición del pad no articulaba | F1 clavado en 450 Hz |
| Bug: el glide no tenía efecto útil | todo el recorrido en el 5 % final del knob |

Los 6 parámetros muertos eran `tubeCharacter`, `mouthSize`, `articSmooth`,
`micProximity`, `synthOctave` y `ambience`: existían en la lista de parámetros y
sus valores se ignoraban por completo.

---

## 2. UI nueva

**Estructura tipo Arturia** (secciones agrupadas y enmarcadas, barra de
cabecera, tira de pestañas) con **controles y paleta tipo Valhalla** (perillas
grandes, valor siempre visible bajo cada perilla, un solo acento ámbar sobre
fondo oscuro cálido).

Componentes nuevos, todos vectoriales y escalables a cualquier DPI:

| archivo | qué aporta |
|---|---|
| `src/ui/TbLookAndFeel.h` | paleta, tipografías, primitivas de sección y rótulo |
| `src/ui/TbKnob.h` | perilla con arco de valor, lectura numérica permanente, arco bipolar desde el centro, arrastre fino con shift, doble clic = reset |
| `src/ui/TbControls.h` | selector segmentado, interruptor con LED, desplegable |
| `src/ui/VowelPad.cpp` | el pad es ahora un **plano F1/F2 real**, no un cuadrado abstracto |

### El pad es un gráfico de formantes de verdad

Las nueve anclas se colocan **según sus formantes publicados**, no según sus
coordenadas internas. Así el dibujo muestra la vocal que el modelo *produce*.
Al arrastrar, se invierte numéricamente la tabla de vocales, de modo que el
marcador cae en la vocal que estás señalando con el ratón, no en una posición
abstracta.

### Las 5 pestañas, todas funcionales

| pestaña | contenido |
|---|---|
| SIMPLE | pad + 8 macros (mix, salida, drive, tubo, boca, articulación, crisp, proximidad) |
| VOCAL TRACT | pad + 10 controles de articulación (lengua, mandíbula, redondeo, apertura, nasal, VTL, glide) |
| MIC & CARRIER | 10 controles de micrófono/sinte + 3 selectores de ruteo |
| HARDWARE CHAIN | 8 controles de driver/manguera/micrófono + modelo de micrófono |
| SYSTEM | bypass + lectura del pad |

**44/44 parámetros accesibles. Cero controles huérfanos.** Verificado por
auditoría estática, no a ojo.

---

## 3. Unidades honestas

Antes los parámetros eran todos 0..1 y la UI habría mentido: una perilla
etiquetada "dB" mostrando `0.72`. Ahora los rangos llevan unidades reales
(dB, %, Hz, cm, m, semitonos) y la conversión a ganancia lineal ocurre **una vez
por bloque** en `syncParams()`.

Verificado aritméticamente que el DSP recibe exactamente los mismos valores que
antes del cambio (10/10 conversiones exactas), así que la precisión de la
versión 1.1.0 se conserva intacta.

---

## 4. Los dos bugs de articulación

**1. El pad no articulaba.** `controlTick()` promediaba `padX` con `tongueX` al
50 % fijo, así que cada vocal quedaba clavada a medio camino entre el pad y los
controles de detalle. Ahora **el pad ES la posición** y los controles de detalle
son desplazamientos alrededor de su neutro.

**2. El glide era inútil.** Estaba mapeado a un coeficiente de un polo, lo que
comprimía todo el recorrido útil en el último 5 % del knob. Ahora se mapea a una
constante de tiempo real:

| glide | tiempo al 95 % |
|---|---|
| 0.00 | 2,7 ms (instantáneo) |
| 0.25 | 9,7 ms |
| 0.50 | 32,7 ms (habla natural) |
| 0.75 | 105 ms |
| 1.00 | 313 ms (portamento) |

---

## 5. Los 6 parámetros muertos, ahora audibles

Verificados con un banco de pruebas dedicado, midiendo el cambio real:

| parámetro | efecto medido |
|---|---|
| `mouthSize` | centroide espectral +21 % (desplaza todos los formantes) |
| `synthOctave` | +32 % de centroide |
| `ambience` | +41 % de cola (reflexiones de tubo/mano) |
| `micProximity` | −4 % de centroide (efecto de proximidad + distancia virtual) |
| `tubeCharacter` | armónicos pares del driver |
| `articSmooth` | 2,7 ms → 92 ms de inercia de articulación |

---

## 6. Verificación

- **DSP: 40/40 tests.** Las cifras de formantes son idénticas a 1.1.0
  (error medio 0,5 % F1–F5), así que el trabajo de precisión no se ha tocado.
- **Compilación contra JUCE 7.0.12 real** en este entorno, no solo en CI: eso
  detectó 2 errores antes de publicar (`FontOptions` es de JUCE 8 y
  `getDefaultValue()` es privado en JUCE 7).
- **Layout simulado numéricamente** para las 5 pestañas: 6–7 columnas, 2 filas,
  perillas de ~100 px, nada se sale de su marco.
- **Checksums verificados en un checkout limpio** antes de publicar.
- **Build de Windows verde**, VST3 y Standalone enlazados.

---

## 7. Instalador

**`TalkBoxReference-Setup-1.2.0.exe`** — 3.308.417 B

<https://github.com/ECPCONBANDLAB/Talkbox-Reference/releases/tag/v1.2.0>

Compilado por CI en Windows Server 2022, JUCE 7.0.12, Inno Setup 6.7.1.
GPLv3, sin firmar. VST3 en `{autopf}\Common Files\VST3\`, Standalone en
`{autopf}\TalkBox Reference\`.

---

## 8. Límites de lo afirmado

- **No he visto la UI renderizada.** No hay servidor X ni freetype en este
  entorno y no tengo permisos para instalarlos. He verificado que compila
  contra JUCE 7.0.12 real, que la lógica de layout no desborda (simulada
  numéricamente) y que los 44 parámetros están enlazados, pero **la
  comprobación visual final tiene que hacerla el usuario**. Es posible que
  alguna perilla quede algo justa de espacio o que un texto se solape.
- El error de enlazado de MSVC solo apareció en el build de Windows; el
  syntax-check con GCC de este entorno no lo detectaba porque nunca compilaba
  `VowelPad.cpp`. Ya está corregido con un glob de CMake.
