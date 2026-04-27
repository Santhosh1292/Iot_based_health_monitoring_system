import cv2
import easyocr
from gtts import gTTS
import os

# Initialize EasyOCR reader for English (replace 'en' with 'es' for Spanish)
reader = easyocr.Reader(['en'])

# Open webcam
cap = cv2.VideoCapture(0)

print("🎥 Webcam started. Press SPACE to capture text, ESC to exit.")

while True:
    ret, frame = cap.read()
    if not ret:
        print("❌ Failed to grab frame")
        break

    # Show the video feed
    cv2.imshow("Webcam OCR", frame)

    # Wait for key press
    key = cv2.waitKey(1) & 0xFF

    if key == 27:  # ESC key to exit
        print("🛑 Exiting...")
        break
    elif key == 32:  # SPACE key to capture
        print("📸 Capturing frame...")
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        # Run OCR
        results = reader.readtext(gray)
        text = " ".join([res[1] for res in results]).strip()

        if text:
            print("📄 Detected text:", text)
            try:
                # Generate speech
                tts = gTTS(text=text, lang='en')  # change lang='es' for Spanish
                tts.save("ocr_audio.mp3")
                os.system("start ocr_audio.mp3")  # Windows playback
                print("🔊 Speaking out text...")
            except Exception as e:
                print("⚠️ Error generating/playing audio:", e)
        else:
            print("❌ No text detected in this frame.")

# Release resources
cap.release()
cv2.destroyAllWindows()