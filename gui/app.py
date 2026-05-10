import os
import subprocess
from flask import Flask, render_template, request, jsonify

app = Flask(__name__, 
            static_folder='static',
            template_folder='templates')

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
OUTPUT_DIR = os.path.join(PROJECT_ROOT, 'output')
GUI_DIR = os.path.join(PROJECT_ROOT, 'gui')

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/build', methods=['POST'])
def build():
    data = request.get_json(silent=True) or {}
    code = data.get('code', '')
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    main_s = os.path.join(GUI_DIR, 'temp_main.s')
    with open(main_s, 'w', encoding='utf-8') as f:
        f.write(code)
    
    # Assembler
    asm_cmd = ["./assembler_bin", "gui/temp_main.s"]
    res_asm = subprocess.run(asm_cmd, cwd=PROJECT_ROOT, capture_output=True, text=True)
    log = res_asm.stdout + res_asm.stderr
    
    if res_asm.returncode != 0:
        return jsonify({'success': False, 'log': log})
    
    # Linker
    lnk_cmd = [
        "./linker_bin", 
        "gui/temp_main.o", 
        "-o", "output/temp_out", 
        "--text-base", "00000000", 
        "--data-base", "00010000", 
        "--stack-top", "00020000"
    ]
    res_lnk = subprocess.run(lnk_cmd, cwd=PROJECT_ROOT, capture_output=True, text=True)
    log += "\n" + res_lnk.stdout + res_lnk.stderr
    
    if res_lnk.returncode != 0:
        return jsonify({'success': False, 'log': log})
        
    # Read outputs
    hex_data, mem_data, map_data = "", "", ""
    try:
        with open(os.path.join(OUTPUT_DIR, 'temp_out.hex'), 'r', encoding='utf-8') as f: hex_data = f.read()
        with open(os.path.join(OUTPUT_DIR, 'temp_out.mem'), 'r', encoding='utf-8') as f: mem_data = f.read()
        with open(os.path.join(OUTPUT_DIR, 'temp_out.map'), 'r', encoding='utf-8') as f: map_data = f.read()
    except Exception as e:
        log += f"\n[HATA] Dosyalar okunamadi: {e}"
        
    return jsonify({
        'success': True,
        'log': log,
        'hex': hex_data,
        'mem': mem_data,
        'map': map_data
    })

def run_server():
    app.run(port=5000)

if __name__ == '__main__':
    app.run(port=5000, debug=os.environ.get('FLASK_DEBUG') == '1')
