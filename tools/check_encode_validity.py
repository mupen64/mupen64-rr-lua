#
# Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
# 
# SPDX-License-Identifier: GPL-2.0-or-later
# 

# Checks whether an encode has the correct frame pacing.
# Only works on encodes created in SM64 (script assumes 2 VIs per input poll) and with no lag additional lag frames.
 
import cv2

def find_duplicate_frames(video_path):
    cap = cv2.VideoCapture(video_path)
    last_frame = None
    anomalies = []

    frame_index = 0
    while True:
        ret, frame = cap.read()
        if not ret:
            break
        if last_frame is not None:
            if (frame == last_frame).all():  # exact duplicate
                anomalies.append(frame_index)
        last_frame = frame
        frame_index += 1

    cap.release()
    return anomalies

def video_viewer(video_path, anomalies):
    cap = cv2.VideoCapture(video_path)
    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    current_frame = 0

    while True:
        cap.set(cv2.CAP_PROP_POS_FRAMES, current_frame)
        ret, frame = cap.read()
        if not ret:
            break

        display = frame.copy()
        if current_frame in anomalies:
            cv2.putText(display, "ANOMALY!", (50,50), cv2.FONT_HERSHEY_SIMPLEX,
                        1, (0,0,255), 2)
        cv2.imshow("Video Viewer", display)
        key = cv2.waitKey(0) & 0xFF

        # Controls
        if key == ord('q'):
            break
        elif key == ord('d'):  # next frame
            current_frame = min(current_frame + 1, total_frames - 1)
        elif key == ord('a'):  # previous frame
            current_frame = max(current_frame - 1, 0)
        elif key == ord('n'):  # next anomaly
            next_anomaly = next((f for f in anomalies if f > current_frame), total_frames-1)
            current_frame = next_anomaly
        elif key == ord('p'):  # previous anomaly
            prev_anomaly = max((f for f in anomalies if f < current_frame), 0)
            current_frame = prev_anomaly

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    video_path = "..\\build\\out\\input.avi"
    anomalies = find_duplicate_frames(video_path)
    print(f"Found {len(anomalies)} duplicate frames:", anomalies)
    video_viewer(video_path, anomalies)