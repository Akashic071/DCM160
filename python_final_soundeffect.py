import serial
import winsound
import time


SERIAL_PORT = "COM4"

BAUD_RATE = 115200


LIGHT_SOUND = r"E:\IM_music_demo\soft.wav"

HEAVY_SOUND = r"E:\IM_music_demo\hard.wav"



print("Connecting to STM32...")

ser = serial.Serial(
    port=SERIAL_PORT,
    baudrate=BAUD_RATE,
    timeout=0.05
)



time.sleep(1)



ser.reset_input_buffer()


print("Connected:", SERIAL_PORT)

print("Waiting for interaction...")

print()




def play_light():
    winsound.PlaySound(
        LIGHT_SOUND,
        winsound.SND_FILENAME |
        winsound.SND_ASYNC
    )


def play_heavy():
    winsound.PlaySound(
        HEAVY_SOUND,
        winsound.SND_FILENAME |
        winsound.SND_ASYNC
    )




try:

    while True:

        
        raw = ser.readline()


        if not raw:
            continue


        message = raw.decode(
            "utf-8",
            errors="ignore"
        ).strip()


        if not message:
            continue


      
        print("STM32:", message)


       

        if message == "SOUND_LIGHT":

            print("PLAY LIGHT")

            play_light()


      

        elif message == "SOUND_HEAVY":

            print("PLAY HEAVY")

            play_heavy()


       

        elif message == "SOUND_NONE":

        
            pass


except KeyboardInterrupt:

    print()

    print("Stopping...")


finally:

    if ser.is_open:
        ser.close()


 
    winsound.PlaySound(
        None,
        winsound.SND_PURGE
    )


    print("Serial port closed.")