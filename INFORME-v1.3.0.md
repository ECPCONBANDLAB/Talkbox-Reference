# TalkBox Reference v1.3.0 — informe

## Lo que estaba mal (y no era cosmético)

Investigué el silencio antes de tocar nada, porque era tu queja número uno.
La cadena de audio estaba **sana**: a −12 dBFS de entrada salía −15.8 dBFS,
con respuesta lineal hasta −60 dBFS, y la puerta de micrófono nunca cortaba
nada. El problema no era el DSP.

El problema era **el valor por defecto**: el plugin arrancaba en
*sintetizador interno*, que solo suena cuando llegan notas MIDI. En un insert
de efecto de FL Studio no llega ninguna, así que el plugin se quedaba mudo sin
explicar por qué. Y el bus de micrófono estaba declarado como **obligatorio**:
si FL Studio no lo enruta, el host silencia el efecto entero.

## Las siete cosas que pediste

**1 y 2. Organización y menos perillas.**
De 44 perillas visibles a **6 macros + salida**. Cada macro mueve un grupo
con pesos (por ejemplo *Drive* mueve saturación, carácter de válvula y
compresor juntos). Comprobado por script: **45/45 parámetros siguen
accesibles** — 6 macros en PLAY y los 38 restantes en ADVANCED. No se perdió
ninguna función, solo dejaron de estar todo a la vista a la vez.

Renuncié a la macro "Vowel" que había puesto al principio: movía X e Y del
pad a la vez, lo que duplica el pad sin elegir nunca una vocal. La sustituí
por *Air* (nasal + corte de bocina + proximidad), que sí llega a parámetros
que estaban enterrados.

**3. Presets que no sonaban distintos.**
Confirmado y era peor de lo que parecía: `CMakeLists.txt` lo decía por escrito —
"nothing reads this data yet". Los 5 presets de fábrica eran JSON incrustado
que **nunca se leía**. No es que sonaran parecido: no se cargaba ninguno.

Los sustituí por un generador. Medido sobre 200 tiradas: cubre **las 9 zonas**
del espacio vocal, X de 0.10 a 0.92, y cuatro vocales medidas con el mismo
acorde dan una separación espectral mínima de **0.59** (muy por encima del
umbral de 0.35 que uso para "se distinguen").

**4. Instalador.**
Ahora eliges: *todo*, *solo VST3*, *solo Standalone*, o selección manual.

**5. Un preset por defecto + dado.**
Botón DICE en la barra. El dado no sortea el pad al azar (eso agrupa las
vocales en la zona frontal y produce ocho parches que suenan igual — tu misma
queja): elige una de las 9 vocales reales, le añade jitter, y a veces la
mezcla con otra. Luego deriva el resto del parche de forma coherente.

**6. El talkbox interno.**
Ya no es el camino por defecto. Pero no lo borré: lo hice funcionar. Un acorde
estático a través de un filtro estático es ininteligible, así que ahora
**arpegia el acorde** y reexcita la portadora en cada paso — eso es literalmente
lo que hace inteligible una talkbox sin micrófono. Medido: 132 → 152 → 180 →
188 Hz sobre un Do-Mi-Sol.

**7. Notas en FL Studio.**
Tres escenarios probados con el sidechain **desconectado**, que es el caso real:
- Efecto por defecto, audio en la pista → **suena** (−15.7 dBFS)
- Synth interno + acorde → **suena** (−10.8 dBFS)
- Modo micrófono sin micrófono → **no muere** (−17.4 dBFS, mantiene el pad)

Y si vuelve a haber silencio, la GUI lo dice en pantalla en vez de dejarte
mirando un plugin mudo.

**8. El splash de JUCE.**
`JUCE_DISPLAY_SPLASH_SCREEN=0` en el CMake. Verificado en el código de
JUCE 7.0.12: el flag existe y por defecto vale 1.

## Dos bugs que encontré por el camino

**Las etiquetas del pad estaban mal, y el motivo era grave.** La imagen usaba
una tabla copiada a mano ("i" a 270/2290) distinta de la que usa el audio
("ee" a 300/2240). Las etiquetas señalaban sitios donde el sonido no estaba.
Ahora leen de la misma tabla que el motor: **error 0.0% en las 9 vocales**.

**Solo se dibujaban 2 de las 5 pestañas.** El código dibujaba las 5 pero no
tenía forma de llegar a las otras tres sin botones. Añadí el botón ADVANCED,
que es lo que convierte el rediseño en "menos perillas" en lugar de
"funciones perdidas".

## Restricciones que respeté

- **Memoria**: +80 B en el sintetizador (268 B en total). Cero asignaciones en
  el camino de audio. `sizeof(TalkboxEngine)` no crece de forma medible.
- **CPU**: el arpegiador es monofónico, así que el coste es plano —
  28.2 µs/bloque con 1 nota, 27.7 con 3, 29.1 con 8. Idle: 12.98 µs (0.49%).
- **Tests**: 40/40 PASS.
- **Layout**: verificado a 920×640, 1000×720 y 1600×1100. Ninguna pestaña
  desborda.

## Lo que falló al publicar (y por qué fue útil)

El primer intento de v1.3.0 **rompió el build de Windows**: `error C3615`,
`std::pow` no es una expresión constante en la biblioteca estándar de MSVC.
Mi comprobación local con g++ lo aceptó porque libstdc++ sí lo trata como
constexpr. Es exactamente el hueco que avisé que no podía ver.

Lo arreglé (quitar `constexpr` de `midiToHz`, que solo corre a frecuencia de
audio) y audité por script **todas** las funciones `constexpr` del código
buscando más casos: no hay ninguna otra.

Y esta vez no volví a publicar a ciegas: lancé el workflow de Windows
**contra la rama, sin publicar release**, esperé a que compilara VST3 +
Standalone + instalador con MSVC, y solo entonces fusioné y etiqueté.

## Lo que NO puedo afirmar

- **No he visto la GUI funcionando.** El binario compila y se empaqueta, pero
  en este entorno no hay X11 ni freetype para abrir la ventana. La geometría la
  calculé por script a tres tamaños; el aspecto real solo lo verás tú.
- **No he podido probarlo en FL Studio.** Simulé los tres caminos de señal,
  pero un host real puede comportarse distinto. Esta es la prueba que falta y
  la que harás tú.
- **El splash y la licencia.** El badge aparece por defecto en JUCE 7; para
  desactivarlo hay que compilar con esa macro a 0. Bajo GPLv3 se distribuye el
  código fuente completo, y eso es lo que veníamos haciendo, así que la
  posición es defendible — pero no es un "no coste": es una decisión de
  licencia. Si prefieres no arriesgar, se quita la línea del CMake y vuelve el
  badge.
- Los hashes los regeneré y validé en árbol limpio (49/49). Eso ha roto el
  Release tres veces; esta vez está comprobado antes de etiquetar.

## Publicado

- Tag `v1.3.0` sobre `247bd93` (commit verificado con MSVC antes de etiquetar).
- Release con `TalkBoxReference-Setup-1.3.0.exe` — **3,313,660 B**.
- Windows installer: success. Release: success. CI en main: success.
- Tests en el release: **40/40 PASS**.
- PRs #8 y #9 fusionadas.

Splash desactivado, como pediste.

## Lo que quiero saber de tu prueba en FL Studio

Lo que no puedo simular:

1. Al insertarlo en una pista con audio, ¿**suena a la primera** sin tocar nada?
2. Los **6 macros**, ¿se sienten como grupos coherentes o como perillas que
   hacen cosas raras a la vez?
3. El **dado**, ¿da parches de verdad distintos entre sí al pulsarlo varias
   veces?
4. Las **etiquetas del pad**, ¿caen ahora donde suena cada vocal?
5. El **instalador**, ¿te dejó elegir solo VST3?
6. ¿Sigue apareciendo el badge "made with JUCE"?

Si algo suena mal, dime qué y con qué ajustes, y lo mido.
