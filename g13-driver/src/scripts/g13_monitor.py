#!/usr/bin/env python3
import time
import psutil
import os
import sys
from datetime import datetime

# Path to the Named Pipe (Must match the path in the C++ driver)
PIPE_PATH = "/tmp/g13-lcd"

def create_bar(percent, length=10):
    """Creates a simple ASCII loading bar."""
    filled_length = int(length * percent // 100)
    bar = 'X' * filled_length + '-' * (length - filled_length)
    return f"[{bar}]"

def format_bytes(size):
    """Converts bytes to GB/MB."""
    power = 2**30 # 1024**3
    n = size / power
    return f"{n:.1f}GB"

def write_to_pipe(message):
    """Writes the string to the pipe."""
    if not os.path.exists(PIPE_PATH):
        print(f"Error: Pipe {PIPE_PATH} not found. Is the driver running?")
        return

    try:
        # We reopen the pipe on every write.
        # This is safe for Named Pipes on Linux and prevents deadlocks.
        with open(PIPE_PATH, 'w') as pipe:
            pipe.write(message)
    except OSError as e:
        print(f"Error writing to pipe: {e}")
    except BrokenPipeError:
        print("Driver closed the connection.")

def main():
    print("G13 Monitor started. Press Ctrl+C to exit.")
    
    while True:
        try:
            # 1. Collect data
            now = datetime.now().strftime("%H:%M:%S")
            cpu_percent = psutil.cpu_percent(interval=None)
            ram = psutil.virtual_memory()
            
            # 2. Create Layout (Max 4-5 lines)
            # Line 1: Time
            line1 = f"TIME: {now}"
            
            # Line 2: CPU with bar
            line2 = f"CPU: {int(cpu_percent):>3}% {create_bar(cpu_percent, 10)}"
            
            # Line 3: RAM Usage (Used / Total)
            line3 = f"RAM: {format_bytes(ram.used)}/{format_bytes(ram.total)} ({ram.percent}%)"
            
            # Line 4: System Info
            line4 = "System: Arch Linux"

            # 3. Assemble message
            final_msg = f"{line1}\n{line2}\n{line3}\n{line4}"
            
            # 4. Send
            write_to_pipe(final_msg)
            
            # Refresh rate (1 second is good for LCDs)
            time.sleep(1)

        except KeyboardInterrupt:
            print("\nStopping monitor...")
            break
        except Exception as e:
            print(f"Unexpected error: {e}")
            time.sleep(5) # Wait before retry

if __name__ == "__main__":
    main()