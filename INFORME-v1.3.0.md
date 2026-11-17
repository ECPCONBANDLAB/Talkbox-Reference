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

## Lo que NO puedo afirmar

- **No he compilado el binario ni he visto la GUI.** En este entorno no hay
  X11, ni freetype, ni permisos de root. Lo que verifiqué es que compila sin
  errores contra JUCE 7.0.12 real, y calculé la geometría por script. Un fallo
  de enlazado o visual solo aparecerá en el CI de Windows.
- **No he podido probarlo en FL Studio.** Simulé los tres caminos de señal,
  pero un host real puede comportarse distinto.
- **El splash y la licencia.** El badge aparece por defecto en JUCE 7; para
  desactivarlo hay que compilar con esa macro a 0. Bajo GPLv3 se distribuye el
  código fuente completo, y eso es lo que veníamos haciendo, así que la
  posición es defendible — pero no es un "no coste": es una decisión de
  licencia. Si prefieres no arriesgar, se quita la línea del CMake y vuelve el
  badge.
- Los hashes los regeneré y validé en árbol limpio (49/49). Eso ha roto el
  Release tres veces; esta vez está comprobado antes de etiquetar.

## Pendiente

No he etiquetado `v1.3.0` ni he lanzado el CI todavía. Dime si quieres que lo
haga, y si el splash se queda fuera o lo devuelvo.
