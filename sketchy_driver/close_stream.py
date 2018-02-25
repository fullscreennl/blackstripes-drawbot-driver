import os

filename = "../build/movement.js"

with open(filename, 'rb+') as filehandle:
    filehandle.seek(-1, os.SEEK_END)
    filehandle.truncate()

with open(filename, 'a') as filehandle:
    filehandle.write(']')
