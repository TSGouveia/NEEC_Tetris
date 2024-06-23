import cv2
import os
import sys

def extract_and_save_frames(video_path, frame_interval):
    cap = cv2.VideoCapture(video_path)
    
    if not cap.isOpened():
        print("Error: Could not open video.")
        return []
    
    frame_count = 0
    frames = []

    while True:
        ret, frame = cap.read()
        if not ret:
            break
        
        if frame_count % frame_interval == 0:
            frames.append(frame)
        
        frame_count += 1
    
    cap.release()
    return frames

def create_video(frames, output_video_path, original_fps, frame_interval):
    if not frames:
        print("Error: No frames to create video.")
        return

    height, width, layers = frames[0].shape
    reduced_fps = original_fps // frame_interval
    
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    out = cv2.VideoWriter(output_video_path, fourcc, reduced_fps, (width, height))

    for frame in frames:
        out.write(frame)
    
    out.release()
    print(f"New video created at {output_video_path}")

def process_video(input_video_path, original_fps, frame_interval=10):
    # Generate the output video name
    base_name = os.path.basename(input_video_path)
    name, ext = os.path.splitext(base_name)
    output_video_path = f"{name}_reduced{ext}"
    
    frames = extract_and_save_frames(input_video_path, frame_interval)
    create_video(frames, output_video_path, original_fps, frame_interval)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python video_extrapolate.py <input_video_path> <original_fps> <frame_interval>")
    else:
        input_video_path = sys.argv[1]
        original_fps = int(sys.argv[2])
        frame_interval = int(sys.argv[3])
        process_video(input_video_path, original_fps, frame_interval)
