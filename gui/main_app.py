import threading
import time
import webbrowser
import os
import subprocess
import socket
from app import app

def is_port_in_use(port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        return s.connect_ex(('localhost', port)) == 0

def start_flask():
    app.run(port=5000, debug=False, threaded=True)

def launch_browser():
    url = "http://localhost:5000"
    
    # Wait for server to be ready
    for _ in range(10):
        if is_port_in_use(5000):
            break
        time.sleep(0.5)
    
    # Try to launch in "App Mode" (Chromium/Chrome)
    # This removes the browser UI (tabs, address bar) making it feel like an app
    app_modes = [
        ["google-chrome", "--app=" + url],
        ["chromium", "--app=" + url],
        ["chromium-browser", "--app=" + url],
        ["microsoft-edge", "--app=" + url]
    ]
    
    success = False
    for cmd in app_modes:
        try:
            # Check if command exists
            subprocess.Popen(["which", cmd[0]], stdout=subprocess.PIPE).wait()
            subprocess.Popen(cmd)
            success = True
            break
        except:
            continue
            
    if not success:
        # Fallback to default browser
        webbrowser.open(url)

if __name__ == '__main__':
    print("PicoRV32 Studio başlatılıyor...")
    
    # Start Flask in a background thread
    t = threading.Thread(target=start_flask)
    t.daemon = True
    t.start()

    # Launch browser window
    launch_browser()
    
    # Keep the main thread alive as long as the server is needed
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nKapatılıyor...")
