import pyautogui
from PIL import Image
import numpy as np
import cv2
import time

def capture_and_downscale():
    while True:
        # Capture the screen
        screenshot = pyautogui.screenshot()
        
        # Convert the screenshot to a NumPy array
        screenshot_np = np.array(screenshot)
        
        # Convert the image from RGB to BGR (which OpenCV uses)
        screenshot_bgr = cv2.cvtColor(screenshot_np, cv2.COLOR_RGB2BGR)
        
        # Downscale the image to 32x18
        downscaled_image = cv2.resize(screenshot_bgr, (32, 18), interpolation=cv2.INTER_AREA)
        
        # Convert downscaled image to a list of RGB values
        rgb_matrix = downscaled_image.reshape(-1, 3).tolist()
        
        # Clear the previous prints and print the matrix to the console
        print("\033c", end="")  # Clear the terminal
        print("RGB Matrix (32x18):")
        for i in range(18):
            row = rgb_matrix[i*32:(i+1)*32]
            print(row)
        
        # Upscale the image to a larger size for display (e.g., 640x360)
        upscaled_image = cv2.resize(downscaled_image, (640, 360), interpolation=cv2.INTER_NEAREST)
        
        # Display the upscaled image
        cv2.imshow('Upscaled Downscaled Screen', upscaled_image)
        
        # Add a small delay to control the frame rate and allow for the window to be closed
        if cv2.waitKey(1) == 27:  # Press 'Esc' to exit the loop
            break
        
        # Add a delay to slow down the loop and reduce print frequency
        #time.sleep(0.1)  # Adjust this value to control the speed

    # Cleanup
    cv2.destroyAllWindows()

if __name__ == "__main__":
    capture_and_downscale()
