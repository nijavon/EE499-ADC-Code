import cv2
import numpy as np
from pyzbar.pyzbar import decode
import tkinter as tk
import pygame  # For playing audio
import time  # For managing cooldown
import serial  # For serial communication with Arduino

# Initialize the video capture from the default camera (index 0)
cap = cv2.VideoCapture(0)
cap.set(3, 640)  # Set width
cap.set(4, 360)  # Set height

# Initialize pygame mixer for audio
pygame.mixer.init()

# Load the sound file using the relative path
beep_sound = pygame.mixer.Sound("/home/team1/xfx/beep.mp3")

# Initialize the serial connection to the Arduino
try:
    arduino = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)  # Adjust port as necessary
    time.sleep(2)  # Allow time for the connection to initialize
except serial.SerialException as e:
    print(f"Error initializing serial connection: {e}")
    arduino = None

# Dictionary to store patient information for different QR codes
qr_data_dict = { 
    "Patient 1": {"name": "Patient A", "room": "101", "procedure": "Knee Surgery"},
    "Patient 2": {"name": "Patient B", "room": "102", "procedure": "Appendectomy"},
    "Patient 3": {"name": "Patient C", "room": "103", "procedure": "Endoscopy"},
    "Patient 4": {"name": "Patient D", "room": "104", "procedure": "Dialysis"},
}

lastData = None  # Variable to keep track of the last scanned QR code
last_scan_time = 0  # Timestamp of the last scan
cooldown_period = 40  # Cooldown period in seconds

# Initialize the Tkinter window
window = tk.Tk()
window.title("Robot Controls")
window.geometry("800x400")  # Set window size

# Create a frame on the left side for control buttons
button_frame = tk.Frame(window)
button_frame.pack(side=tk.LEFT, fill=tk.Y, padx=20, pady=20)

# Functions to send commands to Arduino via serial
def send_to_arduino(command):
    if arduino and arduino.is_open:
        try:
            arduino.write(f"{command}\n".encode('utf-8'))  # Send command with a newline
            print(f"Sent to Arduino: {command}")
        except serial.SerialException as e:
            print(f"Error writing to serial: {e}")
    else:
        print("Arduino is not connected or serial port is closed.")

# Placeholder functions for the buttons
def start():
    update_status("Robot Started", "green")
    send_to_arduino("start")  # Send "start" to Arduino

def speed_high():
    update_status("Speed set to HIGH", "green")
    send_to_arduino("speed_high")  # Send "speed_high" to Arduino

def speed_medium():
    update_status("Speed set to MEDIUM", "green")
    send_to_arduino("speed_medium")  # Send "speed_medium" to Arduino

def speed_low():
    update_status("Speed set to LOW", "green")
    send_to_arduino("speed_low")  # Send "speed_low" to Arduino

def next_song():
    update_status("Playing Next Song", "green")
    send_to_arduino("next_song")  # Send "next_song" to Arduino

# Add buttons to the frame
start_button = tk.Button(button_frame, text="Start", command=start)
start_button.pack(fill=tk.X, pady=10)

speed_high_button = tk.Button(button_frame, text="Speed High", command=speed_high)
speed_high_button.pack(fill=tk.X, pady=10)

speed_medium_button = tk.Button(button_frame, text="Speed Medium", command=speed_medium)
speed_medium_button.pack(fill=tk.X, pady=10)

speed_low_button = tk.Button(button_frame, text="Speed Low", command=speed_low)
speed_low_button.pack(fill=tk.X, pady=10)

next_song_button = tk.Button(button_frame, text="Next Song", command=next_song)
next_song_button.pack(fill=tk.X, pady=10)

# Info label for patient data
font_settings = ("Helvetica", 16)
info_label = tk.Label(window, text="Welcome to Patient Info", padx=20, pady=20, font=font_settings)
info_label.pack(expand=True)

# Status label for QR scan updates
status_label = tk.Label(window, text="", padx=20, pady=10, font=("Helvetica", 12), fg="blue")
status_label.pack()

# Function to update the patient information in the Tkinter window
def update_patient_info(name, room, procedure):
    info_label.config(text=f"Name: {name}\nRoom: {room}\nProcedure: {procedure}")

# Function to update the status label
def update_status(message, color="blue"):
    status_label.config(text=message, fg=color)

# Main loop for processing video and displaying QR data
while True:
    success, img = cap.read()  # Capture a frame from the video
    if not success:
        print("Failed to capture video")
        break

    decoded_barcodes = decode(img)  # Decode the QR code from the frame

    if decoded_barcodes:  # If any QR codes are detected
        for barcode in decoded_barcodes:
            myData = barcode.data.decode('utf-8')  # Convert the data to a readable format
            current_time = time.time()  # Get the current time

            # Trigger only if a new QR code is detected and cooldown has elapsed
            if myData != lastData and (current_time - last_scan_time) > cooldown_period:
                print("New QR Code: ", myData)  # Print the new data to the console
                lastData = myData  # Update the last scanned data
                last_scan_time = current_time  # Update the last scan timestamp

                pygame.mixer.Sound.play(beep_sound)  # Play beep sound

                # Check if the QR code data exists in the dictionary
                if myData in qr_data_dict:
                    patient_info = qr_data_dict[myData]
                    update_patient_info(patient_info['name'], patient_info['room'], patient_info['procedure'])
                    update_status("QR Code Scanned Successfully!", "green")
                    send_to_arduino("qr")  # Send "qr" to Arduino
                else:
                    update_status("Unknown QR Code", "red")

                # Draw a polygon around the QR code
                pts = np.array([barcode.polygon], np.int32)
                pts = pts.reshape((-1, 1, 2))
                cv2.polylines(img, [pts], True, (255, 0, 255), 5)

                # Display the decoded text on the image
                pts2 = barcode.rect
                cv2.putText(img, "QR Captured: " + myData, (pts2[0], pts2[1] - 10), 
                            cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 0), 2)
            elif myData == lastData:
                update_status("Same QR Code Detected. Waiting for a new QR Code.", "orange")

    else:
        # If no QR codes detected, clear the last data
        lastData = None
        update_status("No QR Code detected", "blue")

    # Show the live feed of the camera
    cv2.imshow('Result', img)

    # Update the Tkinter window
    window.update()  # This allows the Tkinter window to refresh

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break  # Allow the user to quit the program by pressing 'q'

# Release the camera if the loop is exited
cap.release()
cv2.destroyAllWindows()
window.destroy()  # Close the Tkinter window

# Close the serial connection
if arduino:
    arduino.close()
