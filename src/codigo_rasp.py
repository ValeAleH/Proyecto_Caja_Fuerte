from flask import Flask, render_template_string, jsonify, request
import serial
import threading
from datetime import datetime

app = Flask(__name__)

try:
    uart = serial.Serial('/dev/ttyS0', 9600, timeout=1)
except:
    uart = None

estado_actual = "CERRADO"
intentos_web = 0
historial = []
password_web = "1234"

def leer_uart():
    global estado_actual
    while True:
        try:
            if uart and uart.in_waiting:
                linea = uart.readline().decode('utf-8').strip()
                if linea in ["ABIERTO", "CERRADO", "BLOQUEADO", "DESBLOQUEADO", "PASS_OK"]:
                    estado_actual = linea
                    hora = datetime.now().strftime("%H:%M:%S")
                    historial.insert(0, {"hora": hora, "evento": linea})
                    if len(historial) > 10:
                        historial.pop()
        except:
            pass

hilo = threading.Thread(target=leer_uart, daemon=True)
hilo.start()

HTML = '''
<!DOCTYPE html>
<html>
<head>
    <title>Caja Fuerte - Panel de Control</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body { font-family: Arial, sans-serif; background: #0f0f1a; color: #fff; min-height: 100vh; }
        .header { background: #1a1a2e; padding: 20px; text-align: center; border-bottom: 2px solid #e94560; }
        .header h1 { color: #e94560; font-size: 28px; }
        .header p { color: #888; font-size: 14px; margin-top: 5px; }
        .container { max-width: 900px; margin: 0 auto; padding: 20px; }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-top: 20px; }
        .card { background: #1a1a2e; border-radius: 12px; padding: 20px; border: 1px solid #2a2a4a; }
        .card h2 { font-size: 16px; color: #aaa; margin-bottom: 15px; text-transform: uppercase; letter-spacing: 1px; }
        .estado-badge { display: inline-block; padding: 10px 20px; border-radius: 8px; font-size: 18px; font-weight: bold; margin: 10px 0; }
        .CERRADO { background: #1e3a5f; color: #7ec8f7; }
        .ABIERTO { background: #1a3d1a; color: #7fff7f; }
        .BLOQUEADO { background: #3d1a1a; color: #ff7f7f; }
        .DESBLOQUEADO { background: #1a3d1a; color: #7fff7f; }
        .PASS_OK { background: #3d3d1a; color: #ffff7f; }
        input[type=password], input[type=text] { width: 100%; padding: 10px; background: #0f0f1a; border: 1px solid #3a3a5a; border-radius: 8px; color: #fff; font-size: 14px; margin: 8px 0; }
        input:focus { outline: none; border-color: #e94560; }
        .btn { width: 100%; padding: 12px; border: none; border-radius: 8px; font-size: 15px; cursor: pointer; margin-top: 8px; font-weight: bold; }
        .btn-red { background: #e94560; color: white; }
        .btn-red:hover { background: #c73652; }
        .btn-green { background: #0f9b58; color: white; }
        .btn-green:hover { background: #0a7a45; }
        .btn-blue { background: #185fa5; color: white; }
        .btn-blue:hover { background: #0d4a80; }
        .intentos { display: flex; gap: 8px; margin: 10px 0; }
        .intento-dot { width: 20px; height: 20px; border-radius: 50%; background: #333; }
        .intento-dot.usado { background: #e94560; }
        .historial-item { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #2a2a4a; font-size: 13px; }
        .historial-item:last-child { border-bottom: none; }
        .hora { color: #888; }
        .evento-ABIERTO { color: #7fff7f; }
        .evento-CERRADO { color: #7ec8f7; }
        .evento-BLOQUEADO { color: #ff7f7f; }
        .evento-DESBLOQUEADO { color: #7fff7f; }
        .evento-PASS_OK { color: #ffff7f; }
        .msg { padding: 10px; border-radius: 8px; margin-top: 10px; font-size: 13px; text-align: center; display: none; }
        .msg-ok { background: #1a3d1a; color: #7fff7f; }
        .msg-err { background: #3d1a1a; color: #ff7f7f; }
        @media(max-width: 600px) { .grid { grid-template-columns: 1fr; } }
    </style>
</head>
<body>
    <div class="header">
        <h1>🔐 Caja Fuerte — Panel de Control</h1>
        <p>Sistema de seguridad inteligente</p>
    </div>
    <div class="container">
        <div class="grid">
            <div class="card">
                <h2>Estado del sistema</h2>
                <div id="estado-badge" class="estado-badge CERRADO">🔒 CERRADO</div>
                <p style="color:#888; font-size:13px; margin-top:10px;">Intentos fallidos:</p>
                <div class="intentos">
                    <div class="intento-dot" id="dot1"></div>
                    <div class="intento-dot" id="dot2"></div>
                    <div class="intento-dot" id="dot3"></div>
                </div>
            </div>

            <div class="card">
                <h2>Abrir caja</h2>
                <p style="color:#888; font-size:13px; margin-bottom:5px;">Ingresa la contraseña para abrir:</p>
                <input type="password" id="pass_abrir" placeholder="Contraseña" maxlength="8">
                <button class="btn btn-red" onclick="abrir()">🔓 Abrir caja</button>
                <div class="msg" id="msg_abrir"></div>
            </div>

            <div class="card">
                <h2>Cambiar contraseña</h2>
                <input type="password" id="pass_actual" placeholder="Contraseña actual" maxlength="8">
                <input type="password" id="pass_nueva" placeholder="Nueva contraseña" maxlength="8">
                <input type="password" id="pass_confirmar" placeholder="Confirmar nueva contraseña" maxlength="8">
                <button class="btn btn-green" onclick="cambiarPass()">🔑 Cambiar contraseña</button>
                <div class="msg" id="msg_pass"></div>
            </div>

            <div class="card">
                <h2>Historial de eventos</h2>
                <div id="historial">
                    <p style="color:#888; font-size:13px;">Sin eventos aún...</p>
                </div>
            </div>
        </div>
    </div>

    <script>
        let intentosWeb = 0;

        function mostrarMsg(id, texto, ok) {
            let el = document.getElementById(id);
            el.textContent = texto;
            el.className = 'msg ' + (ok ? 'msg-ok' : 'msg-err');
            el.style.display = 'block';
            setTimeout(() => el.style.display = 'none', 3000);
        }

        function abrir() {
            let pass = document.getElementById('pass_abrir').value;
            if (!pass) { mostrarMsg('msg_abrir', 'Ingresa la contraseña', false); return; }
            fetch('/abrir', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({password: pass})
            }).then(r => r.json()).then(d => {
                mostrarMsg('msg_abrir', d.mensaje, d.ok);
                document.getElementById('pass_abrir').value = '';
                if (!d.ok) {
                    intentosWeb = Math.min(intentosWeb + 1, 3);
                    actualizarDots();
                } else {
                    intentosWeb = 0;
                    actualizarDots();
                }
            });
        }

        function cambiarPass() {
            let actual = document.getElementById('pass_actual').value;
            let nueva = document.getElementById('pass_nueva').value;
            let confirmar = document.getElementById('pass_confirmar').value;
            if (!actual || !nueva || !confirmar) { mostrarMsg('msg_pass', 'Completa todos los campos', false); return; }
            if (nueva !== confirmar) { mostrarMsg('msg_pass', 'Las contraseñas no coinciden', false); return; }
            if (nueva.length < 4) { mostrarMsg('msg_pass', 'Minimo 4 caracteres', false); return; }
            fetch('/cambiar_pass', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({actual: actual, nueva: nueva})
            }).then(r => r.json()).then(d => {
                mostrarMsg('msg_pass', d.mensaje, d.ok);
                if (d.ok) {
                    document.getElementById('pass_actual').value = '';
                    document.getElementById('pass_nueva').value = '';
                    document.getElementById('pass_confirmar').value = '';
                }
            });
        }

        function actualizarDots() {
            for (let i = 1; i <= 3; i++) {
                let dot = document.getElementById('dot' + i);
                dot.className = 'intento-dot' + (i <= intentosWeb ? ' usado' : '');
            }
        }

        function actualizarEstado() {
            fetch('/estado').then(r => r.json()).then(d => {
                let badge = document.getElementById('estado-badge');
                let iconos = {
                    'ABIERTO': '🔓 ABIERTO',
                    'CERRADO': '🔒 CERRADO',
                    'BLOQUEADO': '🚫 BLOQUEADO',
                    'DESBLOQUEADO': '✅ DESBLOQUEADO',
                    'PASS_OK': '✅ CONTRASEÑA CAMBIADA'
                };
                badge.textContent = iconos[d.estado] || d.estado;
                badge.className = 'estado-badge ' + d.estado;

                if (d.estado === 'BLOQUEADO') intentosWeb = 3;
                else if (d.estado === 'DESBLOQUEADO') { intentosWeb = 0; actualizarDots(); }

                if (d.historial.length > 0) {
                    let html = d.historial.map(h =>
                        '<div class="historial-item"><span class="hora">' + h.hora + '</span><span class="evento-' + h.evento + '">' + h.evento + '</span></div>'
                    ).join('');
                    document.getElementById('historial').innerHTML = html;
                }
            });
        }

        setInterval(actualizarEstado, 1000);
    </script>
</body>
</html>
'''

@app.route('/')
def index():
    return render_template_string(HTML)

@app.route('/estado')
def estado():
    return jsonify({'estado': estado_actual, 'historial': historial})

@app.route('/abrir', methods=['POST'])
def abrir():
    global intentos_web, password_web
    data = request.get_json()
    passwd = data.get('password', '')
    if passwd == password_web:
        intentos_web = 0
        if uart:
            uart.write(b'OPEN\n')
        return jsonify({'ok': True, 'mensaje': 'Abriendo caja...'})
    else:
        intentos_web += 1
        restantes = 3 - intentos_web
        if intentos_web >= 3:
            intentos_web = 0
            return jsonify({'ok': False, 'mensaje': 'Demasiados intentos fallidos'})
        return jsonify({'ok': False, 'mensaje': f'Contraseña incorrecta. Intentos restantes: {restantes}'})

@app.route('/cambiar_pass', methods=['POST'])
def cambiar_pass():
    global password_web
    data = request.get_json()
    actual = data.get('actual', '')
    nueva = data.get('nueva', '')
    if actual != password_web:
        return jsonify({'ok': False, 'mensaje': 'Contraseña actual incorrecta'})
    if len(nueva) < 4:
        return jsonify({'ok': False, 'mensaje': 'La nueva contraseña es muy corta'})
    password_web = nueva
    if uart:
        uart.write(f'PASS:{nueva}\n'.encode())
    return jsonify({'ok': True, 'mensaje': 'Contraseña cambiada exitosamente'})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=False)