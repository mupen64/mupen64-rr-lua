#
# Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
# 
# SPDX-License-Identifier: GPL-2.0-or-later
# 

# Checks whether an encode has the correct frame pacing.
# Only works on encodes created in SM64 (script assumes 2 VIs per input poll) and with no lag additional lag frames.
 
import cv2
import numpy as np

# pip install opencv-python

def check_second_frame_similarity(video_path, threshold=10):
    cap = cv2.VideoCapture(video_path)

    if not cap.isOpened():
        print(f"Error: Could not open video file {video_path}")
        return

    frame_count = 0
    prev_frame = None
    mismatch_found = False

    cap.read()

    while True:

        ret, frame = cap.read()
        if not ret:
            break

        gray_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        if frame_count % 2 == 1:
            if prev_frame is not None:
                diff = cv2.absdiff(gray_frame, prev_frame)
                mean_diff = np.mean(diff)
                if mean_diff > threshold:
                    print(f"Mismatch found at frame {frame_count} with mean difference {mean_diff:.2f}")
                    frame_count = frame_count + 1
                    mismatch_found = True
        

        prev_frame = gray_frame
        frame_count += 1

    cap.release()

    if not mismatch_found:
        print("lgtm")

if __name__ == "__main__":
    video_file_path = "input.avi"
    check_second_frame_similarity(video_file_path, 10)
