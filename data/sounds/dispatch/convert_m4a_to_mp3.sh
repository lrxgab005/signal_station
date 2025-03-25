#!/bin/bash

# Find all .m4a files in the current directory and convert them to MP3
for file in *.m4a; do
  # Check if file exists and is a regular file
  if [ -f "$file" ]; then
    # Get filename without extension
    filename="${file%.*}"
    # Convert to MP3 using ffmpeg
    if ffmpeg -i "$file" -codec:a libmp3lame -qscale:a 2 "${filename}.mp3"; then
      # Remove original file only if conversion was successful
      rm "$file"
    else
      echo "Error converting $file"
    fi
  fi
done