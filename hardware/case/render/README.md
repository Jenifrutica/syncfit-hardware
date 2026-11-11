# Frame de referencia para las animaciones

`pulsi_ref_green.png` — Pulsi de frente, 1024 × 1024, fondo croma **#00B140**.

Se usa como **primer frame** en Gemini/Veo (image-to-video) para los tres clips:

1. Acercamiento a la **panza** (aro del dedo).
2. Acercamiento a la **base** con una flecha hacia abajo (empujar).
3. Acercamiento al **ojo derecho** (el que en pantalla queda a la izquierda).

Reglas para que los tres coincidan:

- La misma imagen en los tres.
- Que no cambie el personaje ni el fondo verde.
- Push-in suave, sin cortes, 3–4 s.
- Sin manos, sin texto.

Regenerar: `python3 hardware/case/render_green.py`
