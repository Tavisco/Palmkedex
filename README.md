# Palmkedex

convert fire.png -background white -alpha remove -colorspace gray -depth 4 -type palette BMP3:fire-4.bmp

## Format images in bulk
### 8 BPP 
```
for file in *.png; do convert "$file" -background white -alpha remove -depth 8 -type palette BMP3:"`basename \"$file\" .png`"-8.bmp; done
```

### 4 BPP
```
for file in *.png; do convert "$file" -background white -alpha remove -colorspace gray -depth 4 -type palette BMP3:"`basename \"$file\" .png`"-4.bmp; done
```