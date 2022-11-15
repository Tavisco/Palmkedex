
## Generate APP Color Icon from PNG
convert poke.png -gravity Center -crop 22x22+0+0 -filter point -background '#00FF00' -alpha remove -depth 8 -type palette BMP3:poke-8.bmp

## Generate APP Greysacale Icon from PNG
convert poke.png -colorspace Gray -ordered-dither checks -gravity Center -crop 22x22+0+0 -filter point -background white -alpha remove -depth 1 -type palette BMP3:poke-1.bmp

## Format Pokémon sprites in bulk to BMP
### 8 BPP 
```
for file in *.png; do convert "$file" -background white -alpha remove -depth 8 -type palette BMP3:"`basename \"$file\" .png`"-8.bmp; done
```

### 4 BPP
```
for file in *.png; do convert "$file" -background white -alpha remove -colorspace gray -depth 4 -type palette BMP3:"`basename \"$file\" .png`"-4.bmp; done
```
