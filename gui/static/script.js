document.addEventListener('DOMContentLoaded', () => {
    const editor = document.getElementById('codeEditor');
    const buildBtn = document.getElementById('buildBtn');
    const clearBtn = document.getElementById('clearBtn');
    const scenarioSelect = document.getElementById('scenarioSelect');
    const tabs = document.querySelectorAll('.tab');
    const contents = document.querySelectorAll('.tab-content');
    
    // Tab switching
    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            tabs.forEach(t => t.classList.remove('active'));
            contents.forEach(c => c.classList.remove('active'));
            
            tab.classList.add('active');
            document.getElementById(tab.dataset.target).classList.add('active');
        });
    });

    // Clear button
    clearBtn.addEventListener('click', () => {
        if(confirm("Are you sure you want to clear the editor?")) {
            editor.value = '';
        }
    });

    // Predefined Scenarios
    const scenarios = {
        'blink': `.text
.global _start
_start:
    li t0, 0x10000000   # GPIO LED Adresi
    li t1, 0x01         # LED Durumu
loop:
    sw t1, 0(t0)        # LED'i yak/sondur
    not t1, t1          # Durumu tersine cevir
    li t2, 100000       # Gecikme
delay:
    addi t2, t2, -1
    bnez t2, delay
    j loop`,
        'fibonacci': `.text
.global _start
_start:
    li a0, 0x00010000   # Data base adresi
    li t0, 0            # f(n-2)
    li t1, 1            # f(n-1)
    li t2, 10           # 10 sayi hesapla
    
    sw t0, 0(a0)
    sw t1, 4(a0)
    addi a0, a0, 8
    addi t2, t2, -2
    
loop:
    add t3, t0, t1      # f(n) = f(n-1) + f(n-2)
    sw t3, 0(a0)        # RAM'e yaz
    mv t0, t1
    mv t1, t3
    addi a0, a0, 4
    addi t2, t2, -1
    bnez t2, loop
    ebreak`,
        'knight_rider': `# knight_rider.s - Knight Rider LED Test Program
# PicoRV32 / RV32I
# GPIO adresi: 0x10000000

.text
.global _start

_start:
    # GPIO base adresini t1'e yukle: 0x10000000
    li   t1, 0x10000000

    # Baslangic LED pattern: sadece bit0 = 0x01
    li   t2, 1

knight_loop:
    # === SOLA TARAMA: bit0 -> bit7 ===
    li   t3, 7               # 7 adim kaydirma

left_scan:
    not  t4, t2              # Aktif-low LED'ler için degeri tersle
    sw   t4, 0(t1)           # LED'e yaz (GPIO)
    
    # Gecikme
    li   t5, 100000
delay_left:
    addi t5, t5, -1
    bnez t5, delay_left

    slli t2, t2, 1           # biti sola kaydir
    addi t3, t3, -1          # sayaci azalt
    bnez t3, left_scan       # sifir degilse devam

    # === SAGA TARAMA: bit7 -> bit0 ===
    li   t3, 7               # 7 adim kaydirma

right_scan:
    not  t4, t2              # Aktif-low LED'ler için degeri tersle
    sw   t4, 0(t1)           # LED'e yaz
    
    # Gecikme
    li   t5, 100000
delay_right:
    addi t5, t5, -1
    bnez t5, delay_right

    srli t2, t2, 1           # biti saga kaydir
    addi t3, t3, -1
    bnez t3, right_scan

    j    knight_loop`,
        'uart': `.text
.global _start
_start:
    li t0, 0x20000000   # UART TX Adresi
    la t1, msg
    
print_loop:
    lb t2, 0(t1)
    beqz t2, end
    sw t2, 0(t0)
    addi t1, t1, 1
    j print_loop
    
end:
    ebreak

.data
msg: .string "Hello PicoRV32!\n"`
    };

    scenarioSelect.addEventListener('change', (e) => {
        const val = e.target.value;
        if(val && scenarios[val]) {
            editor.value = scenarios[val];
        }
    });

    // Build functionality
    buildBtn.addEventListener('click', async () => {
        const code = editor.value;
        if(!code.trim()) {
            alert("Editor is empty!");
            return;
        }

        const terminal = document.getElementById('terminal');
        const hex = document.getElementById('hex');
        const mem = document.getElementById('mem');
        const map = document.getElementById('map');

        // Reset UI
        buildBtn.innerHTML = '<i class="fa-solid fa-spinner fa-spin"></i> Building...';
        buildBtn.disabled = true;
        terminal.innerText = 'Building...';
        hex.innerText = '';
        mem.innerText = '';
        map.innerText = '';

        // Switch to terminal tab
        tabs[0].click();

        try {
            const response = await fetch('/build', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ code })
            });
            const result = await response.json();

            terminal.innerText = result.log;
            if(result.success) {
                terminal.innerHTML += '\n<span class="log-success">[SUCCESS] Build completed successfully.</span>';
                hex.innerText = result.hex;
                mem.innerText = result.mem;
                map.innerText = result.map;
            } else {
                terminal.innerHTML += '\n<span class="log-error">[ERROR] Build failed.</span>';
            }
        } catch (err) {
            terminal.innerHTML = `<span class="log-error">Server Error: ${err.message}</span>`;
        } finally {
            buildBtn.innerHTML = '<i class="fa-solid fa-play"></i> Build & Link';
            buildBtn.disabled = false;
        }
    });
});
