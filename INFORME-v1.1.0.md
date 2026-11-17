# TalkBox Reference 1.1.0 — informe de precisión, tamaño y rendimiento

Fecha: 2026-11-17 · Rama: `arena/49a449cd-talkbox-reference` · Tag: `v1.1.0`

---

## 1. Resultado en una línea

El modelo de vocales pasa de **3,8 % a 0,5 % de error medio** (F1–F5), usando
**33,3 % menos memoria de tabla** y con **el mismo rendimiento de CPU**.

---

## 2. Los porcentajes frente a la versión anterior

| Métrica | v1.0.0 | v1.1.0 | Variación |
|---|---|---|---|
| Error medio F1–F5, anclas | 3,8 % | **0,5 %** | **−87 %** |
| Peor formante de ancla | 45,1 % | **3,3 %** | **−93 %** |
| Error de interpolación (16 puntos) | 3,4 % | **1,4 %** | **−59 %** |
| Tamaño de tabla | 432 B | **288 B** | **−33,3 %** |
| RAM de instancia | 25 472 B | 25 472 B | **0 %** |
| CPU Balanced+Mic (mediana de 5) | 24,08 µs | 24,04 µs | **−0,2 %** (ruido) |

### Desglose por formante (error medio en anclas, %)

| | F1 | F2 | F3 | F4 | F5 |
|---|---|---|---|---|---|
| v1.0.0 | 4,2 | 4,7 | 2,4 | 4,0 | 3,5 |
| **v1.1.0** | **1,3** | **0,4** | **0,2** | **0,2** | **0,5** |
| Mejora | −69 % | −91 % | −92 % | −95 % | −86 % |

---

## 3. Qué estaba mal en la medición anterior (importante)

Los "40/40 tests, error <2 %" de v1.0.0 **no eran una medida válida**. Tres
defectos, todos corregidos:

1. **Circularidad.** El test comparaba la salida del tracto contra `kVowels`,
   los mismos números que configuraban el tracto. Una tabla mal afinada da 0 %
   de error por construcción. De ahí que pasara 40/40 mientras F4/F5 estaban
   escritos a mano como 3500/4500 Hz para todas las vocales.

2. **Selección de picos por magnitud.** En vocales posteriores los cinco máximos
   más fuertes son todos estructura glotal de baja frecuencia. Barriendo `/u/`:
   `188 293 397 762 867 Hz`. F3 está en 2240 Hz y nunca se alcanzaba. Por eso
   aparecía un error del ~70 % en F3 que hablaba del instrumento, no del modelo.

3. **Sin referencia acústica.** F4/F5 se comparaban contra constantes, no contra
   ningún dato publicado.

Ahora el test usa la tabla de **Peterson & Barney (1952)**, varón adulto, como
patrón independiente, y selecciona picos con **restricción de orden** (F1 en
180–1100 Hz, cada formante al menos 150 Hz por encima del anterior) validando
cada pico por su **prominencia**, no por su altura.

---

## 4. El cambio de modelo

La tabla guarda ahora **cinco formantes y cinco anchos de banda** por vocal, en
`int16` a 1 Hz, frente a los tres formantes en `float` de antes:

    struct VowelAnchor {      // 32 B  (antes 48 B)
        const char* name;
        std::int16_t f1,f2,f3,f4,f5;
        std::int16_t b1,b2,b3,b4,b5;
    };

Los valores almacenados son **parámetros de síntesis**, no medidas acústicas.
La cascada de 5 resonadores no deja pasar cada polo intacto: los resonadores
vecinos desplazan los picos realizados, la inclinación de −6 dB/oct del origen
mueve los máximos aparentes y las etapas paso-bajo atenúan las resonancias altas.
Por eso los valores se obtuvieron por **descenso por coordenadas sobre la
cascada física**, exigiendo que el tracto *realice* los formantes publicados.

Esto no es ajuste a una medición defectuosa: el objetivo es la frecuencia de pico
realizada por el filtro real.

---

## 5. Honestidad sobre el rendimiento

**La CPU no mejora.** La mediana de 5 ejecuciones es 24,08 → 24,04 µs, una
diferencia del 0,2 % que está dentro del ruido: ejecuciones del mismo binario
oscilan entre 20,0 y 28,0 µs. No reclamo una mejora de velocidad.

Lo que sí es sólido: **no hay regresión**. El trabajo por muestra es idéntico
(la tabla solo se lee a ritmo de control y los `int16` se convierten a `float`
una vez por tick, igual que antes se leían `float`). La memoria de instancia es
exactamente la misma, `sizeof(TalkboxEngine) = 25472 B`, verificado en ambos
binarios.

La reducción real y verificable es de **tabla**: 432 B → 288 B (−33,3 %).

---

## 6. Verificación

- **Tests: 40/40 PASS**, ahora midiendo F1–F5 de verdad. Peor formante
  end-to-end (con driver y micrófono incluidos): **11,5 %**.
- **Interpolación:** 16 posiciones interiores del pad, contra la mezcla IDW de
  los objetivos publicados: **1,4 %**.
- **Reproducible:** las tres herramientas están en `benchmark/vowel-model/` y
  reproducen 0,5 % y 1,4 % sobre el árbol publicado.

## 7. Instalador

**`TalkBoxReference-Setup-1.1.0.exe`** — 3 301 093 B, publicado en
<https://github.com/ECPCONBANDLAB/Talkbox-Reference/releases/tag/v1.1.0>

Construido por CI en Windows Server 2022, Inno Setup 6.7.1, JUCE 7.0.12.
GPLv3, sin firmar. Instala VST3 en `{autopf}\Common Files\VST3\` y el
Standalone en `{autopf}\TalkBox Reference\`.

## 8. Límites de lo afirmado

- Los objetivos de referencia son medias de varón adulto (P&B 1952). Para voces
  femeninas o infantiles el escalado es por `vtl`, no por tabla.
- El error de 11,5 % end-to-end incluye driver, guía de onda y micrófono; es el
  peor caso real del producto, no una medida aislada del tracto.
- La CPU se midió en un solo núcleo de este sandbox. En otra máquina los
  valores absolutos cambian; lo que se sostiene es la ausencia de regresión.
