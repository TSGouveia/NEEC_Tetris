import cv2
import numpy as np
import pytube
import argparse
import time
import os
import atexit
import signal
import socket
from youtubesearchpython import VideosSearch
import paho.mqtt.client as mqtt
import json

def search_youtube(query):
    videos_search = VideosSearch(query, limit=1)
    result = videos_search.result()
    if result['result']:
        return result['result'][0]['link']
    else:
        print("No results found.")
        return None

def download_youtube_video(url):
    yt = pytube.YouTube(url)
    stream = yt.streams.filter(file_extension='mp4').first()
    video_path = stream.download()
    return video_path

def delete_video_file(file_path):
    if os.path.exists(file_path):
        os.remove(file_path)
        print(f"Deleted video file: {file_path}")

def on_connect(client, userdata, flags, rc):
    print(f"Connected to MQTT broker with result code {rc}")

def publish_rgb_matrix(client, rgb_matrix, topic):
    data = json.dumps(rgb_matrix)
    client.publish(topic, data)
    print("Published RGB matrix to MQTT")

def process_video_frames(video_path, mqtt_client, topic):
    cap = cv2.VideoCapture(video_path)

    if not cap.isOpened():
        print("Error: Could not open video.")
        return

    fps = cap.get(cv2.CAP_PROP_FPS)
    frame_delay = 1.0 / fps  # frame delay in seconds
    next_frame_time = time.time() + frame_delay

    while cap.isOpened():
        start_time = time.time()

        ret, frame = cap.read()
        if not ret:
            break

        # Downscale the image to 32x18
        downscaled_image = cv2.resize(frame, (32, 18), interpolation=cv2.INTER_AREA)

        # Convert downscaled image to a list of RGB values
        rgb_matrix = downscaled_image.reshape(-1, 3).tolist()

        # Clear the previous prints and print the matrix to the console
        print("\033c", end="")  # Clear the terminal
        print("RGB Matrix (32x18):")
        for i in range(18):
            row = rgb_matrix[i * 32:(i + 1) * 32]
            print(row)

        # Publish RGB matrix to MQTT
        publish_rgb_matrix(mqtt_client, rgb_matrix, topic)

        # Upscale the image to a larger size for display (e.g., 640x360)
        upscaled_image = cv2.resize(downscaled_image, (640, 360), interpolation=cv2.INTER_NEAREST)

        # Display the upscaled image
        cv2.imshow('Upscaled Downscaled Video Frame', upscaled_image)

        # Calculate the remaining time to wait until the next frame
        current_time = time.time()
        remaining_time = next_frame_time - current_time
        print(f"Frame Processing Time: {current_time - start_time:.2f} seconds")

        if remaining_time > 0:
            time.sleep(remaining_time)
        else:
            print("Warning: Processing is too slow, skipping sleep.")

        # Update the time for the next frame
        next_frame_time = time.time() + frame_delay

        if cv2.waitKey(1) == 27:  # Press 'Esc' to exit the loop
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process a YouTube video URL or search query.')
    parser.add_argument('query_or_url', type=str, help='YouTube search query or URL')
    parser.add_argument('mqtt_topic', type=str, help='MQTT topic to publish RGB data')
    parser.add_argument('--mqtt_broker_ip', type=str, default='localhost', help='IP address of MQTT broker (default: localhost)')
    args = parser.parse_args()

    input_value = args.query_or_url
    mqtt_topic = args.mqtt_topic
    mqtt_broker_ip = args.mqtt_broker_ip

    if 'youtube.com' in input_value or 'youtu.be' in input_value:
        youtube_url = input_value
    else:
        youtube_url = search_youtube(input_value)

    if youtube_url:
        video_path = download_youtube_video(youtube_url)

        # Register cleanup function to delete the video file at exit
        atexit.register(delete_video_file, video_path)

        # Handle interruption signals to ensure cleanup
        signal.signal(signal.SIGINT, lambda signum, frame: exit(0))

        # Initialize MQTT client
        mqtt_client = mqtt.Client()
        mqtt_client.on_connect = on_connect
        mqtt_client.connect(mqtt_broker_ip, 1883, 60)  # Connect to MQTT broker using specified IP
        mqtt_client.loop_start()

        input("Press Enter to start processing and publishing video frames...")

        process_video_frames(video_path, mqtt_client, mqtt_topic)

        mqtt_client.loop_stop()
        mqtt_client.disconnect()
