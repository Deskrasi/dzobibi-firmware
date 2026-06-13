#!/usr/bin/env python3
"""
Dzobibi NVS provisioning tool.
Compiles + flashes the provisioning sketch, prompts for credentials
over a secure TTY (passwords hidden), writes them to NVS, then
re-compiles + re-flashes the main firmware automatically.
"""
import subprocess, sys, time, getpass, os, glob

FQBN       = "esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc"
REPO       = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROVISION  = os.path.join(REPO, "tools", "provision")
MAIN       = os.path.join(REPO, "dzobibi")

# ── helpers ──────────────────────────────────────────────────────────────────

def run(cmd, **kw):
    print(f"  $ {' '.join(cmd)}")
    r = subprocess.run(cmd, **kw)
    if r.returncode != 0:
        print(f"  FAILED (exit {r.returncode})")
        sys.exit(1)
    return r

def find_port():
    import serial.tools.list_ports
    ports = [p.device for p in serial.tools.list_ports.comports()
             if "usbmodem" in p.device or "ttyUSB" in p.device or "ttyACM" in p.device]
    if not ports:
        sys.exit("No ESP32 USB port found. Plug in the board and retry.")
    if len(ports) == 1:
        return ports[0]
    print("Multiple ports detected:")
    for i, p in enumerate(ports):
        print(f"  [{i}] {p}")
    idx = int(input("Select port: "))
    return ports[idx]

def prompt(label, secret=False, default=None, env_key=None):
    # Check env var first (allows non-interactive use)
    if env_key and env_key in os.environ:
        val = os.environ[env_key]
        masked = "***" if secret else (val if val else "(empty)")
        print(f"  {label}: {masked}  (from env)")
        return val if val else (default or "")
    suffix = f" [{default}]" if default else ""
    prompt_str = f"  {label}{suffix}: "
    if secret:
        val = getpass.getpass(prompt_str)
    else:
        val = input(prompt_str).strip()
    if not val and default:
        return default
    return val

# ── gather credentials ────────────────────────────────────────────────────────

print("\n─── Dzobibi NVS Provisioning ────────────────────────────────")
print("Credentials are sent directly to the board and never written to disk.\n")

fields = {}
print("MQTT broker (HiveMQ Cloud):")
fields["mqtt_broker"] = prompt("  Broker hostname", env_key="DZ_MQTT_BROKER")
fields["mqtt_port"]   = prompt("  Port", default="8883", env_key="DZ_MQTT_PORT")
fields["mqtt_user"]   = prompt("  Username", env_key="DZ_MQTT_USER")
fields["mqtt_pass"]   = prompt("  Password", secret=True, env_key="DZ_MQTT_PASS")

print("\nCellular (SIM7600):")
fields["gsm_apn"]  = prompt("  APN", default="internet", env_key="DZ_GSM_APN")
fields["gsm_user"] = prompt("  APN username (blank if none)", default="", env_key="DZ_GSM_USER")
fields["gsm_pass"] = prompt("  APN password (blank if none)", default="", env_key="DZ_GSM_PASS")

print("\nWiFi fallback (leave blank to skip):")
fields["wifi_ssid"] = prompt("  SSID", default="", env_key="DZ_WIFI_SSID")
fields["wifi_pass"] = prompt("  Password", secret=True, env_key="DZ_WIFI_PASS") if fields["wifi_ssid"] else ""

print("\nDevice:")
fields["device_name"] = prompt("  Device name", default="Dzobibi", env_key="DZ_DEVICE_NAME")

# ── detect port ───────────────────────────────────────────────────────────────

port = find_port()
print(f"\nUsing port: {port}")

# ── compile + flash provisioning sketch ───────────────────────────────────────

print("\n[1/4] Compiling provisioning sketch...")
run(["arduino-cli", "compile", "--fqbn", FQBN, PROVISION])

print("\n[2/4] Flashing provisioning sketch...")
run(["arduino-cli", "upload", "--fqbn", FQBN, "--port", port, PROVISION])

# ── write credentials to NVS over serial ─────────────────────────────────────

print("\n[3/4] Writing credentials to NVS...")
import serial as pyserial

# Wait for USB CDC to re-enumerate after flash reset
print("  Waiting for board to boot...")
time.sleep(5)

ser = pyserial.Serial(port, 115200, timeout=3)
time.sleep(0.5)

def expect(ser, token, tries=20):
    for _ in range(tries):
        line = ser.readline().decode(errors="replace").strip()
        if line:
            print(f"    board: {line}")
        if token in line:
            return True
    return False

if not expect(ser, "PROVISION_READY"):
    sys.exit("Board did not send PROVISION_READY — try pressing the reset button.")

errors = []
for key, val in fields.items():
    if val == "":
        continue
    cmd = f"{key}={val}\n".encode()
    ser.write(cmd)
    time.sleep(0.1)
    resp = ser.readline().decode(errors="replace").strip()
    print(f"    board: {resp}")
    if not resp.startswith("OK"):
        errors.append(f"{key}: {resp}")

ser.write(b"DUMP\n")
time.sleep(0.3)
while ser.in_waiting:
    print(f"    board: {ser.readline().decode(errors='replace').strip()}")

ser.write(b"COMMIT\n")
time.sleep(0.1)
expect(ser, "PROVISION_DONE")
ser.close()

if errors:
    print(f"\nWARNING — some fields failed: {errors}")
else:
    print("\n  NVS provisioned successfully.")

# ── re-flash main firmware ────────────────────────────────────────────────────

print("\n[4/4] Re-flashing main firmware...")
run(["arduino-cli", "compile", "--fqbn", FQBN, MAIN])
run(["arduino-cli", "upload",  "--fqbn", FQBN, "--port", port, MAIN])

print("\n─── Done. Board is running main firmware with provisioned credentials. ───\n")
