import numpy as np
import librosa
from pydub import AudioSegment
import os

y, sr = librosa.load("audio.mp3", sr=44100, mono=True)
pcm_data = (y * 32767).astype(np.int16)
pcm_data.tofile("audio.pcm")