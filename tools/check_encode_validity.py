#
# Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
# 
# SPDX-License-Identifier: GPL-2.0-or-later
# 

# Checks whether an encode has the correct frame pacing.
# Only works on encodes created in SM64 (script assumes 2 VIs per input poll) and with no lag additional lag frames.
 
import cv2
import numpy as np

def frames_equal(a, b):
    return np.array_equal(a, b)


def find_pattern_anomalies(video_path):
    cap = cv2.VideoCapture(video_path)
    frames = []
    while True:
        ret, frame = cap.read()
        if not ret:
            break
        frames.append(frame)
    cap.release()

    anomalies = []
    total = len(frames)
    i = 1
    while i + 3 < total:
        f0, f1, f2, f3 = frames[i:i+4]
        pair1_same = frames_equal(f0, f1)
        pair2_same = frames_equal(f2, f3)
        cross_pair_diff = not frames_equal(f1, f2)
        if not (pair1_same and pair2_same and cross_pair_diff):
            anomalies.extend([i, i+1, i+2, i+3])
        i += 2

    return sorted(set(anomalies))

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
            cv2.putText(display, "ANOMALY!", (50,50),
                        cv2.FONT_HERSHEY_SIMPLEX, 1, (0,0,255), 2)

        cv2.putText(display, f"Frame {current_frame}", (50,100),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255,255,255), 2)

        cv2.imshow("Video Viewer", display)

        key = cv2.waitKey(0) & 0xFF

        if cv2.getWindowProperty("Video Viewer", cv2.WND_PROP_VISIBLE) < 1:
            break

        if key == ord('d'):
            current_frame = current_frame + 1
        elif key == ord('a'):
            current_frame = current_frame - 1
        elif key == ord('n'):
            next_anomaly = next((f for f in anomalies if f > current_frame), total_frames-1)
            current_frame = next_anomaly
        elif key == ord('p'):
            prev_anomaly = max((f for f in anomalies if f < current_frame), 0)
            current_frame = prev_anomaly
        
        current_frame = max(0, min(current_frame, total_frames - 1))

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    video_path = "..\\build\\out\\input.avi"

    anomalies = find_pattern_anomalies(video_path)
    print(f"Found {len(anomalies)} anomalies:", anomalies)

    video_viewer(video_path, anomalies)